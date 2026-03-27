#pragma once

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "IR.h"
#include <map>
#include <set>

using namespace std;

struct ExprValue {
    bool isConstant;
    int value;           // Valeur constante int (si isConstant && type == INT)
    double dvalue;       // Valeur constante double (si isConstant && type == DOUBLE)
    string varName;      // Nom du registre/temp (si !isConstant)
    Type type;           // Type de l'expression (INT ou DOUBLE)
};

// Résultat de l'évaluation d'une lvalue : contient l'adresse mémoire de la lvalue
struct LvalueResult {
    string addrVar;      // Variable temporaire contenant l'adresse de la lvalue
    Type type;           // Type de la valeur pointée (INT ou DOUBLE)
};

class IRGenVisitor : public ifccBaseVisitor {
public:
    IRGenVisitor() : current_cfg(nullptr) {}
    
    std::vector<CFG*> getCFGs() const { return cfgs; }

    virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
    virtual antlrcpp::Any visitFunction_def(ifccParser::Function_defContext *ctx) override;
    virtual antlrcpp::Any visitDeclVar(ifccParser::DeclVarContext *ctx) override;
    virtual antlrcpp::Any visitDeclVarUninit(ifccParser::DeclVarUninitContext *ctx) override;
    virtual antlrcpp::Any visitDeclArray(ifccParser::DeclArrayContext *ctx) override;
    virtual antlrcpp::Any visitAssignExpr(ifccParser::AssignExprContext *ctx);
    virtual antlrcpp::Any visitLvalueVar(ifccParser::LvalueVarContext *ctx);
    virtual antlrcpp::Any visitLvalueArray(ifccParser::LvalueArrayContext *ctx);
    virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;
    virtual antlrcpp::Any visitConstExpr(ifccParser::ConstExprContext *ctx) override;
    virtual antlrcpp::Any visitConstDoubleExpr(ifccParser::ConstDoubleExprContext *ctx) override;
    virtual antlrcpp::Any visitCharExpr(ifccParser::CharExprContext *ctx) override;
    virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;
    virtual antlrcpp::Any visitArrayAccessExpr(ifccParser::ArrayAccessExprContext *ctx) override;
    virtual antlrcpp::Any visitMulDivModExpr(ifccParser::MulDivModExprContext *ctx) override;
    virtual antlrcpp::Any visitAddSubExpr(ifccParser::AddSubExprContext *ctx) override;
    virtual antlrcpp::Any visitUnaryMinusExpr(ifccParser::UnaryMinusExprContext *ctx) override;
    virtual antlrcpp::Any visitLogicalNotExpr(ifccParser::LogicalNotExprContext *ctx) override;
    virtual antlrcpp::Any visitBitAndExpr(ifccParser::BitAndExprContext *ctx) override;
    virtual antlrcpp::Any visitBitXorExpr(ifccParser::BitXorExprContext *ctx) override;
    virtual antlrcpp::Any visitBitOrExpr(ifccParser::BitOrExprContext *ctx) override;
    virtual antlrcpp::Any visitParenExpr(ifccParser::ParenExprContext *ctx) override;
    virtual antlrcpp::Any visitCallExpr(ifccParser::CallExprContext *ctx) override;
    virtual antlrcpp::Any visitBlock(ifccParser::BlockContext *ctx) override;
    virtual antlrcpp::Any visitIfStmt(ifccParser::IfStmtContext *ctx) override;
    virtual antlrcpp::Any visitRelExpr(ifccParser::RelExprContext *ctx) override;
    virtual antlrcpp::Any visitEqExpr(ifccParser::EqExprContext *ctx) override;
    virtual antlrcpp::Any visitWhileStmt(ifccParser::WhileStmtContext *ctx) override;

private:
    std::vector<CFG*> cfgs;
    CFG* current_cfg;
    BasicBlock* exit_bb;
    string loadConst(int value);
    string loadConstDouble(double value);

    // Parse le type depuis un noeud 'type' de la grammaire
    Type parseType(ifccParser::TypeContext* ctx);

    // Environnement pour renommer les variables (shadowing)
    vector<map<string, string>> scopeStack;
    void pushScope() { scopeStack.push_back({}); }
    void popScope() { if (!scopeStack.empty()) scopeStack.pop_back(); }
    string declareScopedVariable(string name) {
        string uniqueName = name + "_" + to_string(nextVarIndex++);
        if (!scopeStack.empty()) {
            scopeStack.back()[name] = uniqueName;
        }
        return uniqueName;
    }
    string getScopedName(string name) {
        for (int i = scopeStack.size() - 1; i >= 0; --i) {
            if (scopeStack[i].count(name)) return scopeStack[i].at(name);
        }
        return name; // fallback
    }
    int nextVarIndex = 0;

    // Émet une instruction de conversion int→double ou double→int si nécessaire.
    string emitConversion(ExprValue& val, Type targetType);

    // Matérialise un ExprValue constant en variable temporaire si besoin
    string materialize(ExprValue& val);

    // ===== Propagation des variables constantes =====
    map<string, int> constMap;
    set<string> collectAssignedVars(antlr4::tree::ParseTree* tree);

    // Compteur global pour les labels de constantes double dans la section .rodata
    int nextDoubleConstIndex = 0;
};
