#include "IRGenVisitor.h"
#include <cstring>
#include <any>
#include <type_traits>

template<typename T, typename AnyType>
T castAny(AnyType a) {
    if constexpr (std::is_same_v<AnyType, std::any>) {
        return std::any_cast<T>(a);
    } else {
        return a.template as<T>();
    }
}

//  Collecte des variables affectées dans un sous-arbre AST 
set<string> IRGenVisitor::collectAssignedVars(antlr4::tree::ParseTree* tree) {
    set<string> result;
    if (auto* assignCtx = dynamic_cast<ifccParser::AssignExprContext*>(tree)) {
        if (auto* lvVar = dynamic_cast<ifccParser::LvalueVarContext*>(assignCtx->lvalue())) {
            result.insert(getScopedName(lvVar->VAR()->getText()));
        } else if (auto* lvArr = dynamic_cast<ifccParser::LvalueArrayContext*>(assignCtx->lvalue())) {
            result.insert(getScopedName(lvArr->VAR()->getText()));
        }
    }
    if (auto* declCtx = dynamic_cast<ifccParser::DeclVarContext*>(tree)) {
        // We do not know the scoped name yet, so we just use the original name as a fallback.
        // It's a new variable anyway, so it shouldn't affect outer constants.
        result.insert(declCtx->VAR()->getText());
    }
    if (auto* declUninitCtx = dynamic_cast<ifccParser::DeclVarUninitContext*>(tree)) {
        result.insert(declUninitCtx->VAR()->getText());
    }
    if (auto* declArrCtx = dynamic_cast<ifccParser::DeclArrayContext*>(tree)) {
        result.insert(declArrCtx->VAR()->getText());
    }
    // ++x et --x modifient aussi la variable
    if (auto* preInc = dynamic_cast<ifccParser::PreIncExprContext*>(tree)) {
        result.insert(getScopedName(preInc->VAR()->getText()));
    }
    if (auto* preDec = dynamic_cast<ifccParser::PreDecExprContext*>(tree)) {
        result.insert(getScopedName(preDec->VAR()->getText()));
    }
    // x++ et x-- (statements)
    if (auto* postInc = dynamic_cast<ifccParser::PostIncStmtContext*>(tree)) {
        result.insert(getScopedName(postInc->VAR()->getText()));
    }
    if (auto* postDec = dynamic_cast<ifccParser::PostDecStmtContext*>(tree)) {
        result.insert(getScopedName(postDec->VAR()->getText()));
    }
    // Compound assignments (+=, -=, etc.)
    if (auto* plusAsgn = dynamic_cast<ifccParser::PlusAssignExprContext*>(tree)) {
        if (auto* lvVar = dynamic_cast<ifccParser::LvalueVarContext*>(plusAsgn->lvalue()))
            result.insert(getScopedName(lvVar->VAR()->getText()));
    }
    if (auto* minusAsgn = dynamic_cast<ifccParser::MinusAssignExprContext*>(tree)) {
        if (auto* lvVar = dynamic_cast<ifccParser::LvalueVarContext*>(minusAsgn->lvalue()))
            result.insert(getScopedName(lvVar->VAR()->getText()));
    }
    if (auto* mulAsgn = dynamic_cast<ifccParser::MulAssignExprContext*>(tree)) {
        if (auto* lvVar = dynamic_cast<ifccParser::LvalueVarContext*>(mulAsgn->lvalue()))
            result.insert(getScopedName(lvVar->VAR()->getText()));
    }
    if (auto* divAsgn = dynamic_cast<ifccParser::DivAssignExprContext*>(tree)) {
        if (auto* lvVar = dynamic_cast<ifccParser::LvalueVarContext*>(divAsgn->lvalue()))
            result.insert(getScopedName(lvVar->VAR()->getText()));
    }
    if (auto* modAsgn = dynamic_cast<ifccParser::ModAssignExprContext*>(tree)) {
        if (auto* lvVar = dynamic_cast<ifccParser::LvalueVarContext*>(modAsgn->lvalue()))
            result.insert(getScopedName(lvVar->VAR()->getText()));
    }
    for (size_t i = 0; i < tree->children.size(); i++) {
        set<string> childVars = collectAssignedVars(tree->children[i]);
        result.insert(childVars.begin(), childVars.end());
    }
    return result;
}

// Helpers

Type IRGenVisitor::parseType(ifccParser::TypeContext* ctx) {
    string text = ctx->getText();
    if (text == "double") return DOUBLE;
    if (text == "void") return VOID;
    if (text == "char") return INT; // char traité comme int
    return INT;
}

string IRGenVisitor::loadConst(int value) {
    string dest = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::ldconst, INT, {dest, to_string(value)});
    return dest;
}

string IRGenVisitor::loadConstDouble(double value) {
    string dest = current_cfg->create_new_tempvar(DOUBLE);
    string label = ".LCD_" + current_cfg->funcName + "_" + to_string(nextDoubleConstIndex++);
    current_cfg->doubleConstants.push_back({label, value});
    current_cfg->current_bb->add_IRInstr(IRInstr::ldconst_double, DOUBLE, {dest, label});
    return dest;
}

string IRGenVisitor::emitConversion(ExprValue& val, Type targetType) {
    if (val.type == targetType) {
        return materialize(val);
    }
    string srcVar = materialize(val);
    if (val.type == INT && targetType == DOUBLE) {
        string dest = current_cfg->create_new_tempvar(DOUBLE);
        current_cfg->current_bb->add_IRInstr(IRInstr::int_to_double, DOUBLE, {dest, srcVar});
        val.type = DOUBLE;
        val.isConstant = false;
        val.varName = dest;
        return dest;
    } else if (val.type == DOUBLE && targetType == INT) {
        string dest = current_cfg->create_new_tempvar(INT);
        current_cfg->current_bb->add_IRInstr(IRInstr::double_to_int, INT, {dest, srcVar});
        val.type = INT;
        val.isConstant = false;
        val.varName = dest;
        return dest;
    }
    return srcVar;
}

string IRGenVisitor::materialize(ExprValue& val) {
    if (val.isConstant) {
        if (val.type == DOUBLE) {
            val.varName = loadConstDouble(val.dvalue);
        } else {
            val.varName = loadConst(val.value);
        }
        val.isConstant = false;
    }
    return val.varName;
}

// Visiteurs Fonctions

antlrcpp::Any IRGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
    for (auto func : ctx->function_def()) {
        this->visit(func);
    }
    return 0;
}

