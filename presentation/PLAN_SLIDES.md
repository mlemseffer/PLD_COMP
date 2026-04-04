# Plan des 90 slides — Soutenance ifcc

> 8 personnes × 7 minutes = 56 minutes | ~1 slide ≈ 45s en moyenne

---

## P1 — Introduction, Architecture, Bilan (slides 1–10)

### Slide 1 — Titre
**Contenu :**
- `ifcc` — Un compilateur C→x86-64 / ARM64
- INSA Lyon 4IF — Projet PLD Compilateur
- Équipe : [noms]
- Date

**P1 | 0:15**

---

### Slide 2 — Le défi
**Contenu :**
- Écrire un compilateur from scratch en C++ + ANTLR4
- Sous-ensemble C → assembleur natif
- Cibles : x86-64 (Linux) **et** ARM64 (Apple Silicon)
- 75/75 tests passent ✓

**P1 | 0:30**

---

### Slide 3 — Bilan en chiffres
**Contenu :**
- **75/75** tests automatisés ✓
- **~40** types d'instructions IR
- **3** passes indépendantes
- **2** backends (x86-64 + ARM64)
- **100%** des features obligatoires + facultatives

**P1 | 0:30**

---

### Slide 4 — Vue d'ensemble : pipeline de compilation
**Contenu :** *(schéma pipeline horizontal)*
```
Code C  →  ANTLR4  →  CST  →  SymbolTableVisitor  →  IRGenVisitor  →  IR/CFG  →  gen_asm()  →  .s
```
- 3 passes clairement séparées
- IR = couche d'abstraction entre front-end et back-end

**P1 | 1:00**

---

### Slide 5 — Passe 1 : Analyse syntaxique (ANTLR4)
**Contenu :**
- `ifcc.g4` : grammaire ANTLR4
- Génère un CST (Concrete Syntax Tree)
- Gestion des priorités opérateurs par la grammaire
- → Détaillé par P2

**P1 | 0:30**

---

### Slide 6 — Passe 2 : Analyse sémantique
**Contenu :**
- `SymbolTableVisitor` : visite le CST
- Vérifications : variable non déclarée, double déclaration, non utilisée
- Construction de la table des symboles
- Renommage des variables pour gérer les scopes
- → Détaillé par P3

**P1 | 0:30**

---

### Slide 7 — Passe 3 : Génération IR + CFG
**Contenu :**
- `IRGenVisitor` : produit des instructions IR 3 adresses
- CFG = Graphe de Flot de Contrôle (un par fonction)
- BasicBlocks reliés par des arêtes conditionnelles
- → Détaillé par P4

**P1 | 0:30**

---

### Slide 8 — Génération de code
**Contenu :**
- `gen_asm()` : IR → assembleur
- Dispatch : x86-64 ou ARM64
- Même IR, deux backends
- Respect de l'ABI System V AMD64 / ARM64 AAPCS
- → Détaillé par P6 et P7

**P1 | 0:30**

---

### Slide 9 — Multi-cible : même IR, deux backends
**Contenu :** *(schéma en T)*
```
        IR (indépendant de la cible)
               ↙           ↘
      x86-64 backend    ARM64 backend
      (Linux/x86)     (Apple Silicon)
```
- Avantage : ajouter un backend = ~500 lignes
- Pas de duplication du front-end

**P1 | 1:00**

---

### Slide 10 — Organisation de la présentation
**Contenu :**
- P1 : Intro + Architecture *(vous y êtes)*
- P2 : Grammaire ANTLR4
- P3 : Analyse sémantique
- P4 : IR & CFG
- P5 : Optimisations
- P6 : Génération code x86-64 & ABI
- P7 : Features avancées & ARM64
- P8 : Tests, démo, conclusion

**P1 | 0:30** *(total P1 ≈ 5:45 + transition)*

---

## P2 — Grammaire ANTLR4 (slides 11–20)

### Slide 11 — ANTLR4 : pourquoi ?
**Contenu :**
- Générateur de parsers LL(*) depuis une grammaire BNF étendue
- Génère C++ : Lexer + Parser + Visitor
- Alternative : écrire un parser récursif descendant à la main
- Notre choix : lisibilité + maintenabilité

**P2 | 0:45**

---

### Slide 12 — Structure du fichier `ifcc.g4`
**Contenu :**
- Règle racine : `prog`
- Fonctions → bloc → instructions → expressions
- Séparation règles lexicales (majuscules) / syntaxiques (minuscules)
- Exemple : `INT_LITERAL`, `IDENTIFIER`

**P2 | 0:45**

---

### Slide 13 — Règle d'expression et priorités
**Contenu :** *(code grammar)*
```antlr
expr
  : expr ('*'|'/'|'%') expr   # mulDivExpr
  | expr ('+'|'-') expr       # addSubExpr
  | expr ('<'|'>'|...) expr   # cmpExpr
  | '-' expr                  # unaryMinusExpr
  | IDENTIFIER '(' args ')'   # callExpr
  | INT_LITERAL               # intExpr
  ;
```
- ANTLR4 : ordre des alternatives = priorité opérateurs

