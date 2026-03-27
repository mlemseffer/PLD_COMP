#include "CodeGenVisitor.h"

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) 
{
    for (auto func : ctx->function_def()) {
        this->visit(func);
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitFunction_def(ifccParser::Function_defContext *ctx) 
{
    std::string funcName = ctx->VAR()->getText();
    #ifdef __APPLE__
    std::cout<< ".globl _" << funcName << "\n" ;
    std::cout<< " _" << funcName << ": \n" ;
    #else
    std::cout<< ".globl " << funcName << "\n" ;
    std::cout<< " " << funcName << ": \n" ;
    #endif

    std::cout << "    pushq %rbp\n";
    std::cout << "    movq %rsp, %rbp\n";
    int stackSize = ((int)symbolTable.size() * 4 + 15) & ~15;
    std::cout << "    subq $" << stackSize << ", %rsp\n";

    for (auto stmt : ctx->statement()) {
        this->visit(stmt);
    }

    std::cout << "    leave\n";
    std::cout << "    ret\n";

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitDeclVar(ifccParser::DeclVarContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    int index = symbolTable[varName];

    this->visit(ctx->expr());

    std::cout << "    movl %eax, -" << index << "(%rbp)\n";

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitDeclVarUninit(ifccParser::DeclVarUninitContext *ctx)
{
    // No assigned value, just reserve the space (already done by symbol table size calculation).
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitDeclArray(ifccParser::DeclArrayContext *ctx)
{
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAssignExpr(ifccParser::AssignExprContext *ctx)
{
    std::string varName;
    if (auto* lvVar = dynamic_cast<ifccParser::LvalueVarContext*>(ctx->lvalue())) {
        varName = lvVar->VAR()->getText();
    }
    int index = symbolTable[varName];

    this->visit(ctx->expr());

    std::cout << "    movl %eax, -" << index << "(%rbp)\n";
    // %eax still contains the assigned value (for chaining)

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    this->visit(ctx->expr());
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx)
{
    int val = stoi(ctx->CONST()->getText());
    std::cout << "    movl $" << val << ", %eax\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitCharExpr(ifccParser::CharExprContext *ctx)
{
    std::string text = ctx->CHAR_CONST()->getText();
    std::string inner = text.substr(1, text.size() - 2);
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
    std::cout << "    movl $" << charValue << ", %eax\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitVarExpr(ifccParser::VarExprContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    int index = symbolTable[varName];
    std::cout << "    movl -" << index << "(%rbp), %eax\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitMulDivModExpr(ifccParser::MulDivModExprContext *ctx)
{
    this->visit(ctx->expr(0));
    std::cout << "    pushq %rax\n";
    this->visit(ctx->expr(1));
    std::cout << "    popq %rbx\n";

    std::string op = ctx->children[1]->getText();
    if (op == "*") {
        std::cout << "    imull %ebx, %eax\n";
    } else if (op == "/") {
        std::cout << "    movl %eax, %ecx\n";
        std::cout << "    movl %ebx, %eax\n";
        std::cout << "    cltd\n";
        std::cout << "    idivl %ecx\n";
    } else { // %
        std::cout << "    movl %eax, %ecx\n";
        std::cout << "    movl %ebx, %eax\n";
        std::cout << "    cltd\n";
        std::cout << "    idivl %ecx\n";
        std::cout << "    movl %edx, %eax\n";
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAddSubExpr(ifccParser::AddSubExprContext *ctx)
{
    this->visit(ctx->expr(0));
    std::cout << "    pushq %rax\n";
    this->visit(ctx->expr(1));
    std::cout << "    popq %rbx\n";

    std::string op = ctx->children[1]->getText();
    if (op == "+") {
        std::cout << "    addl %ebx, %eax\n";
    } else {
        std::cout << "    subl %eax, %ebx\n";
        std::cout << "    movl %ebx, %eax\n";
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitUnaryMinusExpr(ifccParser::UnaryMinusExprContext *ctx)
{
    this->visit(ctx->expr());
    std::cout << "    negl %eax\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitLogicalNotExpr(ifccParser::LogicalNotExprContext *ctx)
{
    this->visit(ctx->expr());
    std::cout << "    cmpl $0, %eax\n";
    std::cout << "    sete %al\n";
    std::cout << "    movzbl %al, %eax\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitBitAndExpr(ifccParser::BitAndExprContext *ctx)
{
    this->visit(ctx->expr(0));
    std::cout << "    pushq %rax\n";
    this->visit(ctx->expr(1));
    std::cout << "    popq %rbx\n";
    std::cout << "    andl %ebx, %eax\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitBitXorExpr(ifccParser::BitXorExprContext *ctx)
{
    this->visit(ctx->expr(0));
    std::cout << "    pushq %rax\n";
    this->visit(ctx->expr(1));
    std::cout << "    popq %rbx\n";
    std::cout << "    xorl %ebx, %eax\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitBitOrExpr(ifccParser::BitOrExprContext *ctx)
{
    this->visit(ctx->expr(0));
    std::cout << "    pushq %rax\n";
    this->visit(ctx->expr(1));
    std::cout << "    popq %rbx\n";
    std::cout << "    orl %ebx, %eax\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitParenExpr(ifccParser::ParenExprContext *ctx)
{
    return this->visit(ctx->expr());
}
