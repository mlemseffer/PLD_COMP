# Fiche P4 — IR & CFG (Représentation Intermédiaire & Graphe de Flot de Contrôle)

## Ce que tu dois expliquer (7 minutes)

1. Définition et rôle de l'IR 3 adresses
2. Structure d'une IRInstr : format `[op, type, dest, src1, src2]`
3. Variables spéciales IR (`!retval`, `!tmpN`, pseudo-registres)
4. CFG : un graphe par fonction, BasicBlocks reliés par arêtes
5. Structure d'un BasicBlock (exit_true / exit_false / test_var)
6. `exit_bb` unique : un seul épilogue par fonction
7. Exemples : `if/else`, `while`, `for` en CFG
8. `loopStack` pour `break`/`continue`

---

## Déroulé minute par minute

- **0:00–1:00** : Définition IR + format 3 adresses (slides 31–32)
  > "L'IR est notre langage intermédiaire : plus simple que l'assembleur (pas de registres physiques), plus proche de la machine que le C (pas de sucre syntaxique). Chaque instruction prend au plus 2 sources et produit 1 destination."

- **1:00–2:00** : Catalogue d'opérations + variables spéciales (slides 33–34)
  > Montrer le tableau des ~40 opérations. Expliquer `!retval`, `!tmp0`, `!edi`.

- **2:00–3:15** : CFG + structure d'un BasicBlock (slides 35–37)
  > Dessiner un CFG simple. Montrer `exit_true`, `exit_false`, `test_var_name`. Insister sur `exit_bb` unique.

- **3:15–4:30** : Exemple if/else + while en CFG (slides 38–39)
  > Dessiner les schémas. C'est le cœur de la présentation.

- **4:30–5:30** : `loopStack` pour break/continue (slide 40)
  > Expliquer la pile de contextes. Montrer comment `break` court-circuite.

- **5:30–6:30** : Exemple `for` (slide 41) + lien avec gen_asm (slide 42)
  > "4 BasicBlocks distincts. Et ensuite, gen_asm() itère sur les blocs dans l'ordre et traduit chaque instruction IR en assembleur — P6 détaillera ça."

- **6:30–7:00** : Transition vers P5

---

## Les concepts clés à maîtriser

### IR 3 adresses
Format : `[opération, type, dest, src1, src2]`

Chaque instruction effectue **au plus** une opération avec **au plus** 2 opérandes sources et **1** destination. Ça force la décomposition des expressions complexes en une suite d'étapes simples.

Exemples :
```
add    int  !tmp0  x_0   y_0      →  !tmp0 = x_0 + y_0
ldconst int  z_0   42            →  z_0 = 42
cmp_lt int  !tmp1  a_0   b_0    →  !tmp1 = (a_0 < b_0) → 0 ou 1
call   int  !tmp2  putchar [c_0] →  !tmp2 = putchar(c_0)
```

### Variables spéciales
- `!retval` : valeur de retour courante. Initialisée à 0 dans le prologue. Tout `return expr` fait `copy !retval expr; jmp exit_bb`.
- `!tmp0`, `!tmp1`, ... : temporaires générés automatiquement pour les sous-expressions.
- `!edi`, `!esi`, `!edx`, `!ecx`, `!r8d`, `!r9d` : pseudo-registres pour les 6 premiers arguments en ABI AMD64. Ils sont copiés vers la pile dans le prologue.
- Convention : `!` préfixe = variable interne, non issue du code source.

### CFG (Graphe de Flot de Contrôle)
- **Un CFG par fonction**
- Nœuds = BasicBlocks
- Arêtes = branchements (conditionnels ou inconditionnels)
- Propriété clé : dans un BasicBlock, le flot est **linéaire** (pas de branchement au milieu)

### Structure d'un BasicBlock
```cpp
struct BasicBlock {
    vector<IRInstr*> instrs;   // liste d'instructions IR
    BasicBlock* exit_true;     // successeur si test vrai (ou unique successeur)
    BasicBlock* exit_false;    // successeur si test faux
    string test_var_name;      // variable à tester pour le branchement
    string label;              // étiquette asm unique (ex: ".bb_3")
    bool has_return;           // ce bloc termine-t-il par un return ?
};
```

Si `test_var_name` est vide → branchement inconditionnel vers `exit_true`.

### exit_bb unique
Tous les `return` dans une fonction sautent vers `exit_bb`. L'épilogue (restauration registres, `leave`, `ret`) est **écrit une seule fois** dans `exit_bb`. Sans ça, chaque `return` au milieu d'une fonction nécessiterait son propre épilogue.

Le flag `has_return` sur un BasicBlock évite de générer un saut superflu après un bloc qui se termine par un `return` (le bloc saute déjà vers `exit_bb`).

### loopStack (break/continue)
```cpp
struct LoopContext {
    BasicBlock* bb_cond;  // destination de continue
    BasicBlock* bb_end;   // destination de break
};
stack<LoopContext> loopStack;
```

