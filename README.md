# ifcc — Compilateur C simplifié

> Compilateur d'un sous-ensemble du langage C vers assembleur x86-64 (System V AMD64 ABI).  
> Projet PLD — INSA Lyon

---

## Architecture du compilateur

```
Code source C  →  ANTLR4 (Lexer/Parser)  →  AST
                                              ↓
                                    SymbolTableVisitor (Passe 1 : vérifications sémantiques)
                                              ↓
                                    IRGenVisitor (Passe 2 : génération de l'IR)
                                              ↓
                                    CFG + BasicBlocks (Représentation Intermédiaire)
                                              ↓
                                    gen_asm() (Passe 3 : génération assembleur x86-64)
                                              ↓
                                    Fichier .s (assembleur)
```

### Fichiers principaux

| Fichier | Rôle |
|---------|------|
| `ifcc.g4` | Grammaire ANTLR4 du langage |
| `main.cpp` | Point d'entrée du compilateur (3 passes) |
| `SymbolTableVisitor.h/.cpp` | Analyse sémantique (variables, fonctions) |
| `IRGenVisitor.h/.cpp` | Génération du Rendu Intermédiaire (IR) |
| `IR.h/.cpp` | Structures IR : `IRInstr`, `BasicBlock`, `CFG` |
| `CodeGenVisitor.h/.cpp` | Ancien générateur de code (conservé, non utilisé par défaut) |

---

## Étapes d'implémentation

### 4.1–4.8 — Bases du compilateur (pré-existant)

Le compilateur de base était déjà en place avant cette session :
- **Grammaire ANTLR4** : parsing d'un programme `int main() { ... }` avec déclarations, affectations, et `return`.
- **Expressions arithmétiques** : `+`, `-`, `*`, parenthèses, moins unaire.
- **Table des symboles** : gestion des variables locales avec détection des variables non déclarées, déjà déclarées, ou inutilisées.
- **Génération d'assembleur** : allocation sur la pile (`-X(%rbp)`), prologue/épilogue.
- **Suite de tests** : 30+ tests couvrant les cas de base.

---

### 4.9 — Propagation de constantes dans les expressions

**Objectif** : Simplifier les expressions à la compilation.

**Implémentation** :
- Introduction d'un `struct ExprValue` (`IRGenVisitor.h`) qui représente le résultat d'une expression : soit une **constante** (`isConstant = true, value = N`), soit une **variable** (`isConstant = false, varName = "x"`).
- Chaque méthode `visitXxxExpr` retourne un `ExprValue`. Si les deux opérandes sont des constantes, le calcul est effectué **à la compilation** (constant folding). Sinon, du code IR est généré.

**Optimisations réalisées** :
| Cas | Résultat |
|-----|----------|
| `2 * 3 + 4 * 5` | → compilé en `movl $26` directement |
| `x + 0` | → simplifié en `x` |
| `x * 1` | → simplifié en `x` |
| `x * 0` | → simplifié en `0` |
| `-constante` | → replié à la compilation |

**Fichiers modifiés** : `IRGenVisitor.h`, `IRGenVisitor.cpp`

---

### 4.10 — Mise en place des appels de fonction

