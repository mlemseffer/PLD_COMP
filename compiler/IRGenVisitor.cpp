#include "IRGenVisitor.h"
#include <cstring>

// ===== Collecte des variables affectées dans un sous-arbre AST =====
set<string> IRGenVisitor::collectAssignedVars(antlr4::tree::ParseTree* tree) {
    set<string> result;
    if (auto* affCtx = dynamic_cast<ifccParser::AffectationContext*>(tree)) {
        // lvalue peut être lvalueVar ou lvalueArray
        if (auto* lvVar = dynamic_cast<ifccParser::LvalueVarContext*>(affCtx->lvalue())) {
            result.insert(lvVar->VAR()->getText());
        } else if (auto* lvArr = dynamic_cast<ifccParser::LvalueArrayContext*>(affCtx->lvalue())) {
            result.insert(lvArr->VAR()->getText());
        }
    }
    if (auto* declCtx = dynamic_cast<ifccParser::DeclVarContext*>(tree)) {
        result.insert(declCtx->VAR()->getText());
    }
    if (auto* declArrCtx = dynamic_cast<ifccParser::DeclArrayContext*>(tree)) {
        result.insert(declArrCtx->VAR()->getText());
    }
    for (size_t i = 0; i < tree->children.size(); i++) {
        set<string> childVars = collectAssignedVars(tree->children[i]);
        result.insert(childVars.begin(), childVars.end());
    }
    return result;
}

// ===== Helpers =====

Type IRGenVisitor::parseType(ifccParser::TypeContext* ctx) {
    if (ctx->getText() == "double") return DOUBLE;
    return INT;
}

string IRGenVisitor::loadConst(int value) {
    string dest = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::ldconst, INT, {dest, to_string(value)});
    return dest;
}

string IRGenVisitor::loadConstDouble(double value) {
    string dest = current_cfg->create_new_tempvar(DOUBLE);
    // Créer un label dans .rodata pour cette constante
    string label = ".LCD_" + current_cfg->funcName + "_" + to_string(nextDoubleConstIndex++);
    current_cfg->doubleConstants.push_back({label, value});
    current_cfg->current_bb->add_IRInstr(IRInstr::ldconst_double, DOUBLE, {dest, label});
    return dest;
}

// Émet une conversion si le type de val ne correspond pas à targetType.
// Retourne le varName du résultat.
string IRGenVisitor::emitConversion(ExprValue& val, Type targetType) {
    if (val.type == targetType) {
        return materialize(val);
    }
    string srcVar = materialize(val);
    if (val.type == INT && targetType == DOUBLE) {
        // int → double
        string dest = current_cfg->create_new_tempvar(DOUBLE);
        current_cfg->current_bb->add_IRInstr(IRInstr::int_to_double, DOUBLE, {dest, srcVar});
        val.type = DOUBLE;
        val.isConstant = false;
        val.varName = dest;
        return dest;
    } else if (val.type == DOUBLE && targetType == INT) {
        // double → int (truncation)
        string dest = current_cfg->create_new_tempvar(INT);
        current_cfg->current_bb->add_IRInstr(IRInstr::double_to_int, INT, {dest, srcVar});
        val.type = INT;
        val.isConstant = false;
        val.varName = dest;
        return dest;
    }
    return srcVar;
}

// Matérialise un ExprValue constant en variable temporaire si besoin
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

