#include "SymbolTableVisitor.h"
#include <iostream>

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
    // Fonctions appelées mais jamais définies (sauf fonctions externes connues)
    std::set<std::string> externFunctions = {"putchar", "getchar"};
    for (auto& name : calledFunctions) {
        if (functionRegistry.find(name) == functionRegistry.end() &&
            externFunctions.find(name) == externFunctions.end()) {
            std::cerr << "error: function '" << name << "' called but never defined" << std::endl;
            hasError = true;
        }
    }

    // Fonctions définies mais jamais appelées (warning, sauf main)
    for (auto& name : definedFunctions) {
        if (name != "main" && calledFunctions.find(name) == calledFunctions.end()) {
            std::cerr << "warning: function '" << name << "' defined but never called" << std::endl;
        }
    }

    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitFunction_def(ifccParser::Function_defContext *ctx) {
    // Réinitialiser la table des symboles pour chaque nouvelle fonction
    symbolTable.clear();
    usedVariables.clear();
    nextFreeIndex = 4;

    std::string funcName = ctx->VAR()->getText();

    // Ajouter les paramètres à la table
    if (ctx->parameters()) {
        auto typeNodes = ctx->parameters()->type();
        auto varNodes = ctx->parameters()->VAR();
        for (size_t i = 0; i < varNodes.size(); i++) {
            std::string paramName = varNodes[i]->getText();
            Type paramType = parseType(typeNodes[i]);
            int size = typeSize(paramType);
            if (symbolTable.find(paramName) != symbolTable.end()) {
                std::cerr << "error: parameter '" << paramName << "' already declared" << std::endl;
                hasError = true;
            } else {
                // Aligner sur la taille du type
                if (size == 8 && (nextFreeIndex % 8 != 0)) {
                    nextFreeIndex += (8 - (nextFreeIndex % 8));
                }
                symbolTable[paramName] = nextFreeIndex;
                nextFreeIndex += size;
            }
        }
    }

    // Visiter le corps de la fonction (statements, return)
    visitChildren(ctx);

    for (auto& [name, index] : symbolTable) {
        if (usedVariables.find(name) == usedVariables.end()) {
            std::cerr << "warning: variable '" << name << "' declared but never used in function " << funcName << std::endl;
        }
    }
    
    std::cerr << "== Symbol Table for " << funcName << " ==" << std::endl;
    for (auto& [name, index] : symbolTable) {
        std::cerr << "  " << name << " -> -" << index << "(%rbp)" << std::endl;
    }

    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitDeclVar(ifccParser::DeclVarContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    Type declType = parseType(ctx->type());
    int size = typeSize(declType);
    if (symbolTable.find(varName) != symbolTable.end()) {
        std::cerr << "error: variable '" << varName << "' already declared" << std::endl;
        hasError = true;
        return 0;
    }
    // Aligner sur la taille du type
    if (size == 8 && (nextFreeIndex % 8 != 0)) {
        nextFreeIndex += (8 - (nextFreeIndex % 8));
    }
    symbolTable[varName] = nextFreeIndex;
    nextFreeIndex += size;
    // Visiter l'expression à droite du '=' (pour checker si c'est une variable utilisée)
    visit(ctx->expr());
    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitDeclArray(ifccParser::DeclArrayContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    Type declType = parseType(ctx->type());
    int arraySize = std::stoi(ctx->CONST()->getText());
    int elemSize = typeSize(declType);
    int totalSize = elemSize * arraySize;
    if (symbolTable.find(varName) != symbolTable.end()) {
        std::cerr << "error: variable '" << varName << "' already declared" << std::endl;
        hasError = true;
        return 0;
    }
    // Aligner sur la taille du type
    if (elemSize == 8 && (nextFreeIndex % 8 != 0)) {
        nextFreeIndex += (8 - (nextFreeIndex % 8));
    }
    symbolTable[varName] = nextFreeIndex;
    nextFreeIndex += totalSize;
    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitAffectation(ifccParser::AffectationContext *ctx) {
    std::string varName;
    // lvalue peut être lvalueVar ou lvalueArray
    if (auto* lvVar = dynamic_cast<ifccParser::LvalueVarContext*>(ctx->lvalue())) {
        varName = lvVar->VAR()->getText();
    } else if (auto* lvArr = dynamic_cast<ifccParser::LvalueArrayContext*>(ctx->lvalue())) {
        varName = lvArr->VAR()->getText();
        // Visiter l'expression d'index pour vérifier les variables utilisées
        visit(lvArr->expr());
    }
    if (symbolTable.find(varName) == symbolTable.end()) {
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
    if (symbolTable.find(varName) == symbolTable.end()) {
        std::cerr << "error: variable '" << varName << "' not declared" << std::endl;
        hasError = true;
        return 0;
    }
    usedVariables.insert(varName);
    // Visiter l'expression d'index
    visit(ctx->expr());
    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    if (symbolTable.find(varName) == symbolTable.end()) {
        std::cerr << "error: variable '" << varName << "' not declared" << std::endl;
        hasError = true;
        return 0;
    }
    // Marquer comme utilisée
    usedVariables.insert(varName);
    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitCallExpr(ifccParser::CallExprContext *ctx) {
    std::string funcName = ctx->VAR()->getText();
    calledFunctions.insert(funcName);

    // Vérifier le nombre d'arguments (si la fonction est connue)
    int numArgs = ctx->expr().size();
    if (functionRegistry.find(funcName) != functionRegistry.end()) {
        int expectedArgs = functionRegistry[funcName];
        if (numArgs != expectedArgs) {
            std::cerr << "error: function '" << funcName << "' expects " 
                      << expectedArgs << " arguments, but " << numArgs << " were provided" << std::endl;
            hasError = true;
        }
    }

    // Visiter les arguments (pour vérifier les variables utilisées)
    for (auto exprCtx : ctx->expr()) {
        visit(exprCtx);
    }
    return 0;
}