antlrcpp::Any IRGenVisitor::visitFunction_def(ifccParser::Function_defContext *ctx) {
    string funcName = ctx->VAR()->getText();
    Type retType = parseType(ctx->type());

    constMap.clear();
    scopeStack.clear();
    nextVarIndex = 0;
    pushScope();

    current_cfg = new CFG(nullptr, funcName);
    current_cfg->returnType = retType;
    cfgs.push_back(current_cfg);

    exit_bb = new BasicBlock(current_cfg, ".LBB_" + funcName + "_exit");
    exit_bb->exit_true = nullptr;
    exit_bb->exit_false = nullptr;

    BasicBlock* entry = new BasicBlock(current_cfg, ".LBB_" + funcName + "_0");
    current_cfg->add_bb(entry);

    if (retType != VOID) {
        current_cfg->add_to_symbol_table("!retval", retType);
    } else {
        // Pour void, on crée quand même un retval INT pour simplifier (valeur ignorée)
        current_cfg->add_to_symbol_table("!retval", INT);
    }

    // Charger les paramètres depuis l'ABI
    if (ctx->parameters()) {
        int paramCount = 0;
        string argRegs[] = {"!edi", "!esi", "!edx", "!ecx", "!r8d", "!r9d"};
        
        auto typeNodes = ctx->parameters()->type();
        auto varNodes = ctx->parameters()->VAR();
        
        for (size_t i = 0; i < varNodes.size(); i++) {
            string paramName = declareScopedVariable(varNodes[i]->getText());
            Type paramType = parseType(typeNodes[i]);
            current_cfg->add_to_symbol_table(paramName, paramType);
            
            if (paramCount < 6) {
                if (paramType == DOUBLE) {
                    current_cfg->current_bb->add_IRInstr(IRInstr::copy, INT, {paramName, argRegs[paramCount]});
                } else {
                    current_cfg->current_bb->add_IRInstr(IRInstr::copy, INT, {paramName, argRegs[paramCount]});
                }
            } else {
                string paramReg = "!param" + to_string(paramCount);
                if (paramType == DOUBLE) {
                    current_cfg->current_bb->add_IRInstr(IRInstr::copy_double, DOUBLE, {paramName, paramReg});
                } else {
                    current_cfg->current_bb->add_IRInstr(IRInstr::copy, INT, {paramName, paramReg});
                }
            }
            paramCount++;
        }
    }

    for (auto stmt : ctx->statement()) {
        this->visit(stmt);
    }

    if (current_cfg->current_bb->exit_true == nullptr && !current_cfg->current_bb->has_return) {
        current_cfg->current_bb->exit_true = exit_bb;
    }

    current_cfg->add_bb(exit_bb);
    popScope();
    return 0;
}

antlrcpp::Any IRGenVisitor::visitDeclVar(ifccParser::DeclVarContext *ctx) {
    string originalName = ctx->VAR()->getText();
    Type declType = parseType(ctx->type());

    string varName = declareScopedVariable(originalName);
    current_cfg->add_to_symbol_table(varName, declType);

    ExprValue exprResult = castAny<ExprValue>(this->visit(ctx->expr()));

    // Conversion implicite si nécessaire
    if (exprResult.type != declType) {
        if (exprResult.isConstant) {
            if (declType == DOUBLE && exprResult.type == INT) {
                exprResult.dvalue = (double)exprResult.value;
                exprResult.type = DOUBLE;
            } else if (declType == INT && exprResult.type == DOUBLE) {
                exprResult.value = (int)exprResult.dvalue;
                exprResult.type = INT;
            }
        } else {
            emitConversion(exprResult, declType);
        }
    }

    if (exprResult.isConstant) {
        if (declType == DOUBLE) {
            string src = loadConstDouble(exprResult.dvalue);
            current_cfg->current_bb->add_IRInstr(IRInstr::copy_double, DOUBLE, {varName, src});
        } else {
            current_cfg->current_bb->add_IRInstr(IRInstr::ldconst, INT, {varName, to_string(exprResult.value)});
            constMap[varName] = exprResult.value;
        }
    } else {
        if (declType == DOUBLE) {
            current_cfg->current_bb->add_IRInstr(IRInstr::copy_double, DOUBLE, {varName, exprResult.varName});
        } else {
            current_cfg->current_bb->add_IRInstr(IRInstr::copy, INT, {varName, exprResult.varName});
            constMap.erase(varName);
        }
    }

    return 0;
}

antlrcpp::Any IRGenVisitor::visitDeclVarUninit(ifccParser::DeclVarUninitContext *ctx) {
    string originalName = ctx->VAR()->getText();
    Type declType = parseType(ctx->type());

    string varName = declareScopedVariable(originalName);
    current_cfg->add_to_symbol_table(varName, declType);

    return 0;
}

antlrcpp::Any IRGenVisitor::visitDeclArray(ifccParser::DeclArrayContext *ctx) {
    string originalName = ctx->VAR()->getText();
    Type elemType = parseType(ctx->type());
    int arraySize = stoi(ctx->CONST()->getText());

    string varName = declareScopedVariable(originalName);
    current_cfg->add_array_to_symbol_table(varName, elemType, arraySize);
    return 0;
}

antlrcpp::Any IRGenVisitor::visitAssignExpr(ifccParser::AssignExprContext *ctx) {
    // 1. Évaluer la rvalue (expression à droite du '=')
    ExprValue exprResult = castAny<ExprValue>(this->visit(ctx->expr()));

    // 2. Evaluer la lvalue (expression a gauche du '=') pour obtenir l'adresse cible
    LvalueResult lv = castAny<LvalueResult>(this->visit(ctx->lvalue()));
    Type varType = lv.type;

    // 3. Conversion implicite de la rvalue vers le type de la lvalue
    if (exprResult.type != varType) {
        if (exprResult.isConstant) {
            if (varType == DOUBLE && exprResult.type == INT) {
                exprResult.dvalue = (double)exprResult.value;
                exprResult.type = DOUBLE;
            } else if (varType == INT && exprResult.type == DOUBLE) {
                exprResult.value = (int)exprResult.dvalue;
                exprResult.type = INT;
            }
        } else {
            emitConversion(exprResult, varType);
        }
    }

    // 4. Sauvegarder l'info de constante avant matérialisation
    bool wasConstant = exprResult.isConstant;
    int constValue = exprResult.value;

    // 5. Matérialiser la rvalue dans un temporaire
    string valVar = materialize(exprResult);

    // 6. Émettre wmem : écrire la valeur à l'adresse calculée
    if (varType == DOUBLE) {
        current_cfg->current_bb->add_IRInstr(IRInstr::wmem_double, DOUBLE, {lv.addrVar, valVar});
    } else {
        current_cfg->current_bb->add_IRInstr(IRInstr::wmem, INT, {lv.addrVar, valVar});
    }

    // 7. Propagation de constantes
    if (auto* lvVar = dynamic_cast<ifccParser::LvalueVarContext*>(ctx->lvalue())) {
        string varName = getScopedName(lvVar->VAR()->getText());
        if (wasConstant && varType == INT) {
            constMap[varName] = constValue;
        } else {
            constMap.erase(varName);
        }
    }

    // 8. Retourner la valeur assignée (pour chaînage a = b = 1)
    ExprValue result;
    result.isConstant = false;
    result.varName = valVar;
    result.type = varType;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitLvalueVar(ifccParser::LvalueVarContext *ctx) {
    string varName = getScopedName(ctx->VAR()->getText());
    Type varType = current_cfg->get_var_type(varName);

    string addrTemp = current_cfg->create_new_tempvar(ADDR);
    current_cfg->current_bb->add_IRInstr(IRInstr::lea, ADDR, {addrTemp, varName});

    LvalueResult lv;
    lv.addrVar = addrTemp;
    lv.type = varType;
    return lv;
}

antlrcpp::Any IRGenVisitor::visitLvalueArray(ifccParser::LvalueArrayContext *ctx) {
    string varName = getScopedName(ctx->VAR()->getText());
    Type elemType = current_cfg->get_array_element_type(varName);
    int elemSize = typeSize(elemType);

    ExprValue indexExpr = castAny<ExprValue>(this->visit(ctx->expr()));

    string baseAddr = current_cfg->create_new_tempvar(ADDR);
    current_cfg->current_bb->add_IRInstr(IRInstr::lea, ADDR, {baseAddr, varName});

    string indexVar = materialize(indexExpr);

    string elemSizeVar = loadConst(elemSize);
    string offsetInt = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::mul, INT, {offsetInt, indexVar, elemSizeVar});

    string resultAddr = current_cfg->create_new_tempvar(ADDR);
    current_cfg->current_bb->add_IRInstr(IRInstr::add_addr, ADDR, {resultAddr, baseAddr, offsetInt});

    LvalueResult lv;
    lv.addrVar = resultAddr;
    lv.type = elemType;
    return lv;
}

