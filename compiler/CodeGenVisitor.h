#pragma once

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include <map>
#include <string>

class  CodeGenVisitor : public ifccBaseVisitor {
	public:
        virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
        virtual antlrcpp::Any visitFunction_def(ifccParser::Function_defContext *ctx) override;
        virtual antlrcpp::Any visitDeclVar(ifccParser::DeclVarContext *ctx) override;
        virtual antlrcpp::Any visitDeclArray(ifccParser::DeclArrayContext *ctx) override;
        virtual antlrcpp::Any visitAssignExpr(ifccParser::AssignExprContext *ctx) override;
        virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;
        virtual antlrcpp::Any visitConstExpr(ifccParser::ConstExprContext *ctx) override;
        virtual antlrcpp::Any visitCharExpr(ifccParser::CharExprContext *ctx) override;
        virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;
        virtual antlrcpp::Any visitMulDivModExpr(ifccParser::MulDivModExprContext *ctx) override;
        virtual antlrcpp::Any visitAddSubExpr(ifccParser::AddSubExprContext *ctx) override;
        virtual antlrcpp::Any visitUnaryMinusExpr(ifccParser::UnaryMinusExprContext *ctx) override;
        virtual antlrcpp::Any visitLogicalNotExpr(ifccParser::LogicalNotExprContext *ctx) override;
        virtual antlrcpp::Any visitBitAndExpr(ifccParser::BitAndExprContext *ctx) override;
        virtual antlrcpp::Any visitBitXorExpr(ifccParser::BitXorExprContext *ctx) override;
        virtual antlrcpp::Any visitBitOrExpr(ifccParser::BitOrExprContext *ctx) override;
        virtual antlrcpp::Any visitParenExpr(ifccParser::ParenExprContext *ctx) override;

        // Table des symboles reçue du SymbolTableVisitor
        std::map<std::string, int> symbolTable;
};