**P2 | 1:00**

---

### Slide 14 — Priorités opérateurs en pratique
**Contenu :** *(tableau)*
| Priorité | Opérateurs |
|----------|-----------|
| 1 (fort) | unaire `-`, `!` |
| 2 | `*`, `/`, `%` |
| 3 | `+`, `-` |
| 4 | `<<`, `>>` |
| 5 | `<`, `>`, `<=`, `>=` |
| 6 | `==`, `!=` |
| 7 | `&`, `^`, `\|` |
| 8 (faible) | `&&`, `\|\|` |

**P2 | 0:45**

---

### Slide 15 — Du code source à l'AST
**Contenu :** *(schéma arbre)*
```c
int f(int x) { return x * 2 + 1; }
```
→ Arbre : `prog → funcDef → bloc → returnStmt → addExpr(mulExpr(x,2), 1)`

**P2 | 1:00**

---

### Slide 16 — Règles de déclaration et d'affectation
**Contenu :**
```antlr
varDecl : type IDENTIFIER ('=' expr)? ';'
        | type IDENTIFIER '[' INT_LITERAL ']' ';'  // tableaux
        ;
affectation : IDENTIFIER '=' expr
            | IDENTIFIER '[' expr ']' '=' expr
            ;
```
- Déclaration avec initialisation optionnelle
- Tableaux intégrés dans la grammaire

**P2 | 0:45**

---

### Slide 17 — Structures de contrôle dans la grammaire
**Contenu :**
```antlr
ifStmt   : 'if' '(' expr ')' bloc ('else if' ... | 'else' bloc)?
whileStmt: 'while' '(' expr ')' bloc
forStmt  : 'for' '(' init ';' expr ';' expr ')' bloc
switchStmt: 'switch' '(' expr ')' '{' caseClause* '}'
```
- Chaque structure → règle dédiée → Visitor method dédiée

**P2 | 0:45**

---

### Slide 18 — Gestion des erreurs syntaxiques
**Contenu :**
- ANTLR4 : error listener par défaut
- Notre implémentation : messages d'erreur vers `stderr`
- Arrêt propre si erreur syntaxique détectée avant passe 2
- Exemple : `error: missing ';'`

**P2 | 0:45**

---

### Slide 19 — Pattern Visitor vs Listener
**Contenu :**
- **Listener** : callbacks entrée/sortie, pas de valeur de retour
- **Visitor** : retourne une valeur, contrôle la descente
- Notre choix : **Visitor** pour pouvoir propager des valeurs (ExprValue, noms de variables IR)
- `castAny<T>` : wrapper pour compatibilité ANTLR 4.9 / 4.10+

**P2 | 0:45**

---

### Slide 20 — Compatibilité ANTLR : wrapper castAny
**Contenu :**
```cpp
template<typename T>
T castAny(antlrcpp::Any val) {
#if ANTLR4_MINOR >= 10
    return std::any_cast<T>(val);
#else
    return val.as<T>();
#endif
}
```
- ANTLR 4.9 : `antlrcpp::Any::as<T>()`
- ANTLR 4.10+ : `std::any_cast<T>()`
- Un seul wrapper → zéro `#ifdef` dans le code métier

**P2 | 0:45** *(total P2 ≈ 7:00)*

---

## P3 — Analyse Sémantique (slides 21–30)

### Slide 21 — Rôle du SymbolTableVisitor
**Contenu :**
- Passe 1 sur le CST (avant la génération IR)
- But : détecter les erreurs sémantiques tôt
- Construit la table des symboles par scope
- N'émet pas d'IR — pure analyse

**P3 | 0:45**

---

### Slide 22 — Table des symboles : structure
**Contenu :**
```
Scope 0 (global)
  └── Scope 1 (fonction f)
        ├── x : int, offset -4
        ├── y : int, offset -8
        └── Scope 2 (if block)
              └── x : int, offset -12  ← shadowing
```
- Pile de scopes (`stack<map<string, SymbolInfo>>`)
- Chaque variable : nom, type, offset sur la pile, utilisée ?

**P3 | 1:00**

---

### Slide 23 — Renommage des variables (scoping)
**Contenu :**
- Problème : deux `x` dans des scopes différents → collision
- Solution : renommer à l'IR
  - `x` en scope 0 → `x_0`
  - `x` en scope 1 → `x_1`
- Garantit l'unicité dans la table CFG
- Implémentation : suffixe `_N` ajouté lors de la résolution

**P3 | 1:00**

---

### Slide 24 — Shadowing : exemple concret
**Contenu :**
```c
int x = 10;
{
    int x = 20;   // x_1 : shadow de x_0
    putchar(x);   // utilise x_1
}
putchar(x);       // utilise x_0
```
→ IR :
```
ldconst int x_0 10
ldconst int x_1 20
call putchar [x_1]
call putchar [x_0]
```

**P3 | 1:00**

---

### Slide 25 — Vérification : variable non déclarée
**Contenu :**
- À chaque usage d'un identifiant → cherche dans la pile de scopes
- Si absent : `error: variable 'x' not declared`
- Arrêt de la compilation (code retour ≠ 0)