// ===== Visiteurs =====

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

    current_cfg = new CFG(nullptr, funcName);
    current_cfg->returnType = retType;
    cfgs.push_back(current_cfg);

    exit_bb = new BasicBlock(current_cfg, ".LBB_" + funcName + "_exit");
    exit_bb->exit_true = nullptr;
    exit_bb->exit_false = nullptr;

    BasicBlock* entry = new BasicBlock(current_cfg, ".LBB_" + funcName + "_0");
    current_cfg->add_bb(entry);

    current_cfg->add_to_symbol_table("!retval", retType);

    // Charger les paramètres depuis l'ABI
    if (ctx->parameters()) {
        int paramCount = 0;
        string argRegs[] = {"!edi", "!esi", "!edx", "!ecx", "!r8d", "!r9d"};
        
        auto typeNodes = ctx->parameters()->type();
        auto varNodes = ctx->parameters()->VAR();
        
        for (size_t i = 0; i < varNodes.size(); i++) {
            string paramName = varNodes[i]->getText();
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
    return 0;
}

antlrcpp::Any IRGenVisitor::visitDeclVar(ifccParser::DeclVarContext *ctx) {
    string varName = ctx->VAR()->getText();
    Type declType = parseType(ctx->type());

    current_cfg->add_to_symbol_table(varName, declType);

    ExprValue exprResult = this->visit(ctx->expr()).as<ExprValue>();

    // Conversion implicite si nécessaire (ex: int x = 3.14 → truncation, double y = 5 → promotion)
    if (exprResult.type != declType) {
        if (exprResult.isConstant) {
            // Conversion constante à la compilation
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

antlrcpp::Any IRGenVisitor::visitDeclArray(ifccParser::DeclArrayContext *ctx) {
    string varName = ctx->VAR()->getText();
    Type elemType = parseType(ctx->type());
    int arraySize = stoi(ctx->CONST()->getText());

    current_cfg->add_array_to_symbol_table(varName, elemType, arraySize);
    // Pas d'initialisation pour les tableaux (ils ne sont pas initialisés)
    return 0;
}

antlrcpp::Any IRGenVisitor::visitAffectation(ifccParser::AffectationContext *ctx) {
    // 1. Évaluer la rvalue (expression à droite du '=')
    ExprValue exprResult = this->visit(ctx->expr()).as<ExprValue>();

    // 2. Évaluer la lvalue (expression à gauche du '=') → obtenir l'adresse cible
    LvalueResult lv = this->visit(ctx->lvalue()).as<LvalueResult>();
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

    // 4. Sauvegarder l'info de constante avant matérialisation (pour constMap)
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

    // 7. Propagation de constantes (invalidation pour les tableaux)
    if (auto* lvVar = dynamic_cast<ifccParser::LvalueVarContext*>(ctx->lvalue())) {
        string varName = lvVar->VAR()->getText();
        if (wasConstant && varType == INT) {
            constMap[varName] = constValue;
        } else {
            constMap.erase(varName);
        }
    }
    // Pour les lvalue tableau, on ne propage pas de constante

    return 0;
}

antlrcpp::Any IRGenVisitor::visitLvalueVar(ifccParser::LvalueVarContext *ctx) {
    // Pour une lvalue simple (variable), calculer son adresse via lea
    string varName = ctx->VAR()->getText();
    Type varType = current_cfg->get_var_type(varName);

    // Créer un temporaire pour stocker l'adresse (8 octets = pointeur)
    string addrTemp = current_cfg->create_new_tempvar(ADDR);
    // lea : addrTemp = &varName
    current_cfg->current_bb->add_IRInstr(IRInstr::lea, ADDR, {addrTemp, varName});

    LvalueResult lv;
    lv.addrVar = addrTemp;
    lv.type = varType;
    return lv;
}

antlrcpp::Any IRGenVisitor::visitLvalueArray(ifccParser::LvalueArrayContext *ctx) {
    // a[i] en tant que lvalue : calculer l'adresse de l'élément a[i]
    // a[i] = Mem[&a_base + i * sizeof(elemType)]
    // Attention : notre convention SymbolIndex stocke le bord supérieur de l'allocation.
    // Pour un tableau de N éléments, l'élément 0 est à l'adresse la plus haute (= &a - 0*elemSize),
    // mais en fait la mémoire croît vers le bas sur la pile.
    // SymbolIndex[a] = bord supérieur. L'élément a[0] est à -(SymbolIndex[a])(%rbp),
    // l'élément a[1] est à -(SymbolIndex[a] - 1*elemSize)(%rbp), etc.
    // Donc a[i] est à %rbp - SymbolIndex[a] + i * elemSize
    // En adresse absolue : leaq -(SymbolIndex[a])(%rbp), %rax donne l'adresse de a[0]
    // Puis on ajoute i * elemSize pour obtenir a[i]

    string varName = ctx->VAR()->getText();
    Type elemType = current_cfg->get_array_element_type(varName);
    int elemSize = typeSize(elemType);

    // Évaluer l'expression d'index
    ExprValue indexExpr = this->visit(ctx->expr()).as<ExprValue>();

    // Calculer l'adresse de base du tableau (adresse de l'élément 0)
    string baseAddr = current_cfg->create_new_tempvar(ADDR);
    current_cfg->current_bb->add_IRInstr(IRInstr::lea, ADDR, {baseAddr, varName});

    // Calculer l'offset : index * elemSize
    // D'abord matérialiser l'index
    string indexVar = materialize(indexExpr);

    // Convertir index (int) en offset en octets : index * elemSize
    // On a besoin d'un temporaire ADDR (8 octets) pour l'arithmétique d'adresse
    // Multiplier l'index par elemSize, puis sign-extend en 64 bits
    string scaledIndex = current_cfg->create_new_tempvar(ADDR);
    // Utiliser ldconst pour charger elemSize, mul pour multiplier, puis stocker comme ADDR
    string elemSizeVar = loadConst(elemSize);
    string offsetInt = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::mul, INT, {offsetInt, indexVar, elemSizeVar});

    // Convertir l'offset int (32 bits) en adresse 64 bits via un int_to_double... non.
    // On va stocker l'offset dans un ADDR directement via une instruction spéciale.
    // En fait, utilisons add_addr qui fait dest = addr + offset (64 bits).
    // Mais l'offset est un int 32 bits. On doit le sign-extend.
    // Solution plus simple : utiliser lea pour faire l'arithmétique côté assembleur.
    // Créons plutôt un schéma simple avec les instructions existantes.

    // On va mettre l'offset (en int) dans un temp ADDR via une conversion manuelle
    // Pour simplifier, on va émettre directement l'assembleur via des instructions existantes.
    // Approche : utiliser add_addr avec l'offset en int, et le gen_asm fera movslq pour sign-extend.

    // Changeons l'approche : add_addr prend [dest, base_addr, offset_int]
    // Le gen_asm fait : movq base, %rax; movsxdl offset, %rcx; addq %rcx, %rax; movq %rax, dest
    string resultAddr = current_cfg->create_new_tempvar(ADDR);
    current_cfg->current_bb->add_IRInstr(IRInstr::add_addr, ADDR, {resultAddr, baseAddr, offsetInt});

    LvalueResult lv;
    lv.addrVar = resultAddr;
    lv.type = elemType;
    return lv;
}

antlrcpp::Any IRGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
    ExprValue exprResult = this->visit(ctx->expr()).as<ExprValue>();
    Type retType = current_cfg->returnType;

    // Conversion implicite vers le type de retour
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

antlrcpp::Any IRGenVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
    string varName = ctx->VAR()->getText();
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
    // a[i] comme rvalue : calculer l'adresse et lire la valeur
    string varName = ctx->VAR()->getText();
    Type elemType = current_cfg->get_array_element_type(varName);
    int elemSize = typeSize(elemType);

    // Évaluer l'expression d'index
    ExprValue indexExpr = this->visit(ctx->expr()).as<ExprValue>();

    // Adresse de base du tableau
    string baseAddr = current_cfg->create_new_tempvar(ADDR);
    current_cfg->current_bb->add_IRInstr(IRInstr::lea, ADDR, {baseAddr, varName});

    // Calculer l'offset : index * elemSize
    string indexVar = materialize(indexExpr);
    string elemSizeVar = loadConst(elemSize);
    string offsetInt = current_cfg->create_new_tempvar(INT);
    current_cfg->current_bb->add_IRInstr(IRInstr::mul, INT, {offsetInt, indexVar, elemSizeVar});

    // Calculer l'adresse de l'élément
    string elemAddr = current_cfg->create_new_tempvar(ADDR);
    current_cfg->current_bb->add_IRInstr(IRInstr::add_addr, ADDR, {elemAddr, baseAddr, offsetInt});

    // Lire la valeur à cette adresse
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

antlrcpp::Any IRGenVisitor::visitMulDivExpr(ifccParser::MulDivExprContext *ctx) {
    ExprValue left = this->visit(ctx->expr(0)).as<ExprValue>();
    ExprValue right = this->visit(ctx->expr(1)).as<ExprValue>();
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
            result.dvalue = (op == "*") ? lv * rv : lv / rv;
            result.value = 0;
        } else {
            if (op == "*") result.value = left.value * right.value;
            else result.value = (right.value != 0) ? left.value / right.value : 0;
            result.dvalue = 0.0;
        }
        return result;
    }

    // Élimination des éléments neutres (seulement pour int * et /)
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

    if (resultType == DOUBLE) {
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
    ExprValue left = this->visit(ctx->expr(0)).as<ExprValue>();
    ExprValue right = this->visit(ctx->expr(1)).as<ExprValue>();
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

    // Élimination des éléments neutres (seulement pour int)
    if (resultType == INT) {
        if (op == "+") {
            if (right.isConstant && right.value == 0) return left;
            if (left.isConstant && left.value == 0) return right;
        } else if (op == "-") {
            if (right.isConstant && right.value == 0) return left;
        }
    }

    // Conversion implicite si les types diffèrent
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
    ExprValue src = this->visit(ctx->expr()).as<ExprValue>();

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
        // -x double : on fait 0.0 - x
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

antlrcpp::Any IRGenVisitor::visitParenExpr(ifccParser::ParenExprContext *ctx) {
    return this->visit(ctx->expr());
}

antlrcpp::Any IRGenVisitor::visitCallExpr(ifccParser::CallExprContext *ctx) {
    string funcName = ctx->VAR()->getText();
    vector<string> args;
    
    for (auto exprCtx : ctx->expr()) {
        ExprValue argVal = this->visit(exprCtx).as<ExprValue>();
        args.push_back(materialize(argVal));
    }
    
    string dest = current_cfg->create_new_tempvar(INT);
    
    vector<string> params = {dest, funcName};
    params.insert(params.end(), args.begin(), args.end());
    
    current_cfg->current_bb->add_IRInstr(IRInstr::call, INT, params);
    
    ExprValue result;
    result.isConstant = false;
    result.varName = dest;
    result.type = INT;  // Les appels de fonction retournent INT par défaut
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitBlock(ifccParser::BlockContext *ctx) {
    for (auto stmt : ctx->statement()) {
        this->visit(stmt);
    }
    return 0;
}

antlrcpp::Any IRGenVisitor::visitIfStmt(ifccParser::IfStmtContext *ctx) {
    set<string> modifiedVars;
    for (auto stmt : ctx->statement()) {
        set<string> vars = collectAssignedVars(stmt);
        modifiedVars.insert(vars.begin(), vars.end());
    }

    ExprValue cond = this->visit(ctx->expr()).as<ExprValue>();
    string condVar = cond.isConstant ? loadConst(cond.value) : cond.varName;
    
    // Si la condition est un double, il faut la convertir en int pour le test
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
    ExprValue left = this->visit(ctx->expr(0)).as<ExprValue>();
    ExprValue right = this->visit(ctx->expr(1)).as<ExprValue>();
    string op = ctx->children[1]->getText();

    // Constant folding
    if (left.isConstant && right.isConstant && left.type == INT && right.type == INT) {
        ExprValue result;
        result.isConstant = true;
        result.type = INT;
        result.dvalue = 0.0;
        if (op == "==") result.value = (left.value == right.value) ? 1 : 0;
        else result.value = (left.value != right.value) ? 1 : 0;
        return result;
    }

    // Pour les comparaisons, on convertit au type commun d'abord
    Type commonType = promoteType(left.type, right.type);
    string leftVar = emitConversion(left, commonType);
    string rightVar = emitConversion(right, commonType);

    // Note: les comparaisons double nécessiteraient ucomisd + sete/setne
    // Pour simplifier, on garde les cmp int (les doubles étant convertis en int pour la comparaison)
    // TODO: ajouter des instructions cmp_eq_double si nécessaire
    if (commonType == DOUBLE) {
        // Convertir les deux en int pour la comparaison (approximation simple)
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
    result.type = INT;  // Le résultat d'une comparaison est toujours int
    result.value = 0;
    result.dvalue = 0.0;
    return result;
}

antlrcpp::Any IRGenVisitor::visitRelExpr(ifccParser::RelExprContext *ctx) {
    ExprValue left = this->visit(ctx->expr(0)).as<ExprValue>();
    ExprValue right = this->visit(ctx->expr(1)).as<ExprValue>();
    string op = ctx->children[1]->getText();

    // Constant folding
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
    ExprValue cond = this->visit(ctx->expr()).as<ExprValue>();
    string condVar = cond.isConstant ? loadConst(cond.value) : cond.varName;
    current_cfg->current_bb->test_var_name = condVar;
    current_cfg->current_bb->exit_true = bb_body;
    current_cfg->current_bb->exit_false = bb_end;

    current_cfg->add_bb(bb_body);
    this->visit(ctx->statement());
    if (!current_cfg->current_bb->has_return) {
        current_cfg->current_bb->exit_true = bb_cond;
        current_cfg->current_bb->exit_false = nullptr;
    }

    current_cfg->add_bb(bb_end);

    return 0;
}