antlrcpp::Any IRGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
    ExprValue exprResult = castAny<ExprValue>(this->visit(ctx->expr()));
    Type retType = current_cfg->returnType;

    if (exprResult.type != retType) {
        if (exprResult.isConstant) {
            if (retType == DOUBLE && exprResult.type == INT) {
                exprResult.dvalue = (double)exprResult.value;
                exprResult.type = DOUBLE;
            } else if (retType == INT && exprResult.type == DOUBLE) {
                exprResult.value = (int)exprResult.dvalue;
                exprResult.type = INT;
            }
        } else {
            emitConversion(exprResult, retType);
        }
    }

    if (exprResult.isConstant) {
        if (retType == DOUBLE) {
            string src = loadConstDouble(exprResult.dvalue);
            current_cfg->current_bb->add_IRInstr(IRInstr::copy_double, DOUBLE, {"!retval", src});
        } else {
            current_cfg->current_bb->add_IRInstr(IRInstr::ldconst, INT, {"!retval", to_string(exprResult.value)});
        }
    } else {
        if (retType == DOUBLE) {
            current_cfg->current_bb->add_IRInstr(IRInstr::copy_double, DOUBLE, {"!retval", exprResult.varName});
        } else {
            current_cfg->current_bb->add_IRInstr(IRInstr::copy, INT, {"!retval", exprResult.varName});
        }
    }

    current_cfg->current_bb->exit_true = exit_bb;
    current_cfg->current_bb->exit_false = nullptr;
    current_cfg->current_bb->has_return = true;

    BasicBlock* dead_bb = new BasicBlock(current_cfg, current_cfg->new_BB_name());
    current_cfg->add_bb(dead_bb);

    return 0;
}

antlrcpp::Any IRGenVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx) {
    ExprValue val;
    val.isConstant = true;
    val.value = stoi(ctx->CONST()->getText());
    val.dvalue = 0.0;
    val.type = INT;
    return val;
}

antlrcpp::Any IRGenVisitor::visitConstDoubleExpr(ifccParser::ConstDoubleExprContext *ctx) {
    ExprValue val;
    val.isConstant = true;
    val.dvalue = stod(ctx->CONST_DOUBLE()->getText());
    val.value = 0;
    val.type = DOUBLE;
    return val;
}

antlrcpp::Any IRGenVisitor::visitCharExpr(ifccParser::CharExprContext *ctx) {
    string text = ctx->CHAR_CONST()->getText();
    // text ressemble a 'a' ou '\n', on enleve les quotes
    string inner = text.substr(1, text.size() - 2);

    int charValue = 0;
    if (inner.size() == 1) {
        charValue = (unsigned char)inner[0];
    } else if (inner.size() == 2 && inner[0] == '\\') {
        switch (inner[1]) {
            case 'n': charValue = '\n'; break;
            case 'r': charValue = '\r'; break;
            case 't': charValue = '\t'; break;
            case '0': charValue = '\0'; break;
            case '\\': charValue = '\\'; break;
            case '\'': charValue = '\''; break;
            default: charValue = (unsigned char)inner[1]; break;
        }
    }

    ExprValue val;
    val.isConstant = true;
    val.value = charValue;
    val.dvalue = 0.0;
    val.type = INT;
    return val;
}

antlrcpp::Any IRGenVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
    string varName = getScopedName(ctx->VAR()->getText());
    Type varType = current_cfg->get_var_type(varName);

    // Propagation des constantes (seulement pour les int)
    if (varType == INT) {
        auto it = constMap.find(varName);
        if (it != constMap.end()) {
            ExprValue val;
            val.isConstant = true;
            val.value = it->second;
            val.dvalue = 0.0;
            val.type = INT;
            return val;
        }
    }

    ExprValue val;
    val.isConstant = false;
    val.varName = varName;
    val.type = varType;
    val.value = 0;
    val.dvalue = 0.0;
    return val;
}