**P3 | 0:45**

---

### Slide 26 — Vérification : double déclaration
**Contenu :**
```c
int x = 1;
int x = 2;  // Erreur !
```
- Vérifié dans le scope courant uniquement
- Pas d'erreur si déclaré dans un scope parent (c'est du shadowing)

**P3 | 0:45**

---

### Slide 27 — Vérification : variable déclarée non utilisée
**Contenu :**
- Flag `used` dans `SymbolInfo`
- Mis à `true` lors de chaque lecture
- À la fermeture du scope : si `used == false` → warning

**P3 | 0:45**

---

### Slide 28 — Vérification des appels de fonctions
**Contenu :**
- Table des fonctions déclarées : `map<string, FuncInfo>`
- Vérification : nb d'arguments à l'appel == nb de paramètres déclarés
- Type des arguments (int/double) — conversions implicites signalées

**P3 | 0:45**

---

### Slide 29 — Types supportés
**Contenu :**
| Type | Taille | Stockage |
|------|--------|---------|
| `int` | 32 bits | `movl` |
| `char` | traité comme int | `movl` |
| `void` | — | — |
| `double` | 64 bits | `movsd` + `.rodata` |

- `char` : simplification — pas de sémantique byte séparée
- `double` : registres XMM, instructions SSE2

**P3 | 0:45**

---

### Slide 30 — Transition vers l'IR
**Contenu :**
- Après la passe 1 : CST propre, table des symboles complète
- Les variables ont leurs noms IR définitifs (`x_0`, `x_1`, ...)
- Les offsets sur la pile sont calculés
- → P4 peut générer l'IR sans ambiguïté

**P3 | 0:30** *(total P3 ≈ 7:00)*

---

## P4 — IR & CFG (slides 31–42)

### Slide 31 — Qu'est-ce qu'une Représentation Intermédiaire ?
**Contenu :**
- IR = langage virtuel entre C et assembleur
- Plus simple que l'assembleur (pas de registres physiques)
- Plus proche de la machine que le C (pas de sucre syntaxique)
- Permet d'optimiser et de changer de cible sans retoucher le front-end

**P4 | 0:45**

---

### Slide 32 — Instructions à 3 adresses
**Contenu :**
```
[opération, type, dest, src1, src2]
```
Exemples :
```
add    int  !tmp0  x_0   y_0      →  !tmp0 = x_0 + y_0
ldconst int  x_0   42             →  x_0 = 42
cmp_lt int  !tmp1  a_0   b_0     →  !tmp1 = (a_0 < b_0)
call   int  !tmp2  putchar  [c_0] →  !tmp2 = putchar(c_0)
```

**P4 | 1:00**

---

### Slide 33 — Catalogue des ~40 opérations IR
**Contenu :** *(tableau 2 colonnes)*
| Arithmétique | Mémoire |
|---|---|
| `add`, `sub`, `mul` | `ldconst`, `copy` |
| `div_int`, `mod_int` | `wmem`, `rmem` |
| `neg`, `not_op` | `lea`, `add_addr` |

| Comparaison | Flottant |
|---|---|
| `cmp_eq`, `cmp_neq` | `add_d`, `mul_d` |
| `cmp_lt`, `cmp_le` | `int_to_double` |
| `cmp_gt`, `cmp_ge` | `double_to_int` |

| Contrôle | Autres |
|---|---|
| `call`, `ret` | `lshift`, `rshift` |
| `jmp` (implicite CFG) | `and_op`, `or_op`, `xor_op` |

**P4 | 1:00**

---

### Slide 34 — Variables spéciales IR
**Contenu :**
- `!retval` : valeur de retour de la fonction courante
- `!tmp0`, `!tmp1`, ... : temporaires générés automatiquement
- `!edi`, `!esi`, `!edx`, `!ecx`, `!r8d`, `!r9d` : pseudo-registres ABI (arguments)
- Convention : `!` = variable interne, pas dans le code source

**P4 | 0:45**

---

### Slide 35 — CFG : Graphe de Flot de Contrôle
**Contenu :**
- **Un CFG par fonction**
- Nœuds = BasicBlocks
- Arêtes = sauts conditionnels/inconditionnels
- Chaque BasicBlock : `exit_true`, `exit_false`, `test_var_name`

**P4 | 0:45**

---

### Slide 36 — Structure d'un BasicBlock
**Contenu :**
```cpp
struct BasicBlock {
    vector<IRInstr*> instrs;   // instructions
    BasicBlock* exit_true;     // si test vrai (ou saut inconditionnel)
    BasicBlock* exit_false;    // si test faux
    string test_var_name;      // variable testée pour le saut
    string label;              // nom unique (bb_0, bb_1, ...)
};
```

**P4 | 0:45**

---

### Slide 37 — Bloc de sortie unique `exit_bb`
**Contenu :**
- Tous les `return` sautent vers `exit_bb`
- `exit_bb` contient l'épilogue (un seul)
- Avantage : un seul `leave; ret` même avec plusieurs `return`
- Flag `has_return` : évite un saut superflu après un return

