# Fiche P7 — Features Avancées & ARM64

## Ce que tu dois expliquer (7 minutes)

1. Type `double` : opérations flottantes, stockage `.rodata`
2. Inférence de type : conversions implicites `int↔double`
3. Tableaux 1D : déclaration, accès indexé (`lea`, `add_addr`, `wmem`, `rmem`)
4. `switch/case` : CFG avec comparaisons chaînées
5. `for`, `do-while` : CFG spécifiques
6. Opérateur ternaire `? :`
7. ARM64 : ABI AAPCS64, prologue/épilogue, accès mémoire, dispatch

---

## Déroulé minute par minute

- **0:00–1:30** : Double + inférence de type (slides 69–70)
  > "Notre compilateur supporte les flottants 64 bits. Le point intéressant : quand on mélange int et double dans une expression, on insère automatiquement la conversion."

- **1:30–2:30** : Tableaux 1D (slides 71–72)
  > Montrer le code C, l'IR générée (lea, add_addr, wmem, rmem), et l'asm correspondant.

- **2:30–3:30** : switch/case + for + do-while (slides 73–74)
  > Montrer rapidement les schémas CFG. Insister sur switch = comparaisons chaînées (pas de jump table).

- **3:30–4:00** : Ternaire (slide 75) + opérateurs composés (slide 76)
  > Rapide. Ternaire = 3 blocs + temporaire commun. Composés = sucre syntaxique sur les opérations de base.

- **4:00–5:30** : ARM64 : registres, prologue/épilogue (slides 77–79)
  > C'est la partie la plus technique. Montrer le tableau de comparaison x86-64 / ARM64. Détailler le prologue stp/ldp.

- **5:30–6:30** : ARM64 : accès mémoire + dispatch (slides 80–81)
  > Montrer le cas offset > 255. Montrer le dispatch dans `gen_asm()`.

- **6:30–7:00** : Bilan + transition (slide 82)

---

## Les concepts clés à maîtriser

### Type `double` : registres XMM et SSE2
Les `double` utilisent les registres vectoriels XMM :
- Stockage : 8 octets sur la pile, aligné à 8 octets
- Opérations : `addsd`, `subsd`, `mulsd`, `divsd` (Scalar Double)
- Comparaison : `ucomisd` + `seta`/`setb`/etc.
- Constantes littérales : stockées dans `.rodata` avec un label unique par valeur

```asm
.section .rodata
.LC0:
    .double 3.14
.text:
    movsd .LC0(%rip), %xmm0   ; chargement via adressage RIP-relative
```

L'adressage `%rip`-relative est nécessaire car le code est compilé en position-independante (PIE).

### Inférence de type (type coercion)
Quand deux opérandes ont des types différents dans une expression binaire :
- Si l'un est `double` et l'autre `int` → on promeut l'`int` en `double`
- L'IRGenVisitor insère une instruction `int_to_double` avant l'opération
- Pour une affectation `int = double` → on insère `double_to_int`

```c
double d = 3;       // 3 est un INT_LITERAL → int_to_double inséré
int i = 2.7 + 1;    // 2.7 est double, 1 est int → 1 promu en double → résultat double → double_to_int
```

IR correspondant :
```
ldconst int !tmp0 3
int_to_double double d_0 !tmp0    ; 3 → 3.0
---
ldconst double !tmp1 2.7
ldconst int !tmp2 1
int_to_double double !tmp3 !tmp2  ; 1 → 1.0
add_d double !tmp4 !tmp1 !tmp3    ; 2.7 + 1.0 = 3.7
double_to_int int i_0 !tmp4       ; 3.7 → 3 (troncature)
```

### Tableaux 1D
Déclaration `int arr[5]` alloue 5×4 = 20 octets sur la pile.

Accès `arr[i]` :
1. `lea arr_0` → charge l'**adresse** de base du tableau dans un registre
2. `add_addr !ptr arr_0 i_0` → calcule `base + i * sizeof(int)` = `base + i * 4`
3. `wmem !ptr value` pour écriture, `rmem dest !ptr` pour lecture

