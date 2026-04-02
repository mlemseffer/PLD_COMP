# Fiche P6 — ABI AMD64 & Génération de Code x86-64

## Ce que tu dois expliquer (7 minutes)

1. Qu'est-ce que l'ABI System V AMD64 et pourquoi c'est critique
2. Les registres utilisés : args, retour, frame pointer
3. Prologue de fonction : `pushq %rbp`, `subq $N`, initialisation `!retval`
4. Épilogue : `exit_bb` unique, `leave`, `ret`
5. Alignement de la pile à 16 octets
6. Appels de fonctions : passage des arguments, appel, récupération du retour
7. Exemple complet Fibonacci : source C → IR → asm
8. Gestion des doubles (XMM) + conversions int↔double

---

## Déroulé minute par minute

- **0:00–0:45** : Définition ABI + registres (slides 55–56)
  > "L'ABI, c'est le contrat entre notre compilateur et le monde extérieur : le système d'exploitation, la libc, les autres bibliothèques. Si on le viole, le programme plante. Voici les règles qu'on respecte."

- **0:45–2:00** : Prologue (slide 57)
  > Montrer le code asm du prologue ligne par ligne. Insister sur l'init de `!retval` à 0 (bug qu'on a eu !).

- **2:00–2:45** : Épilogue + exit_bb (slide 58)
  > Montrer que tous les `return` convergent vers un seul épilogue.

- **2:45–3:30** : Alignement 16 octets (slide 59)
  > Expliquer le problème, montrer le calcul.

- **3:30–4:30** : Appels de fonctions (slide 60)
  > Exemple add(3,4) : charger args dans registres, call, récupérer %eax.

- **4:30–5:15** : Variables en mémoire (slide 61)
  > Expliquer le choix : tout en mémoire. Simple, correct. Pas d'allocation de registres.

- **5:15–6:30** : Exemple Fibonacci (slides 62–64)
  > Aller vite. Montrer source → IR → asm. Pointer les correspondances.

- **6:30–7:00** : Doubles + conversions (slides 65–66) + transition

---

## Les concepts clés à maîtriser

### ABI System V AMD64
C'est le standard sur Linux x86-64. Il définit :
- **Passage des arguments** : les 6 premiers arguments entiers dans `%edi, %esi, %edx, %ecx, %r8d, %r9d`. Le 7ème et suivants : sur la pile (de droite à gauche).
- **Valeur de retour** : entier dans `%eax`, double dans `%xmm0`.
- **Registres callee-saved** : `%rbx, %rbp, %r12–%r15` (à sauvegarder si on les utilise).
- **Registres caller-saved** : `%rax, %rcx, %rdx, %rsi, %rdi, %r8–%r11` (peuvent être détruits par un appel).
- **Alignement** : pile alignée à 16 octets avant tout `call`.

### Prologue complet
```asm
functionName:
    pushq  %rbp                ; empile le frame pointer précédent
    movq   %rsp, %rbp          ; le nouveau frame pointer = sommet de pile
    subq   $N, %rsp            ; réserver N octets pour les locales (N multiple de 16)
    movl   $0, -4(%rbp)       ; !retval = 0 (C99 : main sans return → 0)
    ; Copier les arguments depuis les registres vers la pile
    movl   %edi, -8(%rbp)     ; paramètre 1
    movl   %esi, -12(%rbp)    ; paramètre 2
    movsd  %xmm0, -20(%rbp)   ; paramètre double (si applicable)
```

**Pourquoi copier les args vers la pile ?** Pour qu'on puisse les traiter comme de simples variables locales avec un offset fixe. On n'a pas d'allocateur de registres → tout est en mémoire.

### Épilogue via exit_bb
```asm
.exit_bb:
    movl   -4(%rbp), %eax     ; charger !retval dans %eax
    leave                      ; équivalent à: movq %rbp,%rsp; popq %rbp
    ret                        ; retour à l'appelant
```

`leave` est une instruction x86 qui restaure le frame pointer et dépile en une seule instruction.

**Un seul épilogue par fonction** : tous les `return` font `copy !retval value; jmp .exit_bb`. L'épilogue réel n'est écrit qu'une fois.

### Alignement 16 octets
La pile doit être alignée à 16 octets **avant** chaque `call`. Après `pushq %rbp`, la pile est décalée de 8 par rapport à un multiple de 16. Donc `N` dans `subq $N, %rsp` doit satisfaire :

`(N + 8) mod 16 == 0` → `N mod 16 == 8`

En pratique : on calcule le total des locales en octets, on arrondit au multiple de 16 **supérieur**, puis on ajuste pour le décalage de 8.

Exemple : 3 variables int = 12 octets → arrondir à 16 → `subq $16, %rsp`.

### Variables locales en mémoire
Toutes les variables sont stockées à un offset négatif de `%rbp` :
- Variable 1 : `-4(%rbp)` (4 octets = int)
- Variable 2 : `-8(%rbp)`
- Variable double : `-16(%rbp)` (8 octets)

Chaque opération sur une variable :
1. **Load** depuis la pile dans un registre scratch (`%eax`, `%edx`)
2. **Opération** dans le registre
3. **Store** du résultat sur la pile

C'est moins efficace que d'utiliser les registres directement (allocation de registres), mais c'est simple et correct.