**P4 | 1:00**

---

### Slide 38 — Exemple : `if/else` en CFG
**Contenu :** *(schéma)*
```c
if (x > 0) { y = 1; } else { y = -1; }
```
```
bb_entry: cmp_gt !tmp, x, 0  →  test: !tmp
              ↙ true            ↘ false
       bb_then: y=1         bb_else: y=-1
              ↘                 ↙
              bb_merge (suite)
```

**P4 | 1:00**

---

### Slide 39 — Exemple : `while` en CFG
**Contenu :** *(schéma)*
```c
while (i < 10) { i++; }
```
```
        ↓
    bb_cond: cmp_lt !tmp, i, 10
       ↙ true      ↘ false
  bb_body: i++         bb_end (suite)
       ↑
  (back edge → bb_cond)
```
- `break` → saute à `bb_end`
- `continue` → saute à `bb_cond`

**P4 | 1:00**

---

### Slide 40 — `loopStack` pour break/continue
**Contenu :**
```cpp
struct LoopContext { BasicBlock* bb_cond; BasicBlock* bb_end; };
stack<LoopContext> loopStack;
```
- `break` : `currentBB->exit_true = loopStack.top().bb_end`
- `continue` : `currentBB->exit_true = loopStack.top().bb_cond`
- Fonctionne pour `while`, `for`, `do-while` imbriqués

**P4 | 0:45**

---

### Slide 41 — Exemple `for` : 4 BasicBlocks
**Contenu :**
```c
for (int i=0; i<10; i++) { ... }
```
```
bb_init (i=0) → bb_cond (i<10) → bb_body → bb_update (i++) → bb_cond
                        ↘ false
                       bb_end
```
- Séparation explicite init / cond / body / update

**P4 | 0:45**

---

### Slide 42 — De l'IR à l'assembleur
**Contenu :**
- `CFG::gen_asm(ostream&)` : itère sur les BasicBlocks dans l'ordre
- `IRInstr::gen_asm()` : dispatch selon la cible
  ```cpp
  if (target == "arm64") gen_asm_arm64(o);
  else gen_asm_x86(o);
  ```
- Chaque instruction IR → 1 à 5 instructions asm
- → Détaillé par P6 (x86-64) et P7 (ARM64)

**P4 | 0:45** *(total P4 ≈ 9:30 — légèrement long, ajuster le rythme)*

---

## P5 — Optimisations (slides 43–54)

### Slide 43 — Vue d'ensemble des optimisations
**Contenu :**
- Réalisées **pendant** la génération IR (pas de passe séparée)
- 1. **Constant folding** : calcul compile-time des expressions constantes
- 2. **Propagation de constantes** : suivi des variables à valeur connue
- Structure clé : `ExprValue`

**P5 | 0:45**

---

### Slide 44 — Structure `ExprValue`
**Contenu :**
```cpp
struct ExprValue {
    bool isConstant;   // la valeur est connue à la compilation ?
    int value;         // valeur entière (si isConstant)
    double dvalue;     // valeur double (si isConstant)
    string varName;    // nom de la variable IR (si !isConstant)
    Type type;         // INT ou DOUBLE
};
```
- Retourné par chaque méthode `visitExpr*`
- Permet de décider au moment de la visite

**P5 | 1:00**

---

### Slide 45 — Constant Folding : principe
**Contenu :**
```c
int x = 2 + 3 * 4;
```
→ Sans optimisation :
```
ldconst !tmp0 2
ldconst !tmp1 3
ldconst !tmp2 4
mul !tmp3 !tmp1 !tmp2   // 3*4
add !tmp4 !tmp0 !tmp3   // 2+12
```
→ Avec constant folding :
```
ldconst x_0 14          // calculé à la compilation !
```

**P5 | 1:00**

---

### Slide 46 — Constant Folding : implémentation
**Contenu :**
```cpp
ExprValue visitAddExpr(ctx) {
    auto lhs = visit(ctx->expr(0));
    auto rhs = visit(ctx->expr(1));
    if (lhs.isConstant && rhs.isConstant)
        return ExprValue{true, lhs.value + rhs.value};
    // sinon : émettre une instruction IR add
    ...
}
```
- Récursif : fonctionne pour toute profondeur d'expression

**P5 | 1:00**

---

### Slide 47 — Propagation de constantes : `constMap`
**Contenu :**
```cpp
map<string, int> constMap;  // varName → valeur constante connue
```
```c
int x = 5;     // constMap["x_0"] = 5
int y = x + 1; // lhs.isConstant=true (valeur=5) → folding → y_0=6
```
- Si une variable est dans `constMap` → traitée comme constante
- Mis à jour à chaque affectation de valeur constante

**P5 | 1:00**

---

### Slide 48 — Propagation dans les branches (if)
**Contenu :**
- Problème : si `x = val` dans la branche `then` ET `else` → ok
- Si seulement dans une branche → on ne peut pas propager après
- Solution : `collectAssignedVars()` sur chaque branche
- Invalide dans `constMap` les variables modifiées dans n'importe quelle branche