En asm x86-64 :
```asm
; arr[2] = 42
leaq  -24(%rbp), %rax          ; adresse base du tableau
movl  $2, %edx                 ; index
movl  $42, (%rax,%rdx,4)       ; *(base + 2*4) = 42

; int x = arr[2]
leaq  -24(%rbp), %rax
movl  $2, %edx
movl  (%rax,%rdx,4), %ecx      ; x = *(base + 2*4)
```

### switch/case : comparaisons chaînées
Pas de jump table (trop complexe pour un cas général). Chaque `case` = une comparaison.

```c
switch (x) {
    case 1: a(); break;
    case 2: b(); break;
    default: c();
}
```

CFG :
```
bb_switch:
  cmp_eq !t0 x_0 1  →  vrai: bb_case1, faux: bb_check2
bb_check2:
  cmp_eq !t1 x_0 2  →  vrai: bb_case2, faux: bb_default
bb_case1:
  call a
  jmp bb_end     ← break
bb_case2:
  call b
  jmp bb_end
bb_default:
  call c
bb_end:
```

### ARM64 : différences clés avec x86-64

| Aspect | x86-64 | ARM64 |
|--------|--------|-------|
| Frame pointer | `%rbp` | `x29` |
| Adresse de retour | sur la pile (call) | `x30` (link register) |
| Args entiers | `edi, esi, edx, ecx, r8d, r9d` | `w0, w1, w2, ..., w7` |
| Retour entier | `eax` | `w0` |
| Sauvegarde lr | automatique (call) | manuelle (`stp x29,x30,...`) |
| Accès mémoire | `movl -8(%rbp), %eax` | `ldur w8, [x29, #-8]` |

### ARM64 : prologue/épilogue
```asm
; Prologue
stp x29, x30, [sp, #-16]!  ; push frame pointer ET link register ensemble
mov x29, sp                 ; nouveau frame pointer
sub sp, sp, #N              ; réserver N octets (multiple de 16)
; Copie des args depuis w0,w1,... vers la pile
str w0, [x29, #-8]         ; param1

; Épilogue
mov sp, x29                ; restaurer stack pointer
ldp x29, x30, [sp], #16   ; pop frame pointer et link register
ret                         ; retour (via x30)
```

`stp`/`ldp` = Store/Load Pair : sauvegarde/restaure deux registres 64 bits en une instruction.

**Attention** : en ARM64, l'adresse de retour est dans `x30` (link register). `call` s'appelle `bl` (Branch with Link). Si une fonction en appelle une autre, elle doit sauvegarder `x30` → d'où le `stp x29, x30, [sp, #-16]!`.

### ARM64 : accès mémoire et limitation d'offset
`ldur`/`stur` supportent des offsets signés sur 9 bits : -256 à +255.

Pour des fonctions avec beaucoup de variables locales (offset > 255) :
```asm
; Offset -260 (> 255) : impossible en ldur direct
mov x11, #-260
add x11, x29, x11     ; calcule l'adresse effective
ldr w8, [x11]         ; charge depuis l'adresse calculée
```

Notre compilateur détecte ce cas et génère le code approprié.

### Dispatch dans gen_asm()
```cpp
void IRInstr::gen_asm(ostream& o) {
    if (cfg->target == "arm64")
        gen_asm_arm64(o);
    else
        gen_asm_x86(o);
}
```

Chaque opération IR a deux implémentations asm. L'IR elle-même est identique pour les deux cibles. Le flag `target` est passé à la ligne de commande :
```bash
ifcc -target arm64 source.c
```

---

## Exemples de code à connaître

### Tableau complet
```c
int arr[3];
arr[0] = 10;
arr[1] = 20;
int sum = arr[0] + arr[1];
```

IR :
```
lea arr_0 [base addr]
add_addr !ptr0 arr_0 0
wmem !ptr0 10
add_addr !ptr1 arr_0 1
wmem !ptr1 20
rmem !tmp0 !ptr0
rmem !tmp1 !ptr1
add sum_0 !tmp0 !tmp1
```

