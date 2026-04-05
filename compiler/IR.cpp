#include "IR.h"
#include <map>
#include <cstring>
#include <cstdint>

using namespace std;

//  IRInstr 

IRInstr::IRInstr(BasicBlock* bb_, Operation op, Type t, vector<string> params)
    : bb(bb_), op(op), t(t), params(params) {}

void IRInstr::gen_asm(ostream &o) {
    if (bb->cfg->target == "arm64") {
        gen_asm_arm64(o);
    } else {
        gen_asm_x86(o);
    }
}

void IRInstr::gen_asm_x86(ostream &o) {
    string dest, src1, src2;
    switch(op) {
        case ldconst:
            // params = [dest, constante]
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl $" << params[1] << ", " << dest << "\n";
            break;

        case ldconst_double:
            // params = [dest, label], le label pointe vers .rodata
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movsd " << params[1] << "(%rip), %xmm0\n";
            o << "    movsd %xmm0, " << dest << "\n";
            break;

        case copy:
            // params = [dest, src]
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src1 << ", %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case copy_double:
            // params = [dest, src]
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movsd " << src1 << ", %xmm0\n";
            o << "    movsd %xmm0, " << dest << "\n";
            break;

        case add:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src1 << ", %eax\n";
            o << "    addl " << src2 << ", %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case sub:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src1 << ", %eax\n";
            o << "    subl " << src2 << ", %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case mul:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src1 << ", %eax\n";
            o << "    imull " << src2 << ", %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case div_int:
            // params = [dest, src1, src2], dest = src1 / src2
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src1 << ", %eax\n";
            o << "    cltd\n";  // sign-extend eax -> edx:eax
            o << "    idivl " << src2 << "\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case add_double:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movsd " << src1 << ", %xmm0\n";
            o << "    addsd " << src2 << ", %xmm0\n";
            o << "    movsd %xmm0, " << dest << "\n";
            break;

        case sub_double:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movsd " << src1 << ", %xmm0\n";
            o << "    subsd " << src2 << ", %xmm0\n";
            o << "    movsd %xmm0, " << dest << "\n";
            break;

        case mul_double:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movsd " << src1 << ", %xmm0\n";
            o << "    mulsd " << src2 << ", %xmm0\n";
            o << "    movsd %xmm0, " << dest << "\n";
            break;

        case div_double:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movsd " << src1 << ", %xmm0\n";
            o << "    divsd " << src2 << ", %xmm0\n";
            o << "    movsd %xmm0, " << dest << "\n";
            break;

        case int_to_double:
            // params = [dest(double), src(int)]
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    cvtsi2sdl " << src1 << ", %xmm0\n";
            o << "    movsd %xmm0, " << dest << "\n";
            break;

        case double_to_int:
            // params = [dest(int), src(double)]
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    cvttsd2si " << src1 << ", %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case cmp_eq:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src1 << ", %eax\n";
            o << "    cmpl " << src2 << ", %eax\n";
            o << "    sete %al\n";
            o << "    movzbl %al, %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case cmp_neq:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src1 << ", %eax\n";
            o << "    cmpl " << src2 << ", %eax\n";
            o << "    setne %al\n";
            o << "    movzbl %al, %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case call: {
            int numArgs = params.size() - 2;
            int extraArgs = (numArgs > 6) ? (numArgs - 6) : 0;
            int padding = (extraArgs % 2 != 0) ? 8 : 0;
            
            // Aligner rsp sur 16 octets avant d'empiler (si impair)
            if (padding > 0) {
                o << "    subq $" << padding << ", %rsp\n";
            }
            
            // Empiler les arguments supplémentaires (de droite à gauche)
            for (int i = numArgs - 1; i >= 6; --i) {
                string argMem = bb->cfg->IR_reg_to_asm(params[i + 2]);
                o << "    movslq " << argMem << ", %rax\n";
                o << "    pushq %rax\n";
            }

            string argRegs[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
            
            for (int i = 0; i < numArgs && i < 6; ++i) {
                string argMem = bb->cfg->IR_reg_to_asm(params[i + 2]);
                o << "    movl " << argMem << ", " << argRegs[i] << "\n";
            }
            
            o << "    movl $0, %eax\n";
            
            #ifdef __APPLE__
            o << "    call _" << params[1] << "\n";
            #else
            o << "    call " << params[1] << "\n";
            #endif
            
            // Nettoyer la pile après l'appel
            int totalCleanup = (extraArgs * 8) + padding;
            if (totalCleanup > 0) {
                o << "    addq $" << totalCleanup << ", %rsp\n";
            }
            
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl %eax, " << dest << "\n";
            break;
        }

        case cmp_lt:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src1 << ", %eax\n";
            o << "    cmpl " << src2 << ", %eax\n";
            o << "    setl %al\n";
            o << "    movzbl %al, %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case cmp_le:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src1 << ", %eax\n";
            o << "    cmpl " << src2 << ", %eax\n";
            o << "    setle %al\n";
            o << "    movzbl %al, %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case cmp_gt:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src1 << ", %eax\n";
            o << "    cmpl " << src2 << ", %eax\n";
            o << "    setg %al\n";
            o << "    movzbl %al, %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case cmp_ge:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src1 << ", %eax\n";
            o << "    cmpl " << src2 << ", %eax\n";
            o << "    setge %al\n";
            o << "    movzbl %al, %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case wmem: {
            // params = [addr_var, value_var]
            // addr_var contient l'adresse (offset %rbp) de la lvalue cible
            // On charge l'adresse effective dans %rax, puis on écrit la valeur
            string addrVar = bb->cfg->IR_reg_to_asm(params[0]);
            string valVar = bb->cfg->IR_reg_to_asm(params[1]);
            o << "    movq " << addrVar << ", %rax\n";
            o << "    movl " << valVar << ", %ecx\n";
            o << "    movl %ecx, (%rax)\n";
            break;
        }

        case wmem_double: {
            // params = [addr_var, value_var]
            string addrVar = bb->cfg->IR_reg_to_asm(params[0]);
            string valVar = bb->cfg->IR_reg_to_asm(params[1]);
            o << "    movq " << addrVar << ", %rax\n";
            o << "    movsd " << valVar << ", %xmm0\n";
            o << "    movsd %xmm0, (%rax)\n";
            break;
        }

        case lea: {
            // params = [dest, src_var], dest = &src_var (load effective address)
            string srcAddr = bb->cfg->IR_reg_to_asm(params[1]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    leaq " << srcAddr << ", %rax\n";
            o << "    movq %rax, " << dest << "\n";
            break;
        }

        case rmem: {
            // params = [dest, addr_var], dest = *(int*)addr_var
            string addrVar = bb->cfg->IR_reg_to_asm(params[1]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movq " << addrVar << ", %rax\n";
            o << "    movl (%rax), %ecx\n";
            o << "    movl %ecx, " << dest << "\n";
            break;
        }

        case rmem_double: {
            // params = [dest, addr_var], dest = *(double*)addr_var
            string addrVar = bb->cfg->IR_reg_to_asm(params[1]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movq " << addrVar << ", %rax\n";
            o << "    movsd (%rax), %xmm0\n";
            o << "    movsd %xmm0, " << dest << "\n";
            break;
        }

        case add_addr: {
            // params = [dest, addr, offset_int], dest = addr + (int64)offset (addr 8 bytes, offset 4 bytes)
            string addrVar = bb->cfg->IR_reg_to_asm(params[1]);
            string offsetVar = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movq " << addrVar << ", %rax\n";
            o << "    movslq " << offsetVar << ", %rcx\n";  // sign-extend int32 -> int64
            o << "    addq %rcx, %rax\n";
            o << "    movq %rax, " << dest << "\n";
            break;
        }

        case mod_int:
            // params = [dest, src1, src2], dest = src1 % src2
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src1 << ", %eax\n";
            o << "    cltd\n";  // sign-extend eax -> edx:eax
            o << "    idivl " << src2 << "\n";
            o << "    movl %edx, " << dest << "\n";  // remainder in %edx
            break;

        case bit_and:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src1 << ", %eax\n";
            o << "    andl " << src2 << ", %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case bit_xor:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src1 << ", %eax\n";
            o << "    xorl " << src2 << ", %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case bit_or:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src1 << ", %eax\n";
            o << "    orl " << src2 << ", %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case logical_not:
            // params = [dest, src], dest = (src == 0) ? 1 : 0
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    cmpl $0, " << src1 << "\n";
            o << "    sete %al\n";
            o << "    movzbl %al, %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case shl:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src2 << ", %ecx\n";
            o << "    movl " << src1 << ", %eax\n";
            o << "    sall %cl, %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        case shr:
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src2 << ", %ecx\n";
            o << "    movl " << src1 << ", %eax\n";
            o << "    sarl %cl, %eax\n";
            o << "    movl %eax, " << dest << "\n";
            break;

        default:
            o << "    # unsupported IR instruction\n";
            break;
    }
}

//  IRInstr ARM64 backend 

void IRInstr::gen_asm_arm64(ostream &o) {
    auto& cfg = *bb->cfg;
    switch(op) {
        case ldconst: {
            int val = stoi(params[1]);
            unsigned int uval = (unsigned int)val;
            if (val >= -65535 && val <= 65535) {
                o << "    mov w8, #" << val << "\n";
            } else {
                o << "    movz w8, #" << (uval & 0xFFFF) << "\n";
                o << "    movk w8, #" << ((uval >> 16) & 0xFFFF) << ", lsl #16\n";
            }
            cfg.arm64_store_w(o, "w8", params[0]);
            break;
        }

        case ldconst_double:
            o << "    // double not supported on ARM64 backend\n";
            break;

        case copy:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case copy_double:
            o << "    // double not supported on ARM64 backend\n";
            break;

        case add:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    add w8, w8, w9\n";
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case sub:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    sub w8, w8, w9\n";
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case mul:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    mul w8, w8, w9\n";
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case div_int:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    sdiv w8, w8, w9\n";
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case mod_int:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    sdiv w10, w8, w9\n";
            o << "    msub w8, w10, w9, w8\n"; // w8 = w8 - w10*w9
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case add_double: case sub_double: case mul_double: case div_double:
        case int_to_double: case double_to_int:
        case rmem_double: case wmem_double:
            o << "    // double not supported on ARM64 backend\n";
            break;

        case cmp_eq:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    cmp w8, w9\n";
            o << "    cset w8, eq\n";
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case cmp_neq:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    cmp w8, w9\n";
            o << "    cset w8, ne\n";
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case cmp_lt:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    cmp w8, w9\n";
            o << "    cset w8, lt\n";
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case cmp_le:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    cmp w8, w9\n";
            o << "    cset w8, le\n";
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case cmp_gt:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    cmp w8, w9\n";
            o << "    cset w8, gt\n";
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case cmp_ge:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    cmp w8, w9\n";
            o << "    cset w8, ge\n";
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case bit_and:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    and w8, w8, w9\n";
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case bit_xor:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    eor w8, w8, w9\n";
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case bit_or:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    orr w8, w8, w9\n";
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case logical_not:
            cfg.arm64_load_w(o, "w8", params[1]);
            o << "    cmp w8, #0\n";
            o << "    cset w8, eq\n";
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case shl:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    lsl w8, w8, w9\n";
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case shr:
            cfg.arm64_load_w(o, "w8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    asr w8, w8, w9\n";
            cfg.arm64_store_w(o, "w8", params[0]);
            break;

        case lea: {
            // dest = &src_var
            int offset = cfg.SymbolIndex[params[1]];
            o << "    sub x8, x29, #" << offset << "\n";
            cfg.arm64_store_x(o, "x8", params[0]);
            break;
        }

        case rmem:
            // dest = *(int*)addr_var
            cfg.arm64_load_x(o, "x8", params[1]);
            o << "    ldr w9, [x8]\n";
            cfg.arm64_store_w(o, "w9", params[0]);
            break;

        case wmem:
            // *(int*)addr_var = value_var
            cfg.arm64_load_x(o, "x8", params[0]);
            cfg.arm64_load_w(o, "w9", params[1]);
            o << "    str w9, [x8]\n";
            break;

        case add_addr:
            // dest = addr + (int64)offset_int
            cfg.arm64_load_x(o, "x8", params[1]);
            cfg.arm64_load_w(o, "w9", params[2]);
            o << "    sxtw x9, w9\n"; // sign-extend 32->64
            o << "    add x8, x8, x9\n";
            cfg.arm64_store_x(o, "x8", params[0]);
            break;

        case call: {
            int numArgs = params.size() - 2;
            string armArgRegs[] = {"w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7"};
            int extraArgs = (numArgs > 8) ? (numArgs - 8) : 0;

            // Réserver de l'espace pour les args sur la pile (aligné 16)
            int stackArgSpace = ((extraArgs * 8 + 15) & ~15);
            if (stackArgSpace > 0) {
                o << "    sub sp, sp, #" << stackArgSpace << "\n";
            }

            // Empiler les arguments au-delà du 8e
            for (int i = 8; i < numArgs; i++) {
                cfg.arm64_load_w(o, "w8", params[i + 2]);
                o << "    sxtw x8, w8\n";
                o << "    str x8, [sp, #" << (i - 8) * 8 << "]\n";
            }

            // Charger les 8 premiers arguments dans les registres
            for (int i = 0; i < numArgs && i < 8; i++) {
                cfg.arm64_load_w(o, armArgRegs[i], params[i + 2]);
            }

            #ifdef __APPLE__
            o << "    bl _" << params[1] << "\n";
            #else
            o << "    bl " << params[1] << "\n";
            #endif

            if (stackArgSpace > 0) {
                o << "    add sp, sp, #" << stackArgSpace << "\n";
            }

            cfg.arm64_store_w(o, "w0", params[0]);
            break;
        }

        default:
            o << "    // unsupported IR instruction on ARM64\n";
            break;
    }
}

//  BasicBlock 

BasicBlock::BasicBlock(CFG* cfg, string entry_label)
    : cfg(cfg), label(entry_label), exit_true(nullptr), exit_false(nullptr) {}

void BasicBlock::gen_asm(ostream &o) {
    if (cfg->target == "arm64") {
        gen_asm_arm64(o);
    } else {
        gen_asm_x86(o);
    }
}

void BasicBlock::gen_asm_x86(ostream &o) {
    o << label << ":\n";
    for (auto instr : instrs) {
        instr->gen_asm(o);
    }
    if (exit_true == nullptr) {
        cfg->gen_asm_epilogue(o);
    } else if (exit_false == nullptr) {
        o << "    jmp " << exit_true->label << "\n";
    } else {
        o << "    cmpl $0, " << cfg->IR_reg_to_asm(test_var_name) << "\n";
        o << "    je " << exit_false->label << "\n";
        o << "    jmp " << exit_true->label << "\n";
    }
}

void BasicBlock::gen_asm_arm64(ostream &o) {
    o << label << ":\n";
    for (auto instr : instrs) {
        instr->gen_asm(o);
    }
    if (exit_true == nullptr) {
        cfg->gen_asm_epilogue(o);
    } else if (exit_false == nullptr) {
        o << "    b " << exit_true->label << "\n";
    } else {
        cfg->arm64_load_w(o, "w8", test_var_name);
        o << "    cmp w8, #0\n";
        o << "    b.eq " << exit_false->label << "\n";
        o << "    b " << exit_true->label << "\n";
    }
}

void BasicBlock::add_IRInstr(IRInstr::Operation op, Type t, vector<string> params) {
    instrs.push_back(new IRInstr(this, op, t, params));
}

//  CFG 

CFG::CFG(DefFonction* ast, string name)
    : ast(ast), funcName(name), returnType(INT), nextFreeSymbolIndex(0), nextBBnumber(1), nextTempVarIndex(0), current_bb(nullptr) {}

void CFG::add_bb(BasicBlock* bb) {
    bbs.push_back(bb);
    current_bb = bb;
}

void CFG::gen_asm(ostream &o) {
    if (target == "arm64") {
        gen_asm_prologue_arm64(o);
        for (auto bb : bbs) {
            bb->gen_asm(o);
        }
    } else {
        // Émettre la section .rodata pour les constantes double de ce CFG
        if (!doubleConstants.empty()) {
            o << "    .section .rodata\n";
            for (auto& [label, val] : doubleConstants) {
                o << "    .align 8\n";
                o << label << ":\n";
                uint64_t bits;
                memcpy(&bits, &val, sizeof(bits));
                o << "    .quad " << bits << "\n";
            }
            o << "    .text\n";
        }
        gen_asm_prologue(o);
        for (auto bb : bbs) {
            bb->gen_asm(o);
        }
    }
}

string CFG::IR_reg_to_asm(string reg) {
    // Si la variable est un pseudo-registre ABI (ex: !edi), retourner le vrai nom x86 (%edi)
    if (reg == "!edi") return "%edi";
    if (reg == "!esi") return "%esi";
    if (reg == "!edx") return "%edx";
    if (reg == "!ecx") return "%ecx";
    if (reg == "!r8d") return "%r8d";
    if (reg == "!r9d") return "%r9d";

    // Si c'est un paramètre sur la pile (au-delà du 6ème)
    if (reg.find("!param") == 0) {
        int index = stoi(reg.substr(6));
        int offset = 16 + (index - 6) * 8;
        return to_string(offset) + "(%rbp)";
    }

    // Convertit un nom de variable IR en adresse mémoire x86
    int index = SymbolIndex[reg];
    return "-" + to_string(index) + "(%rbp)";
}

void CFG::gen_asm_prologue(ostream &o) {
    #ifdef __APPLE__
    o << ".globl _" << funcName << "\n";
    o << "_" << funcName << ":\n";
    #else
    o << ".globl " << funcName << "\n";
    o << funcName << ":\n";
    #endif
    o << "    pushq %rbp\n";
    o << "    movq %rsp, %rbp\n";
    // Réserver de l'espace sur la pile (arrondi à un multiple de 16)
    int stackSize = ((nextFreeSymbolIndex + 15) & ~15);
    o << "    subq $" << stackSize << ", %rsp\n";
    // Initialize !retval to 0 (C99: reaching end of main without return == return 0)
    if (SymbolIndex.find("!retval") != SymbolIndex.end()) {
        if (returnType == DOUBLE) {
            o << "    xorpd %xmm7, %xmm7\n";
            o << "    movsd %xmm7, " << IR_reg_to_asm("!retval") << "\n";
        } else {
            o << "    movl $0, " << IR_reg_to_asm("!retval") << "\n";
        }
    }
}

void CFG::gen_asm_epilogue(ostream &o) {
    if (target == "arm64") {
        if (SymbolIndex.find("!retval") != SymbolIndex.end()) {
            arm64_load_w(o, "w0", "!retval");
        }
        o << "    mov sp, x29\n";
        o << "    ldp x29, x30, [sp], #16\n";
        o << "    ret\n";
    } else {
        if (SymbolIndex.find("!retval") != SymbolIndex.end()) {
            if (returnType == DOUBLE) {
                o << "    movsd " << IR_reg_to_asm("!retval") << ", %xmm0\n";
            } else {
                o << "    movl " << IR_reg_to_asm("!retval") << ", %eax\n";
            }
        }
        o << "    leave\n";
        o << "    ret\n";
    }
}

void CFG::add_to_symbol_table(string name, Type t) {
    int size = typeSize(t);
    // Aligner sur la taille du type (les doubles/pointeurs sur 8 octets)
    if (size == 8 && (nextFreeSymbolIndex % 8 != 0)) {
        nextFreeSymbolIndex += (8 - (nextFreeSymbolIndex % 8));
    }
    nextFreeSymbolIndex += size;  // réserver l'espace d'abord
    SymbolType[name] = t;
    SymbolIndex[name] = nextFreeSymbolIndex;  // index = bord supérieur de l'allocation
}

string CFG::create_new_tempvar(Type t) {
    string name = "!tmp" + to_string(nextTempVarIndex++);
    add_to_symbol_table(name, t);
    return name;
}

int CFG::get_var_index(string name) {
    return SymbolIndex[name];
}

Type CFG::get_var_type(string name) {
    return SymbolType[name];
}

void CFG::add_array_to_symbol_table(string name, Type elementType, int size) {
    int elemSize = typeSize(elementType);
    int totalSize = elemSize * size;
    // Aligner sur 8 octets si nécessaire (pour les doubles)
    if (elemSize == 8 && (nextFreeSymbolIndex % 8 != 0)) {
        nextFreeSymbolIndex += (8 - (nextFreeSymbolIndex % 8));
    }
    nextFreeSymbolIndex += totalSize;  // réserver l'espace total du tableau
    SymbolType[name] = elementType;    // le type de la variable est le type des éléments
    SymbolIndex[name] = nextFreeSymbolIndex;  // index = bord supérieur de l'allocation
    ArrayElementType[name] = elementType;
    ArraySize[name] = size;
}

bool CFG::is_array(string name) {
    return ArrayElementType.find(name) != ArrayElementType.end();
}

Type CFG::get_array_element_type(string name) {
    return ArrayElementType[name];
}

string CFG::new_BB_name() {
    return ".LBB_" + funcName + "_" + to_string(nextBBnumber++);
}

//  ARM64 helpers 

void CFG::arm64_load_w(ostream& o, string wreg, string ir_var) {
    // Pseudo-registres ABI x86 -> ARM64
    if (ir_var == "!edi") { if (wreg != "w0") o << "    mov " << wreg << ", w0\n"; return; }
    if (ir_var == "!esi") { if (wreg != "w1") o << "    mov " << wreg << ", w1\n"; return; }
    if (ir_var == "!edx") { if (wreg != "w2") o << "    mov " << wreg << ", w2\n"; return; }
    if (ir_var == "!ecx") { if (wreg != "w3") o << "    mov " << wreg << ", w3\n"; return; }
    if (ir_var == "!r8d") { if (wreg != "w4") o << "    mov " << wreg << ", w4\n"; return; }
    if (ir_var == "!r9d") { if (wreg != "w5") o << "    mov " << wreg << ", w5\n"; return; }

    // Parametres au-dela du 6e (index x86) -> sur ARM64, les params 6 et 7 sont encore en registre
    if (ir_var.find("!param") == 0) {
        int index = stoi(ir_var.substr(6));
        if (index < 8) {
            string armReg = "w" + to_string(index);
            if (wreg != armReg) o << "    mov " << wreg << ", " << armReg << "\n";
        } else {
            int offset = 16 + (index - 8) * 8;
            o << "    ldur " << wreg << ", [x29, #" << offset << "]\n";
        }
        return;
    }

    // Variable sur la pile (offset négatif par rapport à x29)
    int offset = SymbolIndex[ir_var];
    if (offset <= 255) {
        o << "    ldur " << wreg << ", [x29, #-" << offset << "]\n";
    } else {
        o << "    sub x11, x29, #" << offset << "\n";
        o << "    ldr " << wreg << ", [x11]\n";
    }
}

void CFG::arm64_store_w(ostream& o, string wreg, string ir_var) {
    int offset = SymbolIndex[ir_var];
    if (offset <= 255) {
        o << "    stur " << wreg << ", [x29, #-" << offset << "]\n";
    } else {
        o << "    sub x11, x29, #" << offset << "\n";
        o << "    str " << wreg << ", [x11]\n";
    }
}

void CFG::arm64_load_x(ostream& o, string xreg, string ir_var) {
    int offset = SymbolIndex[ir_var];
    if (offset <= 255) {
        o << "    ldur " << xreg << ", [x29, #-" << offset << "]\n";
    } else {
        o << "    sub x11, x29, #" << offset << "\n";
        o << "    ldr " << xreg << ", [x11]\n";
    }
}

void CFG::arm64_store_x(ostream& o, string xreg, string ir_var) {
    int offset = SymbolIndex[ir_var];
    if (offset <= 255) {
        o << "    stur " << xreg << ", [x29, #-" << offset << "]\n";
    } else {
        o << "    sub x11, x29, #" << offset << "\n";
        o << "    str " << xreg << ", [x11]\n";
    }
}

void CFG::gen_asm_prologue_arm64(ostream& o) {
    #ifdef __APPLE__
    o << ".globl _" << funcName << "\n";
    o << ".p2align 2\n";
    o << "_" << funcName << ":\n";
    #else
    o << ".globl " << funcName << "\n";
    o << ".p2align 2\n";
    o << funcName << ":\n";
    #endif
    // Sauvegarder le frame pointer et le link register
    o << "    stp x29, x30, [sp, #-16]!\n";
    o << "    mov x29, sp\n";
    // Réserver de l'espace pour les variables locales (arrondi à 16)
    int stackSize = ((nextFreeSymbolIndex + 15) & ~15);
    if (stackSize > 0) {
        o << "    sub sp, sp, #" << stackSize << "\n";
    }
}