**P5 | 1:00**

---

### Slide 49 — Propagation dans les boucles (while)
**Contenu :**
```c
int x = 0;
while (x < 10) { x = x + 1; }
```
- `x` modifié dans le corps → invalidé dans `constMap` avant d'entrer
- `collectAssignedVars(whileBody)` : parcourt récursivement, retourne `{x}`
- → `constMap.erase("x_0")`
- Conservatif : pas de faux positifs

**P5 | 1:00**

---

### Slide 50 — `collectAssignedVars()` en détail
**Contenu :**
```cpp
set<string> collectAssignedVars(tree::ParseTree* node) {
    // si nœud = affectation → ajouter la variable
    // sinon → récurser sur les enfants
    // union des résultats
}
```
- Traversée de l'AST sans effet de bord
- Utilisée avant if, while, for, do-while

**P5 | 0:45**

---

### Slide 51 — Court-circuit `&&` : implémentation
**Contenu :**
```c
if (a != 0 && b/a > 1) { ... }
```
```
bb_left:  cmp_neq !tmp0, a, 0  →  test !tmp0
              ↙ true      ↘ false
bb_right: b/a>1            bb_false (résultat=0)
              ↓
          bb_true (résultat=1)
```
- `b/a` n'est évalué que si `a != 0` → pas de division par zéro

**P5 | 1:00**

---

### Slide 52 — Court-circuit `||` : implémentation
**Contenu :**
```c
if (a == 0 || b > 0) { ... }
```
```
bb_left: cmp_eq !tmp0, a, 0  →  test !tmp0
            ↙ true (déjà vrai)    ↘ false
       bb_true                  bb_right: b>0
                                     ↓
                                bb_true / bb_false
```
- Si gauche est vraie → on court-circuite

**P5 | 0:45**

---

### Slide 53 — Résultats des optimisations
**Contenu :** *(tableau comparatif)*
| Programme | Sans optim | Avec optim |
|-----------|-----------|-----------|
| `x = 2+3*4` | 6 instructions | 1 instruction |
| `if (1 < 2)` | 3 instr | 0 instr (branch supprimée) |
| `for (i=0;i<N;i++)` avec N constante | N évaluations | N évaluations (constMap) |

**P5 | 0:45**

---

### Slide 54 — Limites et pistes d'amélioration
**Contenu :**
- Pas d'allocation de registres (tout en mémoire)
- Pas d'élimination de code mort (DCE)
- Pas d'inlining de fonctions
- → Pistes pour aller plus loin (cf. conclusion P8)

**P5 | 0:30** *(total P5 ≈ 10:30 — accélérer sur certains slides)*

---

## P6 — ABI & Génération x86-64 (slides 55–67)

### Slide 55 — Rappel : qu'est-ce que l'ABI ?
**Contenu :**
- **ABI** = Application Binary Interface
- Contrat entre compilateur et OS / bibliothèques
- Définit : passage des arguments, valeur de retour, registres sauvegardés, alignement de pile
- Notre ABI : **System V AMD64** (Linux x86-64)

**P6 | 0:45**

---

### Slide 56 — Registres x86-64 utilisés
**Contenu :** *(tableau)*
| Usage | Registres |
|-------|----------|
| Args entiers (1–6) | `%edi`, `%esi`, `%edx`, `%ecx`, `%r8d`, `%r9d` |
| Args 7+ | pile (droite→gauche) |
| Retour int | `%eax` |
| Retour double | `%xmm0` |
| Frame pointer | `%rbp` |
| Stack pointer | `%rsp` |
| Temporaire scratch | `%eax`, `%edx`, `%r10d`, `%r11d` |

**P6 | 0:45**

---

### Slide 57 — Prologue de fonction
**Contenu :**
```asm
pushq  %rbp
movq   %rsp, %rbp
subq   $N, %rsp         ; N = taille des variables locales (aligné 16)
movl   $0, -4(%rbp)    ; !retval = 0 (C99 : main sans return → 0)
; Copie des args depuis registres vers pile
movl   %edi, -8(%rbp)  ; param1
movl   %esi, -12(%rbp) ; param2
```

**P6 | 1:00**

---

### Slide 58 — Épilogue de fonction
**Contenu :**
```asm
; Dans exit_bb :
movl   -4(%rbp), %eax  ; charge !retval dans %eax
leave                   ; movq %rbp,%rsp + popq %rbp
ret
```
- **Un seul épilogue** grâce à `exit_bb`
- `leave` = restauration frame en une instruction

**P6 | 0:45**

---

### Slide 59 — Alignement de pile à 16 octets
**Contenu :**
- SysV AMD64 : pile doit être alignée à 16 octets avant `call`
- `pushq %rbp` décale de 8 → `subq $N` doit compenser
- N calculé : arrondir au multiple de 16 supérieur
- Exemple : 3 variables int (12 oct) → `subq $16, %rsp`

**P6 | 0:45**

---