antlrcpp::Any IRGenVisitor::visitArrayAccessExpr(ifccParser::ArrayAccessExprContext *ctx) {
    string varName = getScopedName(ctx->VAR()->getText());
    Type elemType = current_cfg->get_array_element_type(varName);
    int elemSize = typeSize(elemType);

    ExprValue indexExpr = castAny<ExprValue>(this->visit(ctx->expr()));

    string baseAddr = current_cfg->create_new_tempvar(ADDR);
    current_cfg->current_bb->add_IRInstr(IRInstr::lea, ADDR, {baseAddr, varName});

    string indexVar = materialize(indexExpr);
    string elemSizeVar = loadConst(elemSize);
    string offsetInt = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::mul, INT, {offsetInt, indexVar, elemSizeVar});

    string elemAddr = current_cfg->create_new_tempvar(ADDR);
    current_cfg->current_bb->add_IRInstr(IRInstr::add_addr, ADDR, {elemAddr, baseAddr, offsetInt});

    string dest = current_cfg->create_new_tempvar(elemType);
    if (elemType == DOUBLE) {
        current_cfg->current_bb->add_IRInstr(IRInstr::rmem_double, DOUBLE, {dest, elemAddr});
    } else {
        current_cfg->current_bb->add_IRInstr(IRInstr::rmem, INT, {dest, elemAddr});
    }

    ExprValue result;
    result.isConstant = false;
    result.varName = dest;
    result.type = elemType;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitMulDivModExpr(ifccParser::MulDivModExprContext *ctx) {
    ExprValue left = castAny<ExprValue>(this->visit(ctx->expr(0)));
    ExprValue right = castAny<ExprValue>(this->visit(ctx->expr(1)));
    string op = ctx->children[1]->getText();

    Type resultType = (op == "%") ? INT : promoteType(left.type, right.type);

    // Constant folding
    if (left.isConstant && right.isConstant) {
        ExprValue result;
        result.isConstant = true;
        result.type = resultType;
        if (op == "%") {
            result.value = (right.value != 0) ? left.value % right.value : 0;
            result.dvalue = 0.0;
        } else if (resultType == DOUBLE) {
            double lv = (left.type == INT) ? (double)left.value : left.dvalue;
            double rv = (right.type == INT) ? (double)right.value : right.dvalue;
            result.dvalue = (op == "*") ? lv * rv : lv / rv;
            result.value = 0;
        } else {
            if (op == "*") result.value = left.value * right.value;
            else result.value = (right.value != 0) ? left.value / right.value : 0;
            result.dvalue = 0.0;
        }
        return result;
    }

    // Élimination des éléments neutres
    if (op == "*") {
        if (resultType == INT) {
            if (right.isConstant && right.value == 1) return left;
            if (left.isConstant && left.value == 1) return right;
            if (right.isConstant && right.value == 0) return right;
            if (left.isConstant && left.value == 0) return left;
        }
    }
    if (op == "/") {
        if (resultType == INT && right.isConstant && right.value == 1) return left;
    }

    // Conversion implicite si les types diffèrent
    string leftVar = emitConversion(left, resultType);
    string rightVar = emitConversion(right, resultType);

    string dest = current_cfg->create_new_tempvar(resultType);

    if (op == "%") {
        current_cfg->current_bb->add_IRInstr(IRInstr::mod_int, INT, {dest, leftVar, rightVar});
    } else if (resultType == DOUBLE) {
        if (op == "*")
            current_cfg->current_bb->add_IRInstr(IRInstr::mul_double, DOUBLE, {dest, leftVar, rightVar});
        else
            current_cfg->current_bb->add_IRInstr(IRInstr::div_double, DOUBLE, {dest, leftVar, rightVar});
    } else {
        if (op == "*")
            current_cfg->current_bb->add_IRInstr(IRInstr::mul, INT, {dest, leftVar, rightVar});
        else
            current_cfg->current_bb->add_IRInstr(IRInstr::div_int, INT, {dest, leftVar, rightVar});
    }

    ExprValue result;
    result.isConstant = false;
    result.varName = dest;
    result.type = resultType;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitAddSubExpr(ifccParser::AddSubExprContext *ctx) {
    ExprValue left = castAny<ExprValue>(this->visit(ctx->expr(0)));
    ExprValue right = castAny<ExprValue>(this->visit(ctx->expr(1)));
    string op = ctx->children[1]->getText();

    Type resultType = promoteType(left.type, right.type);

    // Constant folding
    if (left.isConstant && right.isConstant) {
        ExprValue result;
        result.isConstant = true;
        result.type = resultType;
        if (resultType == DOUBLE) {
            double lv = (left.type == INT) ? (double)left.value : left.dvalue;
            double rv = (right.type == INT) ? (double)right.value : right.dvalue;
            result.dvalue = (op == "+") ? lv + rv : lv - rv;
            result.value = 0;
        } else {
            result.value = (op == "+") ? left.value + right.value : left.value - right.value;
            result.dvalue = 0.0;
        }
        return result;
    }

    // Élimination des éléments neutres
    if (resultType == INT) {
        if (op == "+") {
            if (right.isConstant && right.value == 0) return left;
            if (left.isConstant && left.value == 0) return right;
        } else if (op == "-") {
            if (right.isConstant && right.value == 0) return left;
        }
    }

    string leftVar = emitConversion(left, resultType);
    string rightVar = emitConversion(right, resultType);

    string dest = current_cfg->create_new_tempvar(resultType);

    if (resultType == DOUBLE) {
        if (op == "+")
            current_cfg->current_bb->add_IRInstr(IRInstr::add_double, DOUBLE, {dest, leftVar, rightVar});
        else
            current_cfg->current_bb->add_IRInstr(IRInstr::sub_double, DOUBLE, {dest, leftVar, rightVar});
    } else {
        if (op == "+")
            current_cfg->current_bb->add_IRInstr(IRInstr::add, INT, {dest, leftVar, rightVar});
        else
            current_cfg->current_bb->add_IRInstr(IRInstr::sub, INT, {dest, leftVar, rightVar});
    }

    ExprValue result;
    result.isConstant = false;
    result.varName = dest;
    result.type = resultType;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitUnaryMinusExpr(ifccParser::UnaryMinusExprContext *ctx) {
    ExprValue src = castAny<ExprValue>(this->visit(ctx->expr()));

    if (src.isConstant) {
        ExprValue result;
        result.isConstant = true;
        result.type = src.type;
        if (src.type == DOUBLE) {
            result.dvalue = -src.dvalue;
            result.value = 0;
        } else {
            result.value = -src.value;
            result.dvalue = 0.0;
        }
        return result;
    }

    if (src.type == DOUBLE) {
        string zero = loadConstDouble(0.0);
        string srcVar = src.varName;
        string dest = current_cfg->create_new_tempvar(DOUBLE);
        current_cfg->current_bb->add_IRInstr(IRInstr::sub_double, DOUBLE, {dest, zero, srcVar});
        ExprValue result;
        result.isConstant = false;
        result.varName = dest;
        result.type = DOUBLE;
        result.value = 0;
        result.dvalue = 0.0;
        return result;
    } else {
        string zero = loadConst(0);
        string srcVar = src.varName;
        string dest = current_cfg->create_new_tempvar(INT);
        current_cfg->current_bb->add_IRInstr(IRInstr::sub, INT, {dest, zero, srcVar});
        ExprValue result;
        result.isConstant = false;
        result.varName = dest;
        result.type = INT;
        result.value = 0;
        result.dvalue = 0.0;
        return result;
    }
}

antlrcpp::Any IRGenVisitor::visitLogicalNotExpr(ifccParser::LogicalNotExprContext *ctx) {
    ExprValue src = castAny<ExprValue>(this->visit(ctx->expr()));

    // Constant folding
    if (src.isConstant) {
        ExprValue result;
        result.isConstant = true;
        result.type = INT;
        result.dvalue = 0.0;
        if (src.type == DOUBLE) {
            result.value = (src.dvalue == 0.0) ? 1 : 0;
        } else {
            result.value = (src.value == 0) ? 1 : 0;
        }
        return result;
    }

    // Si c'est un double, convertir en int d'abord
    if (src.type == DOUBLE) {
        string intVar = current_cfg->create_new_tempvar(INT);
        current_cfg->current_bb->add_IRInstr(IRInstr::double_to_int, INT, {intVar, src.varName});
        src.varName = intVar;
        src.type = INT;
    }

    string dest = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::logical_not, INT, {dest, src.varName});

    ExprValue result;
    result.isConstant = false;
    result.varName = dest;
    result.type = INT;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitBitAndExpr(ifccParser::BitAndExprContext *ctx) {
    ExprValue left = castAny<ExprValue>(this->visit(ctx->expr(0)));
    ExprValue right = castAny<ExprValue>(this->visit(ctx->expr(1)));

    // Constant folding (int only)
    if (left.isConstant && right.isConstant && left.type == INT && right.type == INT) {
        ExprValue result;
        result.isConstant = true;
        result.type = INT;
        result.value = left.value & right.value;
        result.dvalue = 0.0;
        return result;
    }

    string leftVar = materialize(left);
    string rightVar = materialize(right);

    string dest = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::bit_and, INT, {dest, leftVar, rightVar});

    ExprValue result;
    result.isConstant = false;
    result.varName = dest;
    result.type = INT;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitBitXorExpr(ifccParser::BitXorExprContext *ctx) {
    ExprValue left = castAny<ExprValue>(this->visit(ctx->expr(0)));
    ExprValue right = castAny<ExprValue>(this->visit(ctx->expr(1)));

    if (left.isConstant && right.isConstant && left.type == INT && right.type == INT) {
        ExprValue result;
        result.isConstant = true;
        result.type = INT;
        result.value = left.value ^ right.value;
        result.dvalue = 0.0;
        return result;
    }

    string leftVar = materialize(left);
    string rightVar = materialize(right);

    string dest = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::bit_xor, INT, {dest, leftVar, rightVar});

    ExprValue result;
    result.isConstant = false;
    result.varName = dest;
    result.type = INT;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitBitOrExpr(ifccParser::BitOrExprContext *ctx) {
    ExprValue left = castAny<ExprValue>(this->visit(ctx->expr(0)));
    ExprValue right = castAny<ExprValue>(this->visit(ctx->expr(1)));

    if (left.isConstant && right.isConstant && left.type == INT && right.type == INT) {
        ExprValue result;
        result.isConstant = true;
        result.type = INT;
        result.value = left.value | right.value;
        result.dvalue = 0.0;
        return result;
    }

    string leftVar = materialize(left);
    string rightVar = materialize(right);

    string dest = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::bit_or, INT, {dest, leftVar, rightVar});

    ExprValue result;
    result.isConstant = false;
    result.varName = dest;
    result.type = INT;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitParenExpr(ifccParser::ParenExprContext *ctx) {
    return this->visit(ctx->expr());
}

antlrcpp::Any IRGenVisitor::visitCallExpr(ifccParser::CallExprContext *ctx) {
    string funcName = ctx->VAR()->getText();
    vector<string> args;
    
    for (auto exprCtx : ctx->expr()) {
        ExprValue argVal = castAny<ExprValue>(this->visit(exprCtx));
        args.push_back(materialize(argVal));
    }
    
    string dest = current_cfg->create_new_tempvar(INT);
    
    vector<string> params = {dest, funcName};
    params.insert(params.end(), args.begin(), args.end());
    
    current_cfg->current_bb->add_IRInstr(IRInstr::call, INT, params);
    
    ExprValue result;
    result.isConstant = false;
    result.varName = dest;
    result.type = INT;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitBlock(ifccParser::BlockContext *ctx) {
    pushScope();
    for (auto stmt : ctx->statement()) {
        this->visit(stmt);
    }
    popScope();
    return 0;
}

antlrcpp::Any IRGenVisitor::visitIfStmt(ifccParser::IfStmtContext *ctx) {
    set<string> modifiedVars;
    for (auto stmt : ctx->statement()) {
        set<string> vars = collectAssignedVars(stmt);
        modifiedVars.insert(vars.begin(), vars.end());
    }

    ExprValue cond = castAny<ExprValue>(this->visit(ctx->expr()));
    string condVar = cond.isConstant ? loadConst(cond.value) : cond.varName;
    
    if (cond.type == DOUBLE && !cond.isConstant) {
        string intVar = current_cfg->create_new_tempvar(INT);
        current_cfg->current_bb->add_IRInstr(IRInstr::double_to_int, INT, {intVar, condVar});
        condVar = intVar;
    }
    
    current_cfg->current_bb->test_var_name = condVar;

    BasicBlock* bb_true = new BasicBlock(current_cfg, current_cfg->new_BB_name());
    BasicBlock* bb_end = new BasicBlock(current_cfg, current_cfg->new_BB_name());
    BasicBlock* bb_false = nullptr;

    bool hasElse = ctx->statement().size() > 1;
    if (hasElse) {
        bb_false = new BasicBlock(current_cfg, current_cfg->new_BB_name());
        current_cfg->current_bb->exit_false = bb_false;
    } else {
        current_cfg->current_bb->exit_false = bb_end;
    }

    current_cfg->current_bb->exit_true = bb_true;

    map<string, int> savedConstMap = constMap;

    current_cfg->add_bb(bb_true);
    constMap = savedConstMap;
    this->visit(ctx->statement(0));
    if (!current_cfg->current_bb->has_return) {
        current_cfg->current_bb->exit_false = nullptr;
        current_cfg->current_bb->exit_true = bb_end;
    }

    if (hasElse) {
        current_cfg->add_bb(bb_false);
        constMap = savedConstMap;
        this->visit(ctx->statement(1));
        if (!current_cfg->current_bb->has_return) {
            current_cfg->current_bb->exit_false = nullptr;
            current_cfg->current_bb->exit_true = bb_end;
        }
    }

    current_cfg->add_bb(bb_end);

    constMap = savedConstMap;
    for (const string& var : modifiedVars) {
        constMap.erase(var);
    }

    return 0;
}

antlrcpp::Any IRGenVisitor::visitEqExpr(ifccParser::EqExprContext *ctx) {
    ExprValue left = castAny<ExprValue>(this->visit(ctx->expr(0)));
    ExprValue right = castAny<ExprValue>(this->visit(ctx->expr(1)));
    string op = ctx->children[1]->getText();

    if (left.isConstant && right.isConstant && left.type == INT && right.type == INT) {
        ExprValue result;
        result.isConstant = true;
        result.type = INT;
        result.dvalue = 0.0;
        if (op == "==") result.value = (left.value == right.value) ? 1 : 0;
        else result.value = (left.value != right.value) ? 1 : 0;
        return result;
    }

    Type commonType = promoteType(left.type, right.type);
    string leftVar = emitConversion(left, commonType);
    string rightVar = emitConversion(right, commonType);

    if (commonType == DOUBLE) {
        string leftInt = current_cfg->create_new_tempvar(INT);
        current_cfg->current_bb->add_IRInstr(IRInstr::double_to_int, INT, {leftInt, leftVar});
        string rightInt = current_cfg->create_new_tempvar(INT);
        current_cfg->current_bb->add_IRInstr(IRInstr::double_to_int, INT, {rightInt, rightVar});
        leftVar = leftInt;
        rightVar = rightInt;
    }

    string dest = current_cfg->create_new_tempvar(INT);
    if (op == "==") {
        current_cfg->current_bb->add_IRInstr(IRInstr::cmp_eq, INT, {dest, leftVar, rightVar});
    } else {
        current_cfg->current_bb->add_IRInstr(IRInstr::cmp_neq, INT, {dest, leftVar, rightVar});
    }

    ExprValue result;
    result.isConstant = false;
    result.varName = dest;
    result.type = INT;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitRelExpr(ifccParser::RelExprContext *ctx) {
    ExprValue left = castAny<ExprValue>(this->visit(ctx->expr(0)));
    ExprValue right = castAny<ExprValue>(this->visit(ctx->expr(1)));
    string op = ctx->children[1]->getText();

    if (left.isConstant && right.isConstant && left.type == INT && right.type == INT) {
        ExprValue result;
        result.isConstant = true;
        result.type = INT;
        result.dvalue = 0.0;
        if (op == "<") result.value = (left.value < right.value) ? 1 : 0;
        else if (op == "<=") result.value = (left.value <= right.value) ? 1 : 0;
        else if (op == ">") result.value = (left.value > right.value) ? 1 : 0;
        else if (op == ">=") result.value = (left.value >= right.value) ? 1 : 0;
        return result;
    }

    Type commonType = promoteType(left.type, right.type);
    string leftVar = emitConversion(left, commonType);
    string rightVar = emitConversion(right, commonType);

    if (commonType == DOUBLE) {
        string leftInt = current_cfg->create_new_tempvar(INT);
        current_cfg->current_bb->add_IRInstr(IRInstr::double_to_int, INT, {leftInt, leftVar});
        string rightInt = current_cfg->create_new_tempvar(INT);
        current_cfg->current_bb->add_IRInstr(IRInstr::double_to_int, INT, {rightInt, rightVar});
        leftVar = leftInt;
        rightVar = rightInt;
    }

    string dest = current_cfg->create_new_tempvar(INT);
    if (op == "<") {
        current_cfg->current_bb->add_IRInstr(IRInstr::cmp_lt, INT, {dest, leftVar, rightVar});
    } else if (op == "<=") {
        current_cfg->current_bb->add_IRInstr(IRInstr::cmp_le, INT, {dest, leftVar, rightVar});
    } else if (op == ">") {
        current_cfg->current_bb->add_IRInstr(IRInstr::cmp_gt, INT, {dest, leftVar, rightVar});
    } else if (op == ">=") {
        current_cfg->current_bb->add_IRInstr(IRInstr::cmp_ge, INT, {dest, leftVar, rightVar});
    }

    ExprValue result;
    result.isConstant = false;
    result.varName = dest;
    result.type = INT;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitWhileStmt(ifccParser::WhileStmtContext *ctx) {
    set<string> modifiedVars = collectAssignedVars(ctx->statement());

    for (const string& var : modifiedVars) {
        constMap.erase(var);
    }

    BasicBlock* bb_cond = new BasicBlock(current_cfg, current_cfg->new_BB_name());
    BasicBlock* bb_body = new BasicBlock(current_cfg, current_cfg->new_BB_name());
    BasicBlock* bb_end = new BasicBlock(current_cfg, current_cfg->new_BB_name());

    current_cfg->current_bb->exit_true = bb_cond;
    current_cfg->current_bb->exit_false = nullptr;

    current_cfg->add_bb(bb_cond);
    ExprValue cond = castAny<ExprValue>(this->visit(ctx->expr()));
    string condVar = cond.isConstant ? loadConst(cond.value) : cond.varName;
    current_cfg->current_bb->test_var_name = condVar;
    current_cfg->current_bb->exit_true = bb_body;
    current_cfg->current_bb->exit_false = bb_end;

    loopStack.push_back({bb_cond, bb_end});
    current_cfg->add_bb(bb_body);
    this->visit(ctx->statement());
    loopStack.pop_back();
    if (!current_cfg->current_bb->has_return) {
        current_cfg->current_bb->exit_true = bb_cond;
        current_cfg->current_bb->exit_false = nullptr;
    }

    current_cfg->add_bb(bb_end);

    return 0;
}

antlrcpp::Any IRGenVisitor::visitForStmt(ifccParser::ForStmtContext *ctx) {
    pushScope();

    // Déterminer quels expr() correspondent à init, cond, update
    bool hasDecl = ctx->declaration() != nullptr;
    int numExprs = ctx->expr().size();
    int condIdx = -1, updateIdx = -1, initExprIdx = -1;

    if (hasDecl) {
        this->visit(ctx->declaration());
        if (numExprs >= 1) condIdx = 0;
        if (numExprs >= 2) updateIdx = 1;
    } else {
        if (numExprs == 3) { initExprIdx = 0; condIdx = 1; updateIdx = 2; }
        else if (numExprs == 2) { condIdx = 0; updateIdx = 1; }
        else if (numExprs == 1) { condIdx = 0; }
    }

    // Exécuter init expr si présent
    if (initExprIdx >= 0) {
        this->visit(ctx->expr(initExprIdx));
    }

    // Invalider les constantes APRÈS l'init (les noms scopés existent maintenant)
    set<string> modifiedVars = collectAssignedVars(ctx->statement());
    for (auto expr : ctx->expr()) {
        set<string> exprVars = collectAssignedVars(expr);
        modifiedVars.insert(exprVars.begin(), exprVars.end());
    }
    if (hasDecl) {
        // La variable d'itération elle-même est modifiée par l'update
        if (auto* dv = dynamic_cast<ifccParser::DeclVarContext*>(ctx->declaration())) {
            modifiedVars.insert(getScopedName(dv->VAR()->getText()));
        }
    }
    for (const string& var : modifiedVars) {
        constMap.erase(var);
    }

    BasicBlock* bb_cond = new BasicBlock(current_cfg, current_cfg->new_BB_name());
    BasicBlock* bb_body = new BasicBlock(current_cfg, current_cfg->new_BB_name());
    BasicBlock* bb_update = new BasicBlock(current_cfg, current_cfg->new_BB_name());
    BasicBlock* bb_end = new BasicBlock(current_cfg, current_cfg->new_BB_name());

    current_cfg->current_bb->exit_true = bb_cond;
    current_cfg->current_bb->exit_false = nullptr;

    // Bloc condition
    current_cfg->add_bb(bb_cond);
    if (condIdx >= 0) {
        ExprValue cond = castAny<ExprValue>(this->visit(ctx->expr(condIdx)));
        string condVar = cond.isConstant ? loadConst(cond.value) : cond.varName;
        current_cfg->current_bb->test_var_name = condVar;
    } else {
        string alwaysTrue = loadConst(1);
        current_cfg->current_bb->test_var_name = alwaysTrue;
    }
    current_cfg->current_bb->exit_true = bb_body;
    current_cfg->current_bb->exit_false = bb_end;

    // continue va vers bb_update
    loopStack.push_back({bb_update, bb_end});
    current_cfg->add_bb(bb_body);
    this->visit(ctx->statement());
    if (!current_cfg->current_bb->has_return) {
        current_cfg->current_bb->exit_true = bb_update;
        current_cfg->current_bb->exit_false = nullptr;
    }
    loopStack.pop_back();

    // Bloc update
    current_cfg->add_bb(bb_update);
    if (updateIdx >= 0) {
        this->visit(ctx->expr(updateIdx));
    }
    current_cfg->current_bb->exit_true = bb_cond;
    current_cfg->current_bb->exit_false = nullptr;

    current_cfg->add_bb(bb_end);
    popScope();
    return 0;
}

antlrcpp::Any IRGenVisitor::visitBreakStmt(ifccParser::BreakStmtContext *ctx) {
    if (!loopStack.empty()) {
        current_cfg->current_bb->exit_true = loopStack.back().bb_end;
        current_cfg->current_bb->exit_false = nullptr;
        current_cfg->current_bb->has_return = true;
        BasicBlock* dead_bb = new BasicBlock(current_cfg, current_cfg->new_BB_name());
        current_cfg->add_bb(dead_bb);
    }
    return 0;
}

antlrcpp::Any IRGenVisitor::visitContinueStmt(ifccParser::ContinueStmtContext *ctx) {
    if (!loopStack.empty()) {
        current_cfg->current_bb->exit_true = loopStack.back().bb_cond;
        current_cfg->current_bb->exit_false = nullptr;
        current_cfg->current_bb->has_return = true;
        BasicBlock* dead_bb = new BasicBlock(current_cfg, current_cfg->new_BB_name());
        current_cfg->add_bb(dead_bb);
    }
    return 0;
}

antlrcpp::Any IRGenVisitor::visitShiftExpr(ifccParser::ShiftExprContext *ctx) {
    ExprValue left = castAny<ExprValue>(this->visit(ctx->expr(0)));
    ExprValue right = castAny<ExprValue>(this->visit(ctx->expr(1)));
    string op = ctx->children[1]->getText();

    if (left.isConstant && right.isConstant && left.type == INT && right.type == INT) {
        ExprValue result;
        result.isConstant = true;
        result.type = INT;
        result.dvalue = 0.0;
        if (op == "<<") result.value = left.value << right.value;
        else result.value = left.value >> right.value;
        return result;
    }

    string leftVar = materialize(left);
    string rightVar = materialize(right);
    string dest = current_cfg->create_new_tempvar(INT);
    if (op == "<<") {
        current_cfg->current_bb->add_IRInstr(IRInstr::shl, INT, {dest, leftVar, rightVar});
    } else {
        current_cfg->current_bb->add_IRInstr(IRInstr::shr, INT, {dest, leftVar, rightVar});
    }

    ExprValue result;
    result.isConstant = false;
    result.varName = dest;
    result.type = INT;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitLogicalAndExpr(ifccParser::LogicalAndExprContext *ctx) {
    ExprValue left = castAny<ExprValue>(this->visit(ctx->expr(0)));

    if (left.isConstant && left.type == INT && left.value == 0) {
        ExprValue result;
        result.isConstant = true;
        result.type = INT;
        result.value = 0;
        result.dvalue = 0.0;
        return result;
    }

    string resultVar = current_cfg->create_new_tempvar(INT);
    string leftVar = materialize(left);
    current_cfg->current_bb->add_IRInstr(IRInstr::ldconst, INT, {resultVar, "0"});
    current_cfg->current_bb->test_var_name = leftVar;

    BasicBlock* bb_right = new BasicBlock(current_cfg, current_cfg->new_BB_name());
    BasicBlock* bb_end = new BasicBlock(current_cfg, current_cfg->new_BB_name());

    current_cfg->current_bb->exit_true = bb_right;
    current_cfg->current_bb->exit_false = bb_end;

    current_cfg->add_bb(bb_right);
    ExprValue right = castAny<ExprValue>(this->visit(ctx->expr(1)));
    string rightVar = materialize(right);
    string zero = loadConst(0);
    current_cfg->current_bb->add_IRInstr(IRInstr::cmp_neq, INT, {resultVar, rightVar, zero});
    current_cfg->current_bb->exit_true = bb_end;
    current_cfg->current_bb->exit_false = nullptr;

    current_cfg->add_bb(bb_end);

    ExprValue result;
    result.isConstant = false;
    result.varName = resultVar;
    result.type = INT;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitLogicalOrExpr(ifccParser::LogicalOrExprContext *ctx) {
    ExprValue left = castAny<ExprValue>(this->visit(ctx->expr(0)));

    if (left.isConstant && left.type == INT && left.value != 0) {
        ExprValue result;
        result.isConstant = true;
        result.type = INT;
        result.value = 1;
        result.dvalue = 0.0;
        return result;
    }

    string resultVar = current_cfg->create_new_tempvar(INT);
    string leftVar = materialize(left);
    current_cfg->current_bb->add_IRInstr(IRInstr::ldconst, INT, {resultVar, "1"});
    current_cfg->current_bb->test_var_name = leftVar;

    BasicBlock* bb_right = new BasicBlock(current_cfg, current_cfg->new_BB_name());
    BasicBlock* bb_end = new BasicBlock(current_cfg, current_cfg->new_BB_name());

    current_cfg->current_bb->exit_true = bb_end;
    current_cfg->current_bb->exit_false = bb_right;

    current_cfg->add_bb(bb_right);
    ExprValue right = castAny<ExprValue>(this->visit(ctx->expr(1)));
    string rightVar = materialize(right);
    string zero = loadConst(0);
    current_cfg->current_bb->add_IRInstr(IRInstr::cmp_neq, INT, {resultVar, rightVar, zero});
    current_cfg->current_bb->exit_true = bb_end;
    current_cfg->current_bb->exit_false = nullptr;

    current_cfg->add_bb(bb_end);

    ExprValue result;
    result.isConstant = false;
    result.varName = resultVar;
    result.type = INT;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitPreIncExpr(ifccParser::PreIncExprContext *ctx) {
    string varName = getScopedName(ctx->VAR()->getText());
    string addrTemp = current_cfg->create_new_tempvar(ADDR);
    current_cfg->current_bb->add_IRInstr(IRInstr::lea, ADDR, {addrTemp, varName});
    string oldVal = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::rmem, INT, {oldVal, addrTemp});
    string one = loadConst(1);
    string newVal = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::add, INT, {newVal, oldVal, one});
    current_cfg->current_bb->add_IRInstr(IRInstr::wmem, INT, {addrTemp, newVal});
    constMap.erase(varName);
    ExprValue result;
    result.isConstant = false;
    result.varName = newVal;
    result.type = INT;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitPreDecExpr(ifccParser::PreDecExprContext *ctx) {
    string varName = getScopedName(ctx->VAR()->getText());
    string addrTemp = current_cfg->create_new_tempvar(ADDR);
    current_cfg->current_bb->add_IRInstr(IRInstr::lea, ADDR, {addrTemp, varName});
    string oldVal = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::rmem, INT, {oldVal, addrTemp});
    string one = loadConst(1);
    string newVal = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::sub, INT, {newVal, oldVal, one});
    current_cfg->current_bb->add_IRInstr(IRInstr::wmem, INT, {addrTemp, newVal});
    constMap.erase(varName);
    ExprValue result;
    result.isConstant = false;
    result.varName = newVal;
    result.type = INT;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitPostIncStmt(ifccParser::PostIncStmtContext *ctx) {
    string varName = getScopedName(ctx->VAR()->getText());
    string addrTemp = current_cfg->create_new_tempvar(ADDR);
    current_cfg->current_bb->add_IRInstr(IRInstr::lea, ADDR, {addrTemp, varName});
    string oldVal = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::rmem, INT, {oldVal, addrTemp});
    string one = loadConst(1);
    string newVal = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::add, INT, {newVal, oldVal, one});
    current_cfg->current_bb->add_IRInstr(IRInstr::wmem, INT, {addrTemp, newVal});
    constMap.erase(varName);
    return 0;
}

antlrcpp::Any IRGenVisitor::visitPostDecStmt(ifccParser::PostDecStmtContext *ctx) {
    string varName = getScopedName(ctx->VAR()->getText());
    string addrTemp = current_cfg->create_new_tempvar(ADDR);
    current_cfg->current_bb->add_IRInstr(IRInstr::lea, ADDR, {addrTemp, varName});
    string oldVal = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::rmem, INT, {oldVal, addrTemp});
    string one = loadConst(1);
    string newVal = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::sub, INT, {newVal, oldVal, one});
    current_cfg->current_bb->add_IRInstr(IRInstr::wmem, INT, {addrTemp, newVal});
    constMap.erase(varName);
    return 0;
}

ExprValue IRGenVisitor::emitCompoundAssign(ifccParser::LvalueContext* lvalueCtx, ifccParser::ExprContext* exprCtx, string op) {
    ExprValue exprResult = castAny<ExprValue>(this->visit(exprCtx));
    LvalueResult lv = castAny<LvalueResult>(this->visit(lvalueCtx));
    Type varType = lv.type;

    string currentVal = current_cfg->create_new_tempvar(varType);
    if (varType == DOUBLE) {
        current_cfg->current_bb->add_IRInstr(IRInstr::rmem_double, DOUBLE, {currentVal, lv.addrVar});
    } else {
        current_cfg->current_bb->add_IRInstr(IRInstr::rmem, INT, {currentVal, lv.addrVar});
    }

    if (exprResult.type != varType) emitConversion(exprResult, varType);
    string rhsVar = materialize(exprResult);
    string dest = current_cfg->create_new_tempvar(varType);

    if (op == "+") current_cfg->current_bb->add_IRInstr(varType == DOUBLE ? IRInstr::add_double : IRInstr::add, varType, {dest, currentVal, rhsVar});
    else if (op == "-") current_cfg->current_bb->add_IRInstr(varType == DOUBLE ? IRInstr::sub_double : IRInstr::sub, varType, {dest, currentVal, rhsVar});
    else if (op == "*") current_cfg->current_bb->add_IRInstr(varType == DOUBLE ? IRInstr::mul_double : IRInstr::mul, varType, {dest, currentVal, rhsVar});
    else if (op == "/") current_cfg->current_bb->add_IRInstr(varType == DOUBLE ? IRInstr::div_double : IRInstr::div_int, varType, {dest, currentVal, rhsVar});
    else if (op == "%") current_cfg->current_bb->add_IRInstr(IRInstr::mod_int, INT, {dest, currentVal, rhsVar});

    if (varType == DOUBLE) current_cfg->current_bb->add_IRInstr(IRInstr::wmem_double, DOUBLE, {lv.addrVar, dest});
    else current_cfg->current_bb->add_IRInstr(IRInstr::wmem, INT, {lv.addrVar, dest});

    if (auto* lvVar = dynamic_cast<ifccParser::LvalueVarContext*>(lvalueCtx)) {
        constMap.erase(getScopedName(lvVar->VAR()->getText()));
    }

    ExprValue result;
    result.isConstant = false;
    result.varName = dest;
    result.type = varType;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitPlusAssignExpr(ifccParser::PlusAssignExprContext *ctx) {
    return emitCompoundAssign(ctx->lvalue(), ctx->expr(), "+");
}
antlrcpp::Any IRGenVisitor::visitMinusAssignExpr(ifccParser::MinusAssignExprContext *ctx) {
    return emitCompoundAssign(ctx->lvalue(), ctx->expr(), "-");
}
antlrcpp::Any IRGenVisitor::visitMulAssignExpr(ifccParser::MulAssignExprContext *ctx) {
    return emitCompoundAssign(ctx->lvalue(), ctx->expr(), "*");
}
antlrcpp::Any IRGenVisitor::visitDivAssignExpr(ifccParser::DivAssignExprContext *ctx) {
    return emitCompoundAssign(ctx->lvalue(), ctx->expr(), "/");
}
antlrcpp::Any IRGenVisitor::visitModAssignExpr(ifccParser::ModAssignExprContext *ctx) {
    return emitCompoundAssign(ctx->lvalue(), ctx->expr(), "%");
}

antlrcpp::Any IRGenVisitor::visitReturn_void_stmt(ifccParser::Return_void_stmtContext *ctx) {
    // Pour les fonctions void : pas d'expression à évaluer
    current_cfg->current_bb->exit_true = exit_bb;
    current_cfg->current_bb->exit_false = nullptr;
    current_cfg->current_bb->has_return = true;

    BasicBlock* dead_bb = new BasicBlock(current_cfg, current_cfg->new_BB_name());
    current_cfg->add_bb(dead_bb);

    return 0;
}

antlrcpp::Any IRGenVisitor::visitDoWhileStmt(ifccParser::DoWhileStmtContext *ctx) {
    set<string> modifiedVars = collectAssignedVars(ctx->statement());
    for (const string& var : modifiedVars) {
        constMap.erase(var);
    }

    BasicBlock* bb_body = new BasicBlock(current_cfg, current_cfg->new_BB_name());
    BasicBlock* bb_cond = new BasicBlock(current_cfg, current_cfg->new_BB_name());
    BasicBlock* bb_end = new BasicBlock(current_cfg, current_cfg->new_BB_name());

    // Saut direct vers le corps (pas de test initial)
    current_cfg->current_bb->exit_true = bb_body;
    current_cfg->current_bb->exit_false = nullptr;

    // Corps de la boucle
    loopStack.push_back({bb_cond, bb_end});
    current_cfg->add_bb(bb_body);
    this->visit(ctx->statement());
    if (!current_cfg->current_bb->has_return) {
        current_cfg->current_bb->exit_true = bb_cond;
        current_cfg->current_bb->exit_false = nullptr;
    }
    loopStack.pop_back();

    // Condition (après le corps)
    current_cfg->add_bb(bb_cond);
    ExprValue cond = castAny<ExprValue>(this->visit(ctx->expr()));
    string condVar = cond.isConstant ? loadConst(cond.value) : cond.varName;
    current_cfg->current_bb->test_var_name = condVar;
    current_cfg->current_bb->exit_true = bb_body;   // si vrai, retour au corps
    current_cfg->current_bb->exit_false = bb_end;    // si faux, sortie

    current_cfg->add_bb(bb_end);
    return 0;
}

antlrcpp::Any IRGenVisitor::visitTernaryExpr(ifccParser::TernaryExprContext *ctx) {
    // Évaluer la condition
    ExprValue cond = castAny<ExprValue>(this->visit(ctx->expr(0)));

    // Constant folding : si la condition est constante, on n'évalue qu'une branche
    if (cond.isConstant && cond.type == INT) {
        if (cond.value != 0) {
            return this->visit(ctx->expr(1));  // branche vraie
        } else {
            return this->visit(ctx->expr(2));  // branche fausse
        }
    }

    string condVar = materialize(cond);

    // Variable résultat
    string resultVar = current_cfg->create_new_tempvar(INT);

    current_cfg->current_bb->test_var_name = condVar;

    BasicBlock* bb_true = new BasicBlock(current_cfg, current_cfg->new_BB_name());
    BasicBlock* bb_false = new BasicBlock(current_cfg, current_cfg->new_BB_name());
    BasicBlock* bb_end = new BasicBlock(current_cfg, current_cfg->new_BB_name());

    current_cfg->current_bb->exit_true = bb_true;
    current_cfg->current_bb->exit_false = bb_false;

    // Branche vraie
    current_cfg->add_bb(bb_true);
    ExprValue trueVal = castAny<ExprValue>(this->visit(ctx->expr(1)));
    string trueVar = materialize(trueVal);
    current_cfg->current_bb->add_IRInstr(IRInstr::copy, INT, {resultVar, trueVar});
    current_cfg->current_bb->exit_true = bb_end;
    current_cfg->current_bb->exit_false = nullptr;

    // Branche fausse
    current_cfg->add_bb(bb_false);
    ExprValue falseVal = castAny<ExprValue>(this->visit(ctx->expr(2)));
    string falseVar = materialize(falseVal);
    current_cfg->current_bb->add_IRInstr(IRInstr::copy, INT, {resultVar, falseVar});
    current_cfg->current_bb->exit_true = bb_end;
    current_cfg->current_bb->exit_false = nullptr;

    current_cfg->add_bb(bb_end);

    ExprValue result;
    result.isConstant = false;
    result.varName = resultVar;
    result.type = INT;
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}