### Inférence de type
```c
double pi = 3.14;
int r = 2;
double area = pi * r;   // r promu en double
```

### ARM64 vs x86-64 : même C, assembleur différent
```c
int add(int a, int b) { return a + b; }
```

x86-64 :
```asm
add:
    pushq %rbp; movq %rsp, %rbp; subq $16, %rsp
    movl %edi, -8(%rbp); movl %esi, -12(%rbp)
    movl -8(%rbp), %eax; addl -12(%rbp), %eax
    movl %eax, -4(%rbp)
    movl -4(%rbp), %eax; leave; ret
```

ARM64 :
```asm
add:
    stp x29, x30, [sp, #-16]!; mov x29, sp; sub sp, sp, #16
    str w0, [x29, #-8]; str w1, [x29, #-12]
    ldur w8, [x29, #-8]; ldur w9, [x29, #-12]; add w8, w8, w9
    str w8, [x29, #-4]
    ldur w0, [x29, #-4]; mov sp, x29; ldp x29, x30, [sp], #16; ret
```

---

## Questions pièges possibles du jury

**Q : Pourquoi utiliser des comparaisons chaînées pour switch et pas une jump table ?**
R : Une jump table est efficace pour des cases contiguës (case 1, 2, 3, 4...). Pour des cases épars (case 1, 100, 1000), elle gaspille de la mémoire. Notre approche est universelle et simple à implémenter. Un compilateur optimisant génèrerait une jump table si les cases sont denses.

**Q : ARM64 supporte les mêmes opérations que x86-64 ?**
R : Toutes les opérations sont supportées, mais les instructions diffèrent. ARM64 est une architecture RISC : moins d'instructions, mais plus régulières. Par exemple, ARM64 n'a pas d'instruction `imul` à 2 opérandes comme x86 — on utilise `mul` avec 3 registres.

**Q : La conversion `double_to_int` tronque-t-elle vers zéro ou arrondit-elle ?**
R : `cvttsd2si` sur x86-64 tronque vers zéro (le `t` = truncate). C'est le comportement standard C : `(int)3.9 == 3`, `(int)-3.9 == -3`. Sur ARM64, on utilise `fcvtzs` (Float Convert to Zero, Signed) pour le même comportement.

**Q : Votre ARM64 tourne-t-il sur Apple Silicon en natif ?**
R : Oui. On a testé sur une machine M1. L'assembleur généré est ensuite assemblé avec `as` (assembler Apple) et lié avec `ld`. Le format de sortie est MachO au lieu d'ELF, ce qui nécessite quelques ajustements dans les directives (`.section __DATA, __data` au lieu de `.data`, etc.).

**Q : Comment gérez-vous les tableaux passés en argument de fonction ?**
R : On ne supporte pas les tableaux en arguments de fonction (pas de pointeurs). Nos tableaux sont uniquement des variables locales. C'est une limitation documentée.

**Q : Les opérateurs composés `+=` sont-ils sucre syntaxique dans votre implémentation ?**
R : Dans la grammaire, `x += e` est une règle distincte (`compoundAssign`). Dans l'IRGenVisitor, on le traduit directement en `add x x e` sans passer par un `x = x + e` intermédiaire — ce qui est équivalent mais légèrement plus direct.

**Q : Qu'est-ce qui empêche les tableaux hors limites ?**
R : Rien — comme en C standard. Pas de bounds checking à l'exécution. C'est délibéré : notre cible est un sous-ensemble C semantiquement fidèle au standard.

---

## Phrases d'accroche pour enchaîner avec P8

> "Vous avez maintenant une vue complète de notre compilateur, des features au backend. P8 va vous montrer la preuve que tout ça fonctionne : 75 tests sur 75, et une démo en direct."

> "Features implémentées, backends validés — reste à prouver que ça marche. [P8] vous montre nos tests, notre CI, et une démo du programme game.c compilé avec ifcc."