### Slide 60 — Exemple : appel de fonction avec arguments
**Contenu :**
```c
int add(int a, int b) { return a + b; }
int main() { return add(3, 4); }
```
```asm
main:
    movl $3, %edi      ; arg1
    movl $4, %esi      ; arg2
    call add
    movl %eax, -4(%rbp) ; !retval = résultat
```

**P6 | 1:00**

---

### Slide 61 — Variables en mémoire (pas de registres)
**Contenu :**
- Toutes les variables locales : sur la pile (`-N(%rbp)`)
- Chaque accès = load depuis la pile dans `%eax`, opération, store
- Volontairement simple : pas d'allocation de registres
- Exemple :
  ```asm
  movl -8(%rbp), %eax   ; charge x
  addl -12(%rbp), %eax  ; + y
  movl %eax, -16(%rbp)  ; → z
  ```

**P6 | 0:45**

---

### Slide 62 — Exemple complet : Fibonacci (source C)
**Contenu :**
```c
int fib(int n) {
    if (n <= 1) return n;
    return fib(n-1) + fib(n-2);
}
```

**P6 | 0:30**

---

### Slide 63 — Fibonacci : IR généré
**Contenu :**
```
bb_entry:
  cmp_le !tmp0 n_0 1
  → test !tmp0
      ↙ true            ↘ false
bb_then:               bb_else:
  copy !retval n_0       sub !tmp1 n_0 1
  jmp exit_bb            call fib [!tmp1] → !tmp2
                         sub !tmp3 n_0 2
                         call fib [!tmp3] → !tmp4
                         add !retval !tmp2 !tmp4
exit_bb:
  movl !retval → %eax
```

**P6 | 1:00**

---

### Slide 64 — Fibonacci : assembleur généré
**Contenu :**
```asm
fib:
    pushq %rbp
    movq  %rsp, %rbp
    subq  $32, %rsp
    movl  $0, -4(%rbp)
    movl  %edi, -8(%rbp)    ; n
    movl  -8(%rbp), %eax
    cmpl  $1, %eax
    setle %al
    movzbl %al, %eax
    movl  %eax, -12(%rbp)  ; !tmp0
    testl %eax, %eax
    je    .bb_else
.bb_then:
    movl -8(%rbp), %eax
    movl %eax, -4(%rbp)    ; !retval = n
    jmp  .exit_bb
.bb_else:
    ...
.exit_bb:
    movl -4(%rbp), %eax
    leave
    ret
```

**P6 | 1:00**

---

### Slide 65 — Gestion des doubles : registres XMM
**Contenu :**
- `double` : registres `%xmm0`–`%xmm7`
- Instructions SSE2 : `movsd`, `addsd`, `subsd`, `mulsd`, `divsd`
- Constantes : stockées en `.rodata` avec label unique
  ```asm
  .rodata
  .LC0: .double 3.14
  ...
  movsd .LC0(%rip), %xmm0
  ```

**P6 | 0:45**

---

### Slide 66 — Conversion int ↔ double
**Contenu :**
```c
double d = 42;    // int_to_double
int i = d + 0.5;  // double_to_int après addition
```
```asm
; int → double
cvtsi2sdl -8(%rbp), %xmm0
; double → int
cvttsd2si %xmm0, %eax
```
- `cvtsi2sdl` : int 32b → double
- `cvttsd2si` : double → int (troncature vers zéro)

**P6 | 1:00**

---

### Slide 67 — Bilan génération x86-64
**Contenu :**
- Toutes les variables en mémoire → code simple mais correct
- Respect strict de l'ABI SysV AMD64
- Un seul épilogue par fonction → propre
- 75/75 tests passent, y compris les cas ABI complexes (7 args, double, récursion)

**P6 | 0:30** *(total P6 ≈ 9:30)*

---

## P7 — Features Avancées & ARM64 (slides 68–82)

### Slide 68 — Plan de P7
**Contenu :**
- `double` et inférence de type *(déjà évoqué par P6, exemples ici)*
- Tableaux 1D
- `switch/case`
- `for`, `do-while`
- Opérateur ternaire `? :`
- ARM64 : backend complet

**P7 | 0:30**

---

### Slide 69 — Type `double` : déclaration et opérations
**Contenu :**
```c
double pi = 3.14159;
double r = 5.0;
double area = pi * r * r;
```
- Constantes → `.rodata`, chargées via `%rip`-relative
- Opérations : `add_d`, `sub_d`, `mul_d`, `div_d` en IR
- → `addsd`, `mulsd`, etc. en asm

**P7 | 0:45**

---

### Slide 70 — Inférence de type : int ↔ double
**Contenu :**
```c
double d = 3;      // int 3 → double : insert int_to_double
int i = 2.7 + 1;  // double 3.7 → int : insert double_to_int
```
- L'IRGenVisitor détecte la mismatch de types
- Insère automatiquement l'instruction de conversion dans l'IR
- Règle : `int op double` → promote int vers double

**P7 | 0:45**

---