### Instruction de comparaison
```asm
; !tmp0 = (a_0 < b_0)
movl  -8(%rbp), %eax    ; charge a_0
cmpl  -12(%rbp), %eax   ; compare a_0 avec b_0
setl  %al               ; AL = 1 si a_0 < b_0, sinon 0
movzbl %al, %eax        ; zero-extend vers 32 bits
movl  %eax, -16(%rbp)  ; stocke !tmp0
```

### Branchement conditionnel
```asm
; test !tmp0 → sauter vers bb_else si !tmp0 == 0
movl  -16(%rbp), %eax
testl %eax, %eax
je    .bb_else
; ... bloc then ...
jmp   .bb_merge
.bb_else:
```

### Doubles : registres XMM et SSE2
```asm
movsd  .LC0(%rip), %xmm0   ; charger double depuis .rodata
addsd  %xmm1, %xmm0        ; addition double
movsd  %xmm0, -16(%rbp)   ; stocker
```

Constantes double en `.rodata` :
```asm
.section .rodata
.LC0:
    .double 3.14159
```

---

## Exemples de code à connaître

### Exemple complet : add(3, 4)
```c
int add(int a, int b) { return a + b; }
int main() { return add(3, 4); }
```

```asm
add:
    pushq %rbp
    movq  %rsp, %rbp
    subq  $16, %rsp
    movl  $0, -4(%rbp)     ; !retval = 0
    movl  %edi, -8(%rbp)   ; a = %edi
    movl  %esi, -12(%rbp)  ; b = %esi
    ; return a + b
    movl  -8(%rbp), %eax
    addl  -12(%rbp), %eax
    movl  %eax, -4(%rbp)   ; !retval = a+b
    jmp   .exit_bb
.exit_bb:
    movl  -4(%rbp), %eax
    leave
    ret

main:
    pushq %rbp
    movq  %rsp, %rbp
    subq  $16, %rsp
    movl  $0, -4(%rbp)
    ; add(3, 4)
    movl  $3, %edi
    movl  $4, %esi
    call  add
    movl  %eax, -4(%rbp)   ; !retval = résultat
    jmp   .exit_bb
.exit_bb:
    movl  -4(%rbp), %eax
    leave
    ret
```

### Conversion int → double
```asm
; double d = 42;
movl $42, -4(%rbp)             ; d'abord en int temporaire
cvtsi2sdl -4(%rbp), %xmm0     ; converti en double
movsd %xmm0, -12(%rbp)        ; stocke dans la variable double
```

### Conversion double → int
```asm
; int i = d;
movsd -12(%rbp), %xmm0
cvttsd2si %xmm0, %eax          ; troncature vers zéro
movl %eax, -4(%rbp)
```

---

## Questions pièges possibles du jury

**Q : Pourquoi `leave` plutôt que `movq %rbp,%rsp; popq %rbp` ?**
R : `leave` est exactement équivalent à cette séquence, mais encodé en 1 octet. C'est une instruction historique x86 conçue pour les prologues/épilogues de fonctions.

**Q : Que se passe-t-il si la pile n'est pas alignée à 16 octets avant un `call` ?**
R : Les instructions SSE (comme `movaps`) requièrent un alignement à 16 octets et lèvent un `#GP` (General Protection fault) si violé. Même sans SSE, certaines ABIs requièrent cet alignement. En pratique, le programme segfaulte de manière cryptique.

**Q : Vous initialisez `!retval` à 0 dans le prologue. Pourquoi ?**
R : C99 spécifie que `main()` sans `return` retourne 0. Sans cette initialisation, si le flot atteint `exit_bb` sans passer par un `return` explicite, `%eax` contiendrait une valeur aléatoire. C'était notre dernier bug : le test `5_no_return` crashait à cause de ça.

**Q : Comment gérez-vous les fonctions avec plus de 6 arguments ?**
R : Les arguments 7+ sont poussés sur la pile de droite à gauche avant le `call`, selon l'ABI SysV. On génère autant de `pushq` que nécessaire, puis on nettoie la pile avec `addq $N, %rsp` après l'appel.

**Q : Vos appels de `putchar`/`getchar` utilisent-ils l'ABI correctement ?**
R : Oui. `putchar(c)` met `c` dans `%edi` et fait `call putchar`. `getchar()` fait juste `call getchar` et récupère le résultat dans `%eax`. Les deux sont des fonctions C standard, donc leur ABI est SysV AMD64.

**Q : Pourquoi ne pas allouer les variables fréquemment utilisées dans des registres ?**
R : L'allocation de registres est un problème NP-complet (coloration de graphe). Notre approche est correcte mais conservatrice. GCC et LLVM passent beaucoup de temps sur cette passe. C'est une amélioration future clairement identifiée.

**Q : La constante double `.LC0` est dans `.rodata`. Pourquoi pas en `.data` ?**
R : `.rodata` = read-only data. Les constantes ne sont pas modifiées à l'exécution. Les mettre en `.rodata` permet au système de partager cette section entre processus (si le même programme est lancé plusieurs fois) et empêche une modification accidentelle.

---

## Phrases d'accroche pour enchaîner avec P7

> "On vient de couvrir la génération de code x86-64. Mais notre compilateur supporte aussi ARM64, et plusieurs fonctionnalités avancées comme les types double, les tableaux, le switch. P7 va vous montrer tout ça."

> "x86-64, c'est la cible principale. Mais une des fiertés du projet, c'est le support ARM64 — le même IR, un backend différent. [P7] vous explique aussi les features avancées qu'on a implémentées."
