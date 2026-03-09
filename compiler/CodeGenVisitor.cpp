#include "CodeGenVisitor.h"

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) 
{
    // Visiter chaque définition de fonction
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

    // Prologue : sauvegarder %rbp et réserver de l'espace sur la pile
    std::cout << "    pushq %rbp\n";
    std::cout << "    movq %rsp, %rbp\n";
    // Réserver de l'espace pour toutes les variables (arrondi à un multiple de 16 pour l'alignement)
    int stackSize = ((int)symbolTable.size() * 4 + 15) & ~15;
    std::cout << "    subq $" << stackSize << ", %rsp\n";

    // Visiter toutes les instructions (déclarations, affectations, return)
    for (auto stmt : ctx->statement()) {
        this->visit(stmt);
    }

    // Épilogue
    std::cout << "    leave\n";
    std::cout << "    ret\n";

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitDeclVar(ifccParser::DeclVarContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    int index = symbolTable[varName];

    // Évaluer l'expression (résultat dans %eax)
    this->visit(ctx->expr());

    // Stocker %eax dans la variable sur la pile
    std::cout << "    movl %eax, -" << index << "(%rbp)\n";

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitDeclArray(ifccParser::DeclArrayContext *ctx)
{
    // Déclaration de tableau sans initialisation — rien à faire côté CodeGen legacy
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAffectation(ifccParser::AffectationContext *ctx)
{
    std::string varName;
    if (auto* lvVar = dynamic_cast<ifccParser::LvalueVarContext*>(ctx->lvalue())) {
        varName = lvVar->VAR()->getText();
    }
    int index = symbolTable[varName];

    // Évaluer l'expression (résultat dans %eax)
    this->visit(ctx->expr());

    // Stocker %eax dans la variable sur la pile
    std::cout << "    movl %eax, -" << index << "(%rbp)\n";

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    // Évaluer l'expression (résultat dans %eax)
    this->visit(ctx->expr());

    // %eax contient déjà la valeur de retour
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx)
{
    int val = stoi(ctx->CONST()->getText());
    std::cout << "    movl $" << val << ", %eax\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitVarExpr(ifccParser::VarExprContext *ctx)
{
    std::string varName = ctx->VAR()->getText();
    int index = symbolTable[varName];
    std::cout << "    movl -" << index << "(%rbp), %eax\n";
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitMulDivExpr(ifccParser::MulDivExprContext *ctx)
{
    this->visit(ctx->expr(0));          // fils gauche → %eax
    std::cout << "    pushq %rax\n";    // sauver sur la pile
    this->visit(ctx->expr(1));          // fils droit → %eax
    std::cout << "    popq %rbx\n";     // récupérer fils gauche dans %ebx

    std::string op = ctx->children[1]->getText();
    if (op == "*") {
        std::cout << "    imull %ebx, %eax\n";  // %eax = %ebx * %eax
    } else {
        // division : on veut gauche / droit = %ebx / %eax
        std::cout << "    movl %eax, %ecx\n";   // diviseur dans %ecx
        std::cout << "    movl %ebx, %eax\n";   // dividende dans %eax
        std::cout << "    cltd\n";               // sign-extend %eax → %edx:%eax
        std::cout << "    idivl %ecx\n";         // %eax = quotient
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAddSubExpr(ifccParser::AddSubExprContext *ctx)
{
    this->visit(ctx->expr(0));          // fils gauche → %eax
    std::cout << "    pushq %rax\n";    // sauver sur la pile
    this->visit(ctx->expr(1));          // fils droit → %eax
    std::cout << "    popq %rbx\n";     // récupérer fils gauche dans %ebx

    std::string op = ctx->children[1]->getText();
    if (op == "+") {
        std::cout << "    addl %ebx, %eax\n";  // %eax = %ebx + %eax
    } else {
        // soustraction : on veut gauche - droit = %ebx - %eax
        std::cout << "    subl %eax, %ebx\n";  // %ebx = %ebx - %eax
        std::cout << "    movl %ebx, %eax\n";  // résultat dans %eax
    }
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitUnaryMinusExpr(ifccParser::UnaryMinusExprContext *ctx)
{
    this->visit(ctx->expr());           // évalue l'expression → %eax
    std::cout << "    negl %eax\n";     // %eax = -%eax
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitParenExpr(ifccParser::ParenExprContext *ctx)
{
    return this->visit(ctx->expr());    // juste évaluer ce qu'il y a entre parenthèses
}