### Slide 71 — Tableaux 1D : déclaration
**Contenu :**
```c
int arr[5];
arr[2] = 42;
int x = arr[2];
```
IR généré :
```
lea arr_0 -24(%rbp)   ; adresse base du tableau
add_addr !tmp0 arr_0 2 ; adresse de arr[2]
wmem !tmp0 42          ; écriture
rmem x_0 !tmp0         ; lecture
```
- `leaq` + calcul d'offset → accès mémoire indirect

**P7 | 1:00**

---

### Slide 72 — Tableaux : implémentation IR
**Contenu :**
- `lea` : charge l'adresse base → pseudo-registre `arr_0`
- `add_addr` : `base + index * sizeof(type)` → adresse effective
- `wmem` : write memory (store)
- `rmem` : read memory (load)
- Asm x86-64 : `leaq`, `movl (%rax,%rdx,4), %eax`

**P7 | 0:45**

---

### Slide 73 — `switch/case` : implémentation
**Contenu :**
```c
switch (x) {
  case 1: ...; break;
  case 2: ...; break;
  default: ...;
}
```
CFG :
```
bb_switch: test x
  → cmp_eq x,1 → bb_case1
  → cmp_eq x,2 → bb_case2
  → bb_default
```
- Chaque `case` = comparaison chaînée
- `break` = `jmp bb_end`
- Pas de jump table (suffisant pour notre sous-ensemble)

**P7 | 1:00**

---

### Slide 74 — `for` et `do-while`
**Contenu :**
**`for`** : déjà vu slide 41 (4 blocs)

**`do-while`** :
```c
do { body; } while (cond);
```
CFG :
```
bb_body → bb_cond → (vrai) → bb_body
                  → (faux) → bb_end
```
- Corps exécuté au moins une fois
- `break`/`continue` : même `loopStack`

**P7 | 0:45**

---

### Slide 75 — Opérateur ternaire `? :`
**Contenu :**
```c
int x = (a > 0) ? a : -a;
```
CFG :
```
bb_cond: a>0 ?
    ↙ true           ↘ false
bb_then: !tmp=a    bb_else: !tmp=-a
    ↘                   ↙
    bb_merge: x_0 = !tmp
```
- Résultat stocké dans un temporaire commun `!tmp`
- → Fusion dans `bb_merge`

**P7 | 0:45**

---

### Slide 76 — `++`, `--`, `+=`, `-=` etc.
**Contenu :**
| Opérateur | IR généré |
|-----------|----------|
| `x++` (post) | `copy !tmp x; add x x 1` → retourne `!tmp` |
| `++x` (pre) | `add x x 1` → retourne `x` |
| `x += y` | `add x x y` |
| `x *= 3` | `mul x x 3` (avec folding si 3 est constant) |

**P7 | 0:45**

---

### Slide 77 — ARM64 : pourquoi ?
**Contenu :**
- Apple Silicon (M1/M2/M3) = ARM64
- Même IR → seul le backend change
- Architecture RISC : registres plus nombreux, instructions régulières
- Défi : ABI différente (AAPCS64), instructions différentes

**P7 | 0:45**

---

### Slide 78 — ARM64 : registres et ABI
**Contenu :**
| Rôle | ARM64 | x86-64 |
|------|-------|--------|
| Frame pointer | `x29` | `%rbp` |
| Link register | `x30` | *(sur la pile)* |
| Args 1–8 | `w0`–`w7` | `%edi`–`%r9d` |
| Retour | `w0` | `%eax` |
| Scratch | `w8`–`w11` | `%eax`, `%r10d` |

**P7 | 0:45**

---

### Slide 79 — ARM64 : prologue/épilogue
**Contenu :**
```asm
; Prologue
stp x29, x30, [sp, #-16]!  ; sauvegarde fp + lr
mov x29, sp
sub sp, sp, #N              ; espace variables locales

; Épilogue
mov sp, x29
ldp x29, x30, [sp], #16    ; restaure fp + lr
ret
```
- `stp`/`ldp` : store/load pair (efficace)
- `x30` = link register (adresse de retour)

**P7 | 1:00**

---

### Slide 80 — ARM64 : accès aux variables locales
**Contenu :**
```asm
; Offset < 256 : instruction directe
ldur w8, [x29, #-8]    ; charge variable à offset -8

; Offset > 255 : via registre intermédiaire
mov x11, #-260
ldur w8, [x29, x11]    ; via x11
```
- Limitation ARM64 : offset signed sur 9 bits pour `ldur`
- Notre compilateur gère les deux cas

**P7 | 0:45**

---

### Slide 81 — ARM64 : dispatch dans gen_asm
**Contenu :**
```cpp
void IRInstr::gen_asm(ostream& o) {
    if (cfg->target == "arm64")
        gen_asm_arm64(o);
    else
        gen_asm_x86(o);
}
```
- Ajout d'un backend = implémenter `gen_asm_arm64()` pour chaque opération
- ~500 lignes pour ARM64 vs ~600 pour x86-64
- Aucun changement au front-end

**P7 | 0:45**

---

### Slide 82 — Bilan P7
**Contenu :**
- Double + inférence : code C mixte int/double ✓
- Tableaux : accès indexé via adresse ✓
- switch, for, do-while, ternaire : structures de contrôle complètes ✓
- ARM64 : même programme, deux cibles, zéro duplication front-end ✓

