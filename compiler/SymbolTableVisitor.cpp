#include "SymbolTableVisitor.h"
#include <iostream>

// ===== Scope management =====

void SymbolTableVisitor::pushScope() {
    scopeStack.push_back({});
}

void SymbolTableVisitor::popScope() {
    if (!scopeStack.empty()) {
        scopeStack.pop_back();
    }
}

void SymbolTableVisitor::declareVariable(const std::string& name, int index) {
    if (!scopeStack.empty()) {
        scopeStack.back()[name] = index;
    }
}

bool SymbolTableVisitor::isVariableDeclared(const std::string& name) const {
    for (int i = (int)scopeStack.size() - 1; i >= 0; --i) {
        if (scopeStack[i].find(name) != scopeStack[i].end()) {
            return true;
        }
    }
    return false;
}

int SymbolTableVisitor::lookupVariable(const std::string& name) const {
    for (int i = (int)scopeStack.size() - 1; i >= 0; --i) {
        auto it = scopeStack[i].find(name);
        if (it != scopeStack[i].end()) {
            return it->second;
        }
    }
    return -1; // not found
}

// ===== Visitors =====

antlrcpp::Any SymbolTableVisitor::visitProg(ifccParser::ProgContext *ctx) {
    // Passe 1 : enregistrer toutes les fonctions définies (nom + nb params)
    for (auto func : ctx->function_def()) {
        std::string name = func->VAR()->getText();
        int paramCount = 0;
        if (func->parameters()) {
            paramCount = func->parameters()->VAR().size();
        }

        if (functionRegistry.find(name) != functionRegistry.end()) {
            std::cerr << "error: function '" << name << "' already defined" << std::endl;
            hasError = true;
        } else {
            functionRegistry[name] = paramCount;
            definedFunctions.insert(name);
        }
    }

    // Passe 2 : visiter chaque fonction (vérifications locales)
    for (auto func : ctx->function_def()) {
        visit(func);
    }

    // Passe 3 : vérifications globales post-visite
    std::set<std::string> externFunctions = {"putchar", "getchar"};
    for (auto& name : calledFunctions) {
        if (functionRegistry.find(name) == functionRegistry.end() &&
            externFunctions.find(name) == externFunctions.end()) {
            std::cerr << "error: function '" << name << "' called but never defined" << std::endl;
            hasError = true;
        }
    }

    for (auto& name : definedFunctions) {
        if (name != "main" && calledFunctions.find(name) == calledFunctions.end()) {
            std::cerr << "warning: function '" << name << "' defined but never called" << std::endl;
        }
    }

    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitFunction_def(ifccParser::Function_defContext *ctx) {
    // Réinitialiser pour chaque nouvelle fonction
    scopeStack.clear();
    usedVariables.clear();
    nextFreeIndex = 4;

    std::string funcName = ctx->VAR()->getText();

    // Push the function-level scope
    pushScope();

    // Ajouter les paramètres à la table
    if (ctx->parameters()) {
        auto typeNodes = ctx->parameters()->type();
        auto varNodes = ctx->parameters()->VAR();
        for (size_t i = 0; i < varNodes.size(); i++) {
            std::string paramName = varNodes[i]->getText();
            Type paramType = parseType(typeNodes[i]);
            if (paramType == VOID) {
                std::cerr << "error: parameter '" << paramName << "' cannot have type void" << std::endl;
                hasError = true;
                continue;
            }
            int size = typeSize(paramType);
            if (scopeStack.back().find(paramName) != scopeStack.back().end()) {
                std::cerr << "error: parameter '" << paramName << "' already declared" << std::endl;
                hasError = true;
            } else {
                if (size == 8 && (nextFreeIndex % 8 != 0)) {
                    nextFreeIndex += (8 - (nextFreeIndex % 8));
                }
                declareVariable(paramName, nextFreeIndex);
                nextFreeIndex += size;
            }
        }
    }

    // Visiter le corps de la fonction (statements, return)
    visitChildren(ctx);

    // Warnings pour variables non utilisées
    for (auto& scope : scopeStack) {
        for (auto& [name, index] : scope) {
            if (usedVariables.find(name) == usedVariables.end()) {
                std::cerr << "warning: variable '" << name << "' declared but never used in function " << funcName << std::endl;
            }
        }
    }

    std::cerr << "== Symbol Table for " << funcName << " ==" << std::endl;
    for (int i = 0; i < (int)scopeStack.size(); i++) {
        for (auto& [name, index] : scopeStack[i]) {
            std::cerr << "  [scope " << i << "] " << name << " -> -" << index << "(%rbp)" << std::endl;
        }
    }

    popScope();

    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitDeclVar(ifccParser::DeclVarContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    Type declType = parseType(ctx->type());

    // Interdire les déclarations de type void
    if (declType == VOID) {
        std::cerr << "error: variable '" << varName << "' declared void" << std::endl;
        hasError = true;
        return 0;
    }

    int size = typeSize(declType);
    // Check only the current (innermost) scope for redeclaration
    if (!scopeStack.empty() && scopeStack.back().find(varName) != scopeStack.back().end()) {
        std::cerr << "error: variable '" << varName << "' already declared in this scope" << std::endl;
        hasError = true;
        return 0;
    }
    if (size == 8 && (nextFreeIndex % 8 != 0)) {
        nextFreeIndex += (8 - (nextFreeIndex % 8));
    }
    declareVariable(varName, nextFreeIndex);
    nextFreeIndex += size;
    // Visiter l'expression à droite du '='
    visit(ctx->expr());
    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitDeclVarUninit(ifccParser::DeclVarUninitContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    Type declType = parseType(ctx->type());

    if (declType == VOID) {
        std::cerr << "error: variable '" << varName << "' declared void" << std::endl;
        hasError = true;
        return 0;
    }

    int size = typeSize(declType);
    if (!scopeStack.empty() && scopeStack.back().find(varName) != scopeStack.back().end()) {
        std::cerr << "error: variable '" << varName << "' already declared in this scope" << std::endl;
        hasError = true;
        return 0;
    }
    if (size == 8 && (nextFreeIndex % 8 != 0)) {
        nextFreeIndex += (8 - (nextFreeIndex % 8));
    }
    declareVariable(varName, nextFreeIndex);
    nextFreeIndex += size;
    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitDeclArray(ifccParser::DeclArrayContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    Type declType = parseType(ctx->type());

    if (declType == VOID) {
        std::cerr << "error: array '" << varName << "' cannot have element type void" << std::endl;
        hasError = true;
        return 0;
    }

    int arraySize = std::stoi(ctx->CONST()->getText());
    int elemSize = typeSize(declType);
    int totalSize = elemSize * arraySize;
    if (!scopeStack.empty() && scopeStack.back().find(varName) != scopeStack.back().end()) {
        std::cerr << "error: variable '" << varName << "' already declared in this scope" << std::endl;
        hasError = true;
        return 0;
    }
    if (elemSize == 8 && (nextFreeIndex % 8 != 0)) {
        nextFreeIndex += (8 - (nextFreeIndex % 8));
    }
    declareVariable(varName, nextFreeIndex);
    nextFreeIndex += totalSize;
    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitAssignExpr(ifccParser::AssignExprContext *ctx) {
    std::string varName;
    // lvalue peut être lvalueVar ou lvalueArray
    if (auto* lvVar = dynamic_cast<ifccParser::LvalueVarContext*>(ctx->lvalue())) {
        varName = lvVar->VAR()->getText();
    } else if (auto* lvArr = dynamic_cast<ifccParser::LvalueArrayContext*>(ctx->lvalue())) {
        varName = lvArr->VAR()->getText();
        visit(lvArr->expr());
    }
    if (!isVariableDeclared(varName)) {
        std::cerr << "error: variable '" << varName << "' not declared" << std::endl;
        hasError = true;
        return 0;
    }
    // Visiter l'expression à droite du '='
    visit(ctx->expr());
    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitArrayAccessExpr(ifccParser::ArrayAccessExprContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    if (!isVariableDeclared(varName)) {
        std::cerr << "error: variable '" << varName << "' not declared" << std::endl;
        hasError = true;
        return 0;
    }
    usedVariables.insert(varName);
    visit(ctx->expr());
    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    if (!isVariableDeclared(varName)) {
        std::cerr << "error: variable '" << varName << "' not declared" << std::endl;
        hasError = true;
        return 0;
    }
    usedVariables.insert(varName);
    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitCallExpr(ifccParser::CallExprContext *ctx) {
    std::string funcName = ctx->VAR()->getText();
    calledFunctions.insert(funcName);

    int numArgs = ctx->expr().size();
    if (functionRegistry.find(funcName) != functionRegistry.end()) {
        int expectedArgs = functionRegistry[funcName];
        if (numArgs != expectedArgs) {
            std::cerr << "error: function '" << funcName << "' expects "
                      << expectedArgs << " arguments, but " << numArgs << " were provided" << std::endl;
            hasError = true;
        }
    }

    for (auto exprCtx : ctx->expr()) {
        visit(exprCtx);
    }
    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitBlock(ifccParser::BlockContext *ctx) {
    pushScope();
    for (auto stmt : ctx->statement()) {
        visit(stmt);
    }
    popScope();
    return 0;
}