**Objectif** : Supporter les appels de fonctions (jusqu'à 6 arguments) via l'ABI System V AMD64.

**Implémentation** :
- **Grammaire** (`ifcc.g4`) : ajout de la règle `callExpr` dans les expressions : `VAR '(' (expr (',' expr)*)? ')'`.
- **Nouvelle instruction IR** : `IRInstr::call` dans `IR.h`.
- **Génération assembleur** (`IR.cpp`) : les arguments sont placés dans les registres ABI (`%edi`, `%esi`, `%edx`, `%ecx`, `%r8d`, `%r9d`) avant d'émettre l'instruction `call`. Le résultat (`%eax`) est récupéré dans une variable temporaire.
- **Visiteur** (`IRGenVisitor.cpp`) : `visitCallExpr` évalue chaque argument, génère les `loadConst` nécessaires, et émet l'instruction `call`.

**Exemple testé** : `putchar(98)` → affiche `'b'`.

**Fichiers modifiés** : `ifcc.g4`, `IR.h`, `IR.cpp`, `IRGenVisitor.h`, `IRGenVisitor.cpp`

---

### 4.11 — Programmes à plusieurs fonctions et enregistrements d'activation

**Objectif** : Compiler des programmes avec plusieurs fonctions, chacune ayant son propre espace mémoire (stack frame).

**Implémentation** :
- **Grammaire** : `prog : function_def+ EOF` au lieu d'un unique `int main()`. Ajout de la règle `parameters : 'int' VAR (',' 'int' VAR)*`.
- **Architecture multi-CFG** : `IRGenVisitor` maintient un `vector<CFG*> cfgs`. Chaque `function_def` crée un nouveau `CFG` avec son propre `funcName`, sa propre table des symboles, et ses propres BasicBlocks.
- **Récupération des paramètres ABI** : à l'entrée d'une fonction, des instructions `copy` transfèrent les registres ABI (`%edi`, `%esi`...) vers les variables locales sur la pile. Les pseudo-registres `!edi`, `!esi`... sont traduits vers `%edi`, `%esi`... par `IR_reg_to_asm`.
- **Labels dynamiques** : `gen_asm_prologue` utilise `CFG::funcName` pour les labels (`.globl add`, `add:`).
- **Alignement 16 octets** : `subq $X, %rsp` avec `X = ((nextFreeSymbolIndex + 15) & ~15)`.
- **`main.cpp`** : boucle sur `irv.getCFGs()` pour générer le code de chaque fonction.

**Exemple testé** :
```c
int add(int a, int b) { return a + b; }
int addFour(int a, int b, int c, int d) {
    int sum1 = add(a, b);
    int sum2 = add(c, d);
    return add(sum1, sum2);
}
int main() { return addFour(10, 20, 30, 40); }  /* → 100 */
```

**Fichiers modifiés** : `ifcc.g4`, `IR.h`, `IR.cpp`, `IRGenVisitor.h`, `IRGenVisitor.cpp`, `main.cpp`, `SymbolTableVisitor.h`, `SymbolTableVisitor.cpp`, `CodeGenVisitor.h`, `CodeGenVisitor.cpp`

---

### 4.12 — Compiler le if...else

**Objectif** : Supporter les structures conditionnelles et les opérateurs de comparaison.

**Implémentation** :
- **Grammaire** :
  - `block : '{' statement* '}'` — blocs d'instructions entre accolades.
  - `ifStmt : 'if' '(' expr ')' statement ('else' statement)?` — conditionnelles.
  - Opérateurs relationnels (`<`, `>`, `<=`, `>=`) et d'égalité (`==`, `!=`) dans les expressions.
- **Nouvelles instructions IR** : `cmp_neq`, `cmp_gt`, `cmp_ge` ajoutées à l'enum `IRInstr::Operation`, avec génération assembleur (`setne`, `setg`, `setge`).
- **Visiteur `visitIfStmt`** :
  1. Évalue la condition → résultat stocké dans `test_var_name` du BasicBlock courant.
  2. Crée 3 BasicBlocks : `bb_true`, `bb_false` (optionnel), `bb_end`.
  3. Le bloc courant branche : `exit_true → bb_true`, `exit_false → bb_false` (ou `bb_end`).
  4. Après le corps du `if`, saut inconditionnel vers `bb_end`.
- **Génération assembleur des sauts** (déjà dans `BasicBlock::gen_asm`) :
  ```asm
  cmpl $0, <test_var>
  je <exit_false_label>
  jmp <exit_true_label>
  ```
- **Gestion du `has_return`** : si un bloc contient un `return`, il ne doit pas générer de saut vers `bb_end` (pour éviter les boucles infinies en récursion).

**Exemple testé** : Fonction factorielle récursive → `factorial(5) = 120` ✅

**Fichiers modifiés** : `ifcc.g4`, `IR.h`, `IR.cpp`, `IRGenVisitor.h`, `IRGenVisitor.cpp`

---

### 4.13 — Gestion du return n'importe où

**Objectif** : Permettre plusieurs `return` dans une même fonction, avec un seul épilogue.

**Implémentation** :
- **Grammaire** : `return_stmt` retiré de la fin obligatoire de `function_def`. C'est désormais un `statement` comme un autre.
- **Exit BB unique** : chaque `CFG` possède un `exit_bb` (`.LBB_funcName_exit`) créé au début de `visitFunction_def`. C'est le **seul** bloc qui génère l'épilogue (`movl !retval, %eax; leave; ret`).
- **`visitReturn_stmt`** : évalue l'expression, copie dans `!retval`, fait un `jmp` vers `exit_bb`, puis crée un nouveau BB "mort" pour le code après le return.
- **Séparation des compteurs** : `nextBBnumber` (pour les labels de blocs) et `nextTempVarIndex` (pour les variables temporaires) ont été séparés pour éviter des collisions de noms.

**Exemple testé** :
```c
int classify(int x) {
    if (x == 0) { return 0; }
    if (x > 0) { return 1; }
    return 2;
}
```
→ Tous les `return` sautent vers le même épilogue. Résultat identique à GCC.

**Fichiers modifiés** : `ifcc.g4`, `IR.h`, `IR.cpp`, `IRGenVisitor.h`, `IRGenVisitor.cpp`, `CodeGenVisitor.cpp`

---

### 4.14 — Compiler les boucles while

**Objectif** : Supporter la boucle `while`.

**Implémentation** :
- **Grammaire** : `whileStmt : 'while' '(' expr ')' statement ;`
- **Visiteur `visitWhileStmt`** — crée 3 BasicBlocks :
  1. `bb_cond` : évalue la condition, branche vers `bb_body` (vrai) ou `bb_end` (faux).
  2. `bb_body` : exécute le corps, puis **revient à `bb_cond`** (c'est la seule différence avec le `if` !).
  3. `bb_end` : suite du programme.

**Micro-tâche** : comme le dit le sujet, une fois le `if` en place, le `while` est une question de minutes. L'infrastructure des BasicBlocks est identique — seul le saut de retour diffère.

**Exemple testé** : Factorielle itérative → `factorial_iter(5) = 120` ✅

**Fichiers modifiés** : `ifcc.g4`, `IRGenVisitor.h`, `IRGenVisitor.cpp`

---

### 4.15 — Vérifications statiques sur les fonctions

**Objectif** : Détecter les erreurs sémantiques liées aux fonctions.

**Implémentation** dans `SymbolTableVisitor` :
- **Registre des fonctions** : `map<string, int> functionRegistry` stocke chaque fonction définie et son nombre de paramètres.
- **Passe 1** (`visitProg`) : enregistre toutes les fonctions avant de visiter les corps.
- **Passe 2** : visite chaque corps de fonction (vérifie les variables locales).
- **Passe 3** : vérifications globales après toutes les visites.

| Vérification | Type | Détail |
|--|--|--|
| Fonction appelée mais jamais définie | **erreur** | Sauf fonctions externes (`putchar`, `getchar`) |
| Mauvais nombre d'arguments | **erreur** | Compare avec le registre |
| Double définition de fonction | **erreur** | Même nom défini deux fois |
| Fonction définie mais jamais appelée | **warning** | Sauf `main` |

**Fichiers modifiés** : `SymbolTableVisitor.h`, `SymbolTableVisitor.cpp`

---

### 4.16 — Propagation des variables constantes (avec analyse du data-flow)

**Objectif** : Propager les valeurs constantes connues à travers les variables pour déclencher davantage de constant folding en cascade.

**Principe** :
- On maintient une table `constMap` (`map<string, int>`) qui associe chaque variable à sa valeur constante connue pendant la visite de l'AST.
- Lors d'une affectation `x = 5`, on enregistre `constMap["x"] = 5`.
- Lors de la lecture d'une variable (`visitVarExpr`), si elle est dans `constMap`, on retourne directement un `ExprValue` constant au lieu de lire la variable. Cela déclenche du constant folding en cascade avec les optimisations de la tâche 4.9.
- Lors d'une affectation non-constante (`x = f()`), on invalide `constMap["x"]`.

**Analyse data-flow pour if/else et while** :
- **`visitIfStmt`** : on collecte statiquement (via `collectAssignedVars`) toutes les variables modifiées dans les deux branches *avant* de les visiter. Chaque branche part d'une copie de la `constMap` d'avant le `if`. Après le `if`, on restaure l'état d'avant et on invalide toutes les variables potentiellement modifiées (approche conservative).
- **`visitWhileStmt`** : on collecte les variables modifiées dans le corps *avant* de visiter quoi que ce soit, puis on les invalide immédiatement dans la `constMap` (car le corps peut s'exécuter 0 ou N fois, et la condition est réévaluée à chaque itération).
- **`collectAssignedVars`** : fonction utilitaire qui parcourt récursivement un sous-arbre AST pour trouver toutes les déclarations et affectations (via `dynamic_cast` sur `AffectationContext` et `DeclarationContext`).

**Exemple** :
```c
int main() {
    int x = 5;           // constMap: {x: 5}
    int y = x + 3;       // x propagé → 5+3 → replié en 8, constMap: {x: 5, y: 8}
    int z = y * 2;       // y propagé → 8*2 → replié en 16, constMap: {x: 5, y: 8, z: 16}
    return z;             // → movl $16 directement, aucun calcul à l'exécution
}
```

**Bonus** : constant folding étendu aux comparaisons (`visitEqExpr`, `visitRelExpr`) — si les deux opérandes sont des constantes propagées, le résultat de la comparaison est calculé à la compilation.

**Fichiers modifiés** : `IRGenVisitor.h`, `IRGenVisitor.cpp`

---

### 4.17 — Support des flottants et inférence de type

**Objectif** : Ajouter le type `double` au compilateur avec les conversions implicites entre `int` et `double`.

**Implémentation** :

#### Extension de la grammaire (`ifcc.g4`)
- `function_def : type VAR '(' parameters? ')' ...` — le type de retour est désormais `int` ou `double`.
- `parameters : type VAR (',' type VAR)*` — chaque paramètre a un type explicite.
- `declaration : type VAR '=' expr` — idem pour les déclarations.
- `type : 'int' | 'double'` — nouvelle règle de type.
- `CONST_DOUBLE : [0-9]+ '.' [0-9]* | '.' [0-9]+` — nouveau token pour les littéraux flottants (ex: `3.14`, `.5`).
- `expr` : ajout de `constDoubleExpr` et remplacement de `mulExpr` par `mulDivExpr` (ajout de la division `/`).

#### Gestion des types (`type.h`)
- Ajout de `DOUBLE` dans l'enum `Type`.
- `typeSize(Type t)` : retourne 4 pour `INT`, 8 pour `DOUBLE`.
- `promoteType(Type a, Type b)` : retourne `DOUBLE` si l'un des deux est `DOUBLE` (promotion implicite C).

#### Gestion des offsets dans l'AR (`IR.cpp`)
- `add_to_symbol_table` aligne les `double` sur 8 octets avant d'allouer.
- Les `double` occupent 8 octets sur la pile au lieu de 4.

#### Nouvelles instructions IR (`IR.h`, `IR.cpp`)
| Instruction | Assembleur | Description |
|---|---|---|
| `ldconst_double` | `movsd label(%rip), %xmm0` | Charge un double depuis `.rodata` |
| `copy_double` | `movsd src, %xmm0; movsd %xmm0, dest` | Copie un double |
| `add_double` | `addsd` | Addition de doubles |
| `sub_double` | `subsd` | Soustraction de doubles |
| `mul_double` | `mulsd` | Multiplication de doubles |
| `div_double` | `divsd` | Division de doubles |
| `div_int` | `cltd; idivl` | Division entière signée |
| `int_to_double` | `cvtsi2sdl` | Conversion int → double |
| `double_to_int` | `cvttsd2si` | Conversion double → int (troncature) |

#### Constantes double en `.rodata`
Les constantes `double` sont stockées dans la section `.rodata` avec leur représentation binaire IEEE 754 (`.quad`). Chaque CFG maintient un vecteur `doubleConstants` de paires `(label, valeur)`.

#### Inférence de type dans le visiteur (`IRGenVisitor.cpp`)
- `ExprValue` étendu avec `dvalue` (double) et `type` (INT ou DOUBLE).
- **Arithmétique** : `promoteType(left.type, right.type)` détermine le type résultat. Si les types diffèrent, `emitConversion()` insère une instruction `int_to_double` ou `double_to_int`.
- **Déclaration/affectation** : conversion implicite si le type de l'expression ne correspond pas au type de la variable (ex: `int x = 3.14` → troncature, `double y = 5` → promotion).
- **Return** : conversion implicite vers le type de retour de la fonction.
- **Constant folding** : étendu aux doubles (ex: `3.14 * 2.0` → replié en `6.28` à la compilation).
- **Comparaisons** : les opérandes sont promus au type commun avant la comparaison.

**Exemple** :
```c
double circleArea(int r) {
    double pi = 3.14159;
    return pi * r * r;    // r est promu en double implicitement (cvtsi2sdl)
}
int main() {
    int area = circleArea(5);  // le double retourné est tronqué en int
    return area;               // → 78
}
```

**Fichiers modifiés** : `ifcc.g4`, `type.h`, `IR.h`, `IR.cpp`, `IRGenVisitor.h`, `IRGenVisitor.cpp`, `SymbolTableVisitor.h`, `SymbolTableVisitor.cpp`

---

### 4.18 — Affectation à une lvalue quelconque

**Objectif** : Mettre en place une architecture générique pour l'affectation, où le côté gauche (lvalue) est évalué pour produire une **adresse mémoire**, et le côté droit (rvalue) est évalué pour produire une **valeur**. L'affectation revient alors à un simple `wmem(addr, val)`.

**Principe du cours** : le compilateur sépare clairement :
1. **Évaluation de la rvalue** : l'expression à droite du `=` est évaluée normalement, produisant une valeur dans un temporaire.
2. **Évaluation de la lvalue** : l'expression à gauche du `=` est évaluée pour produire l'**adresse** de l'emplacement cible (via `leaq` en x86).
3. **Écriture en mémoire** : `wmem(addr, val)` écrit la valeur à l'adresse calculée.

**Implémentation** :

#### Extension de la grammaire (`ifcc.g4`)
```antlr
affectation : lvalue '=' expr ;
lvalue : VAR ;   // extensible à d'autres formes (tableaux, déréférencements…)
```
La règle `lvalue` est une indirection qui permet d'ajouter facilement d'autres formes de lvalues plus tard (accès tableau, déréférencement de pointeur, etc.).

#### Nouvelles structures (`IRGenVisitor.h`)
- `LvalueResult` : contient `addrVar` (variable temporaire avec l'adresse de la lvalue) et `type` (type de la valeur pointée).

#### Nouvelles instructions IR (`IR.h`, `IR.cpp`)
| Instruction | Assembleur | Description |
|---|---|---|
| `lea` | `leaq src(%rbp), %rax; movq %rax, dest` | Charge l'adresse effective d'une variable dans un temporaire |
| `wmem` | `movq addr, %rax; movl val, %ecx; movl %ecx, (%rax)` | Écrit un int à l'adresse contenue dans le temporaire |
| `wmem_double` | `movq addr, %rax; movsd val, %xmm0; movsd %xmm0, (%rax)` | Écrit un double à l'adresse contenue dans le temporaire |

#### Nouveau type `ADDR` (`type.h`)
Le type `ADDR` (8 octets) est ajouté pour les temporaires contenant des adresses (pointeurs). Il est aligné sur 8 octets comme les doubles.

#### Correction de l'allocation mémoire (`IR.cpp`)
Le passage à des temporaires de 8 octets (adresses) a révélé un bug de chevauchement mémoire dans la convention d'offsets : `movq -I(%rbp)` écrit 8 octets **vers les adresses croissantes** (vers `%rbp`), ce qui pouvait écraser les variables voisines. La correction consiste à stocker dans `SymbolIndex` le bord **supérieur** de l'allocation (côté `%rbp`) :

```cpp
nextFreeSymbolIndex += size;           // réserver l'espace d'abord
SymbolIndex[name] = nextFreeSymbolIndex; // index = bord supérieur
```

#### Visiteur `visitAffectation` (`IRGenVisitor.cpp`)
Le nouveau `visitAffectation` suit le schéma générique :
1. Évaluer la rvalue → `ExprValue` (avec constant folding si possible)
2. Évaluer la lvalue → `LvalueResult` (adresse via `lea`)
3. Conversion implicite de type si nécessaire
4. Matérialiser la rvalue dans un temporaire
5. Émettre `wmem(addr, val)` ou `wmem_double(addr, val)`

**Exemple** : pour `x = x + 1` où `x` est à `-8(%rbp)` :
```asm
leaq -8(%rbp), %rax       # calcul de l'adresse de x
movq %rax, -16(%rbp)       # stocker l'adresse dans un temp
movl -8(%rbp), %eax        # charger x
addl $1, %eax              # x + 1
movl %eax, -20(%rbp)       # stocker le résultat dans un temp
movq -16(%rbp), %rax       # recharger l'adresse
movl -20(%rbp), %ecx       # charger la valeur
movl %ecx, (%rax)          # écrire dans x via l'adresse
```

**Tests** : tous les tests d'affectation existants passent sans régression (déclarations, affectations simples, chaînes d'affectation, swap, while avec affectation, etc.).

**Fichiers modifiés** : `ifcc.g4`, `type.h`, `IR.h`, `IR.cpp`, `IRGenVisitor.h`, `IRGenVisitor.cpp`, `SymbolTableVisitor.cpp`, `CodeGenVisitor.cpp`

---

### 4.19 — Tableaux unidimensionnels

**Objectif** : Permettre la déclaration de tableaux de taille constante et l'accès à leurs éléments.

**Implémentation** :
- **Grammaire** (`ifcc.g4`) :
  - `declaration_array : type VAR '[' CONST ']' ';'`
  - `lvalue_array : VAR '[' expr ']'`
  - `arrayAccessExpr : VAR '[' expr ']'`
- **Gestion Mémoire (`IR.cpp`)** : 
  - Ajout de `add_array_to_symbol_table` qui alloue `$taille_element * nb_elements` sur la pile (aligné sur 8 octets pour les `double`).
- **Évaluation `ArrayAccessExpr` (rvalue)** : 
  1. On évalue l'expression de l'index.
  2. On calcule physiquement l'offset mémoire : `offset_temporaire = index_temporaire * taille_element`.
  3. L'instruction IR `add_addr` décale l'adresse de base du tableau avec cet offset pour obtenir l'adresse de l'élément : `addr_element = addr_base + offset`.
  4. On émet un `rmem` (ou `rmem_double`) pour charger la valeur contenue à cette adresse depuis la pile vers un registre temporaire.
- **Affectation tableau (`LvalueArray`)** : Grâce à la généricité de l'étape 4.18, `visitLvalueArray` évalue l'index, calcule l'adresse en mémoire et génère l'adresse temporaire. L'affectation utilise alors l'instruction de base `wmem`.

**Exemple testé** :
```c
int a[5]; 
a[2] = 42; 
return a[2];  /* Return 42 */
```

**Fichiers modifiés** : `ifcc.g4`, `IRGenVisitor.cpp`, `IR.h`, `IR.cpp`

---

### 4.20 — Appels de fonction ayant plus de 6 arguments

**Objectif** : Respecter pleinement l'ABI System V AMD64 qui spécifie que les 6 premiers arguments passent par les registres et que les suivants sont empilés.

**Implémentation** :
- **Appelant (`IRInstr::call` dans `IR.cpp`)** :
  - Identifie s'il y a plus de 6 arguments.
  - S'il y a un nombre impair d'arguments sur la pile (par exemple le 7ème), l'ABI impose d'aligner `%rsp` sur 16 octets _avant_ le `call`. Le compilateur génère donc un `subq $8, %rsp`.
  - Empile les arguments à partir du 7e, **de la droite vers la gauche**, via la séquence `movslq src, %rax` ; `pushq %rax`.
  - Charge les 6 premiers arguments dans les registres classiques.
  - Exécute l'instruction `call`.
  - Nettoie sa propre pile de paramètres en réajustant le stack pointer : `addq $TotalStackParamsSize, %rsp`.
- **Appelé (`IRGenVisitor::visitFunction_def`)** :
  - Les 6 premiers paramètres sont transférés depuis les registres vers la pile locale (-X(%rbp)).
  - Les paramètres restants (index 6, 7...) utilisent une adresse *positive* basée sur le ressentiment depuis la pile (caller-stack frame). Ils sont identifiés par les pseudo-registres `!param6`, `!param7`, etc.
  - La fonction `IR_reg_to_asm` lit l'identifiant `!paramX` et le traduit statiquement en l'offset mémoire positif correspondant : `16 + (X-6)*8 (%rbp)`.

**Exemple testé** : `my_sum_10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)` testé avec succès (return 55).

**Fichiers modifiés** : `IR.cpp`, `IRGenVisitor.cpp`

---

## Grammaire complète

```antlr
grammar ifcc;

prog : function_def+ EOF ;
function_def : type VAR '(' parameters? ')' '{' statement* '}' ;
parameters : type VAR (',' type VAR)* ;
type : 'int' | 'double' ;

statement : declaration ';' | affectation ';' | expr ';'
          | return_stmt | block | ifStmt | whileStmt ;

block : '{' statement* '}' ;
ifStmt : 'if' '(' expr ')' statement ('else' statement)? ;
whileStmt : 'while' '(' expr ')' statement ;
declaration : type VAR '=' expr ;
affectation : lvalue '=' expr ;
lvalue : VAR ;

expr : '-' expr                                    # unaryMinusExpr
     | expr ('*' | '/') expr                       # mulDivExpr
     | expr ('+' | '-') expr                       # addSubExpr
     | expr ('<' | '>' | '<=' | '>=') expr         # relExpr
     | expr ('==' | '!=') expr                     # eqExpr
     | VAR '(' (expr (',' expr)*)? ')'             # callExpr
     | '(' expr ')'                                # parenExpr
     | CONST_DOUBLE                                # constDoubleExpr
     | CONST                                       # constExpr
     | VAR                                         # varExpr
     ;

return_stmt : RETURN expr ';' ;
CONST_DOUBLE : [0-9]+ '.' [0-9]* | '.' [0-9]+ ;
CONST : [0-9]+ ;
```

---

## Instructions IR supportées

| Instruction | Assembleur généré | Description |
|-------------|-------------------|-------------|
| `ldconst` | `movl $C, dest` | Charger une constante int |
| `ldconst_double` | `movsd label(%rip), dest` | Charger une constante double depuis `.rodata` |
| `copy` | `movl src, %eax; movl %eax, dest` | Copie entre variables int |
| `copy_double` | `movsd src, %xmm0; movsd %xmm0, dest` | Copie entre variables double |
| `add` | `movl + addl` | Addition int |
| `sub` | `movl + subl` | Soustraction int |
| `mul` | `movl + imull` | Multiplication int |
| `div_int` | `cltd + idivl` | Division entière signée |
| `add_double` | `movsd + addsd` | Addition double |
| `sub_double` | `movsd + subsd` | Soustraction double |
| `mul_double` | `movsd + mulsd` | Multiplication double |
| `div_double` | `movsd + divsd` | Division double |
| `int_to_double` | `cvtsi2sdl` | Conversion int → double |
| `double_to_int` | `cvttsd2si` | Conversion double → int (troncature) |
| `cmp_eq` | `cmpl + sete` | Égalité (`==`) |
| `cmp_neq` | `cmpl + setne` | Inégalité (`!=`) |
| `cmp_lt` | `cmpl + setl` | Inférieur (`<`) |
| `cmp_le` | `cmpl + setle` | Inférieur ou égal (`<=`) |
| `cmp_gt` | `cmpl + setg` | Supérieur (`>`) |
| `cmp_ge` | `cmpl + setge` | Supérieur ou égal (`>=`) |
| `call` | `movl args → regs; call func` | Appel de fonction (ABI System V) |
| `lea` | `leaq src(%rbp), %rax; movq %rax, dest` | Charge l'adresse effective d'une variable |
| `wmem` | `movq addr, %rax; movl val, (%rax)` | Écrit un int à l'adresse calculée |
| `wmem_double` | `movq addr, %rax; movsd val, (%rax)` | Écrit un double à l'adresse calculée |

---

## Compilation et exécution

```bash
# Compiler le compilateur
cd compiler
make clean && make

# Compiler un programme C
./ifcc ../testfiles/39_recursive_factorial.c > output.s

# Assembler et exécuter
gcc output.s -o output
./output
echo $?  # affiche le code de retour

# Lancer la suite de tests
cd ../testfiles
./run_tests.sh
```

---

## Tests

La suite de tests (`testfiles/run_tests.sh`) couvre **30 cas** :
- Retour de constantes et variables
- Déclarations et affectations
- Arithmétique complète (+, -, *, parenthèses, moins unaire, priorités)
- Appels de fonctions (putchar, fonctions utilisateur)
- Fonctions multiples avec paramètres
- Récursion (factorielle récursive)
- Boucle while (factorielle itérative)
- Return multiples (classify, abs)