**P7 | 0:30** *(total P7 ≈ 10:30 — aller vite sur les slides bilan)*

---

## P8 — Tests, Démo, Conclusion (slides 83–92)

### Slide 83 — La suite de tests
**Contenu :**
- `python3 ifcc-test.py testfiles/`
- Principe : compiler avec GCC **et** ifcc → comparer codes de sortie + stdout
- **75 tests** couvrant toutes les features
- Automatisé : CI/CD GitHub Actions

**P8 | 0:45**

---

### Slide 84 — 75/75 : graphique de progression
**Contenu :** *(graphique barres ou timeline)*
- Semaine 1 : 15/75 (bases int, +, -)
- Semaine 2 : 35/75 (if, while, fonctions)
- Semaine 3 : 55/75 (double, tableaux, switch)
- Semaine 4 : 75/75 (ARM64, optimisations, bug fixes)
- Dernier bug : `5_no_return` — `!retval` non initialisé → fix : `movl $0, -4(%rbp)` dans le prologue

**P8 | 1:00**

---

### Slide 85 — Catégories de tests
**Contenu :**
| Catégorie | Nb | Exemples |
|-----------|-----|---------|
| Expressions | 12 | arithmétique, comparaisons, bitwise |
| Contrôle | 15 | if/else, while, for, switch |
| Fonctions | 10 | récursion, args multiples, void |
| Types | 8 | double, char, conversions |
| Features avancées | 20 | tableaux, ternaire, ++ |
| Edge cases | 10 | no_return, shadowing, break imbriqué |

**P8 | 0:45**

---

### Slide 86 — CI/CD : GitHub Actions
**Contenu :**
- Pipeline déclenché sur chaque push
- Build : `make` avec ANTLR4 + g++
- Tests : `python3 ifcc-test.py testfiles/`
- Badge vert = 75/75 ✓
- Feedback immédiat sur les régressions

**P8 | 0:45**

---

### Slide 87 — Démo : `game.c` — présentation
**Contenu :**
- Jeu de devinette : le programme choisit un nombre, le joueur devine
- Features utilisées : `while`, fonctions, `&&` court-circuit, `if/else`, I/O `putchar`/`getchar`
- Compilé **uniquement avec ifcc** (pas GCC)
- Input scripté : `printf '10\n80\n42\n' | ./game`

**P8 | 0:45**

---

### Slide 88 — Démo : script exact
**Contenu :**
```bash
# Compilation
./ifcc game.c -o game.s
as game.s -o game.o
gcc game.o -o game -nostdlib -lc  # ou avec notre runtime

# Lancement (input scripté)
printf '10\n80\n42\n' | ./game

# Sortie attendue :
# Trop petit !
# Trop grand !
# Bravo ! Trouvé en 3 essais.
```

**P8 | 1:00**

---

### Slide 89 — Ce qu'on n'a pas fait (et pourquoi)
**Contenu :**
| Feature | Raison |
|---------|--------|
| Pointeurs | Arithmétique de pointeurs complexe, hors scope |
| Variables globales | Segment `.data`, linkage — hors scope |
| Tableaux multidimensionnels | Extension de tableaux 1D — temps |
| Allocation de registres | Coloration de graphe — complexité |
| Inlining | Nécessite analyse inter-proc |
| Chaînes de caractères | Nécessite `.rodata` + pointeurs |

**P8 | 0:45**

---

### Slide 90 — Améliorations futures
**Contenu :**
- **Allocation de registres** : algorithme de coloration de graphe → moins de mémoire
- **Dead Code Elimination** : supprimer les BasicBlocks non atteignables
- **SSA form** : représentation statique à affectation unique → optimisations plus puissantes
- **LLVM backend** : utiliser l'IR LLVM comme cible intermédiaire

**P8 | 0:45**

---

### Slide 91 — Bilan du projet
**Contenu :**
- **Architecture propre** : 3 passes indépendantes, IR découplée
- **Multi-cible** : x86-64 + ARM64, même front-end
- **100%** des features obligatoires ET facultatives
- **75/75** tests automatisés
- **Optimisations** : constant folding + propagation de constantes

**P8 | 0:45**

---

### Slide 92 — Merci / Questions
**Contenu :**
- Merci pour votre attention
- Code source disponible sur [dépôt Git]
- Questions ?

*(slide de fond pendant les questions)*

**P8 | 0:15** *(total P8 ≈ 7:30)*

---

## Récapitulatif timing

| Présentateur | Slides | Durée estimée |
|---|---|---|
| P1 | 1–10 | ~7:00 |
| P2 | 11–20 | ~7:00 |
| P3 | 21–30 | ~7:00 |
| P4 | 31–42 | ~7:00 |
| P5 | 43–54 | ~7:00 |
| P6 | 55–67 | ~7:00 |
| P7 | 68–82 | ~7:00 |
| P8 | 83–92 | ~7:00 |
| **Total** | **92 slides** | **~56:00** |
