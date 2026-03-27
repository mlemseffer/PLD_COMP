#pragma once
#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "type.h"
#include <map>
#include <set>
#include <string>
#include <vector>

class SymbolTableVisitor : public ifccBaseVisitor {
public:
    virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
    virtual antlrcpp::Any visitFunction_def(ifccParser::Function_defContext *ctx) override;
    virtual antlrcpp::Any visitDeclVar(ifccParser::DeclVarContext *ctx) override;
    virtual antlrcpp::Any visitDeclArray(ifccParser::DeclArrayContext *ctx) override;
    virtual antlrcpp::Any visitAssignExpr(ifccParser::AssignExprContext *ctx) override;
    virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;
    virtual antlrcpp::Any visitArrayAccessExpr(ifccParser::ArrayAccessExprContext *ctx) override;
    virtual antlrcpp::Any visitCallExpr(ifccParser::CallExprContext *ctx) override;
    virtual antlrcpp::Any visitBlock(ifccParser::BlockContext *ctx) override;

    // Scope stack: each element is a map of variable names to memory indices
    std::vector<std::map<std::string, int>> scopeStack;
    std::set<std::string> usedVariables;
    bool hasError = false;

    // Lookup a variable through the scope stack (innermost first)
    int lookupVariable(const std::string& name) const;
    bool isVariableDeclared(const std::string& name) const;

private:
    int nextFreeIndex = 4;

    // Scope management
    void pushScope();
    void popScope();
    void declareVariable(const std::string& name, int index);

    // Registre des fonctions : nom -> nombre de paramètres
    std::map<std::string, int> functionRegistry;
    std::set<std::string> calledFunctions;
    std::set<std::string> definedFunctions;

    // Parse un noeud type de la grammaire
    Type parseType(ifccParser::TypeContext* ctx) {
        std::string text = ctx->getText();
        if (text == "double") return DOUBLE;
        if (text == "void") return VOID;
        return INT;
    }
};