À chaque `break` : `currentBB->exit_true = loopStack.top().bb_end`
À chaque `continue` : `currentBB->exit_true = loopStack.top().bb_cond`

Fonctionne pour les boucles imbriquées : `loopStack` contient le contexte de la boucle la plus interne en haut.

---

## Exemples de code à connaître

### if/else en CFG
```c
if (x > 0) { y = 1; } else { y = -1; }
// Suite...
```
```
bb_entry:
  cmp_gt !tmp0 x_0 0
  → test !tmp0
      ↙ vrai              ↘ faux
bb_then:               bb_else:
  ldconst y_0 1          ldconst y_0 -1
  → exit_true: bb_merge  → exit_true: bb_merge

bb_merge: (suite du code)
```

### while en CFG
```c
while (i < 10) { i = i + 1; }
```
```
bb_entry → bb_cond

bb_cond:
  cmp_lt !tmp0 i_0 10
  → test !tmp0
      ↙ vrai          ↘ faux
bb_body:              bb_end:
  add i_0 i_0 1        (suite)
  → exit_true: bb_cond
```

### for en 4 BasicBlocks
```c
for (int i=0; i<10; i++) { body; }
```
```
bb_init: ldconst i_0 0  →  bb_cond
bb_cond: cmp_lt !tmp i_0 10  →  vrai: bb_body, faux: bb_end
bb_body: {instructions}  →  bb_update
bb_update: add i_0 i_0 1  →  bb_cond
bb_end: (suite)
```

### return avec exit_bb
```c
int f(int x) {
    if (x > 0) return x;    // → copy !retval x; jmp exit_bb
    return -1;              // → ldconst !retval -1; jmp exit_bb
}
```
```
exit_bb:
  movl -4(%rbp), %eax   ; !retval → %eax
  leave
  ret
```
→ Un seul épilogue, peu importe le nombre de `return`.

---

## Questions pièges possibles du jury

**Q : Pourquoi une IR 3 adresses plutôt que directement de l'assembleur ?**
R : L'IR découple le front-end du back-end. Avec une IR, on peut ajouter un backend ARM64 sans toucher au front-end. On peut aussi optimiser l'IR (constant folding, propagation) avant de générer l'asm. L'asm serait trop dépendant des registres physiques pour être manipulable.

**Q : Votre IR est-elle en SSA (Static Single Assignment) ?**
R : Non. Nos variables IR peuvent être réassignées (`!tmp0` peut être écrit plusieurs fois). La SSA simplifierait certaines optimisations mais complexifie la construction de l'IR. C'est une piste d'amélioration future.

**Q : Comment gérez-vous les BasicBlocks qui ne se terminent pas par un branchement (fallthrough) ?**
R : `exit_false == nullptr` et `test_var_name.empty()` → branchement inconditionnel vers `exit_true`. En asm, ça peut être soit un `jmp`, soit un fallthrough naturel si les blocs sont consécutifs.

**Q : Comment determinez-vous l'ordre d'émission des BasicBlocks en asm ?**
R : On part du bloc d'entrée et on fait un parcours dans l'ordre de création (BFS/ordre d'insertion). Les étiquettes garantissent que les `jmp` pointent au bon endroit indépendamment de l'ordre.

**Q : Que se passe-t-il si le corps d'un `if` contient un `return` ?**
R : Le flag `has_return` est mis à `true` sur le BasicBlock `bb_then`. Quand on génère le saut vers `bb_merge` en fin de `bb_then`, on vérifie ce flag : si `has_return == true`, on ne génère pas de saut superflu (le `return` a déjà généré un `jmp exit_bb`).

**Q : Et pour les switch imbriqués dans des boucles ?**
R : `loopStack` ne contient que les contextes de boucles (`while`, `for`, `do-while`). Un `break` dans un `switch` saute à la fin du `switch`, pas de la boucle. On gère ça avec une pile séparée pour les switch, ou en ne poussant pas de contexte loop pour le switch.

**Q : Votre IR gère-t-elle les fonctions variadiques (printf) ?**
R : Non. On supporte uniquement `putchar` et `getchar` pour les I/O, qui sont des fonctions à nombre fixe d'arguments. `printf` nécessiterait une gestion spéciale (ABI différente, registre AL = nb registres XMM utilisés).

---

## Phrases d'accroche pour enchaîner avec P5

> "Vous avez vu comment on construit l'IR et le CFG. Mais avant de générer l'assembleur, on fait quelque chose d'intelligent : on optimise cette IR. P5 va vous expliquer notre système de constant folding et de propagation de constantes."

> "L'IR qu'on vient de décrire est déjà correcte. Mais on peut faire mieux en calculant certaines choses à la compilation plutôt qu'à l'exécution. [P5] vous présente nos optimisations."
