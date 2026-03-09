#include "IR.h"
#include <map>
#include <cstring>

using namespace std;

// ==================== IRInstr ====================

IRInstr::IRInstr(BasicBlock* bb_, Operation op, Type t, vector<string> params)
    : bb(bb_), op(op), t(t), params(params) {}

void IRInstr::gen_asm(ostream &o) {
    string dest, src1, src2;
    switch(op) {
        case ldconst:
            // params = [dest, constante]
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl $" << params[1] << ", " << dest << "\n";
            break;

        case ldconst_double:
            // params = [dest, label]  — le label pointe vers .rodata
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
            // params = [dest, src1, src2] → dest = src1 / src2
            src1 = bb->cfg->IR_reg_to_asm(params[1]);
            src2 = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movl " << src1 << ", %eax\n";
            o << "    cltd\n";  // sign-extend eax → edx:eax
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
            // params = [dest, src_var] — dest = &src_var (load effective address)
            string srcAddr = bb->cfg->IR_reg_to_asm(params[1]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    leaq " << srcAddr << ", %rax\n";
            o << "    movq %rax, " << dest << "\n";
            break;
        }

        case rmem: {
            // params = [dest, addr_var] — dest = *(int*)addr_var
            string addrVar = bb->cfg->IR_reg_to_asm(params[1]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movq " << addrVar << ", %rax\n";
            o << "    movl (%rax), %ecx\n";
            o << "    movl %ecx, " << dest << "\n";
            break;
        }

        case rmem_double: {
            // params = [dest, addr_var] — dest = *(double*)addr_var
            string addrVar = bb->cfg->IR_reg_to_asm(params[1]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movq " << addrVar << ", %rax\n";
            o << "    movsd (%rax), %xmm0\n";
            o << "    movsd %xmm0, " << dest << "\n";
            break;
        }

        case add_addr: {
            // params = [dest, addr, offset_int] — dest = addr + (int64)offset (addr is 8-byte, offset is 4-byte int)
            string addrVar = bb->cfg->IR_reg_to_asm(params[1]);
            string offsetVar = bb->cfg->IR_reg_to_asm(params[2]);
            dest = bb->cfg->IR_reg_to_asm(params[0]);
            o << "    movq " << addrVar << ", %rax\n";
            o << "    movslq " << offsetVar << ", %rcx\n";  // sign-extend int32 → int64
            o << "    addq %rcx, %rax\n";
            o << "    movq %rax, " << dest << "\n";
            break;
        }

        default:
            o << "    # unsupported IR instruction\n";
            break;
    }
}

// ==================== BasicBlock ====================

BasicBlock::BasicBlock(CFG* cfg, string entry_label)
    : cfg(cfg), label(entry_label), exit_true(nullptr), exit_false(nullptr) {}

void BasicBlock::gen_asm(ostream &o) {
    // Émettre le label du bloc
    o << label << ":\n";

    // Générer le code de chaque instruction
    for (auto instr : instrs) {
        instr->gen_asm(o);
    }

    // Gestion des sauts en sortie de bloc
    if (exit_true == nullptr) {
        // Dernier bloc → épilogue (leave + ret)
        cfg->gen_asm_epilogue(o);
    } else if (exit_false == nullptr) {
        // Saut inconditionnel vers exit_true
        o << "    jmp " << exit_true->label << "\n";
    } else {
        // Branchement conditionnel (pour if/while)
        o << "    cmpl $0, " << cfg->IR_reg_to_asm(test_var_name) << "\n";
        o << "    je " << exit_false->label << "\n";
        o << "    jmp " << exit_true->label << "\n";
    }
}

void BasicBlock::add_IRInstr(IRInstr::Operation op, Type t, vector<string> params) {
    instrs.push_back(new IRInstr(this, op, t, params));
}

// ==================== CFG ====================

CFG::CFG(DefFonction* ast, string name)
    : ast(ast), funcName(name), returnType(INT), nextFreeSymbolIndex(0), nextBBnumber(1), nextTempVarIndex(0), current_bb(nullptr) {}

void CFG::add_bb(BasicBlock* bb) {
    bbs.push_back(bb);
    current_bb = bb;
}

void CFG::gen_asm(ostream &o) {
    // Émettre la section .rodata pour les constantes double de ce CFG
    if (!doubleConstants.empty()) {
        o << "    .section .rodata\n";
        for (auto& [label, val] : doubleConstants) {
            o << "    .align 8\n";
            o << label << ":\n";
            // Écrire la représentation binaire IEEE 754 du double comme .quad
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

string CFG::IR_reg_to_asm(string reg) {
    // Si la variable est un pseudo-registre ABI (ex: !edi), retourner le vrai nom x86 (%edi)
    if (reg == "!edi") return "%edi";
    if (reg == "!esi") return "%esi";
    if (reg == "!edx") return "%edx";
    if (reg == "!ecx") return "%ecx";
    if (reg == "!r8d") return "%r8d";
    if (reg == "!r9d") return "%r9d";

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
}

void CFG::gen_asm_epilogue(ostream &o) {
    // Charger la valeur de retour
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
