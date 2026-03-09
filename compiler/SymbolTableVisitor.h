#pragma once
#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "type.h"
#include <map>
#include <set>
#include <string>

class SymbolTableVisitor : public ifccBaseVisitor {
public:
    virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
    virtual antlrcpp::Any visitFunction_def(ifccParser::Function_defContext *ctx) override;
    virtual antlrcpp::Any visitDeclVar(ifccParser::DeclVarContext *ctx) override;
    virtual antlrcpp::Any visitDeclArray(ifccParser::DeclArrayContext *ctx) override;
    virtual antlrcpp::Any visitAffectation(ifccParser::AffectationContext *ctx) override;
    virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;
    virtual antlrcpp::Any visitArrayAccessExpr(ifccParser::ArrayAccessExprContext *ctx) override;
    virtual antlrcpp::Any visitCallExpr(ifccParser::CallExprContext *ctx) override;

    // La table des symboles : nom -> index mémoire (locale à chaque fonction)
    std::map<std::string, int> symbolTable;
    std::set<std::string> usedVariables;
    bool hasError = false;

private:
    int nextFreeIndex = 4;

    // Registre des fonctions : nom -> nombre de paramètres
    std::map<std::string, int> functionRegistry;
    // Fonctions appelées (pour détecter les appels à des fonctions non définies)
    std::set<std::string> calledFunctions;
    // Fonctions définies (pour détecter les fonctions non utilisées)
    std::set<std::string> definedFunctions;

    // Parse un noeud type de la grammaire
    Type parseType(ifccParser::TypeContext* ctx) {
        if (ctx->getText() == "double") return DOUBLE;
        return INT;
    }
};