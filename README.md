# ifcc - Compilateur C simplifie

Compilateur d'un sous-ensemble du langage C vers assembleur x86-64 et ARM64.
Projet PLD, INSA Lyon 4IF.

Le compilateur s'appelle `ifcc`. Il prend un fichier `.c` en entree et produit un fichier assembleur `.s`
qu'on peut ensuite assembler et linker avec GCC. Le langage supporte est un sous-ensemble de C
suffisamment large pour ecrire des vrais programmes (fonctions, boucles, tableaux, doubles...).

On utilise ANTLR4 pour le parsing, et le compilateur est ecrit en C++17.


## Comment ca marche

Le compilateur fait 3 passes sur le code source :

```
Source C  ->  ANTLR4 (lexer + parser)  ->  AST
                                            |
                                   Passe 1 : SymbolTableVisitor
                                   Analyse semantique, verification des types,
                                   detection d'erreurs
                                            |
                                   Passe 2 : IRGenVisitor
                                   Generation de l'IR (code 3 adresses)
                                   + constant folding / propagation
                                            |
                                   Passe 3 : CFG::gen_asm()
                                   IR -> assembleur x86-64 ou ARM64
                                            |
                                        fichier .s
```

La passe 1 verifie que le programme est valide sans generer de code. La passe 2 construit
une representation intermediaire (IR) a base de `BasicBlock` et `CFG` (control flow graph).
La passe 3 traduit chaque instruction IR en assembleur natif. Le choix x86 vs ARM64 se fait
a la passe 3 uniquement - les passes 1 et 2 sont completement independantes de la cible.


## Build

```bash
cd compiler
make
```

Il faut ANTLR4 4.13.2 et un compilo C++17. Les chemins ANTLR sont configures dans `config.mk`
(il y a aussi `config-IF501.mk` et `config-wsl-2025.mk` pour les machines de l'INSA).

Pour compiler un programme :

```bash
./ifcc mon_fichier.c > output.s
gcc output.s -o output
./output
echo $?   # code de retour
```

Pour choisir la cible manuellement :

```bash
./ifcc --target=x86 mon_fichier.c > output.s
./ifcc --target=arm64 mon_fichier.c > output.s
```

Si on ne specifie pas de cible, le compilateur choisit automatiquement en fonction de
l'architecture sur laquelle il a ete compile (via `#if __aarch64__` au moment de la compilation,
pas au runtime). Sur Mac Apple Silicon c'est ARM64 par defaut, sur les machines Linux du CI c'est x86.


## Fonctionnalites

### Types supportes

- `int` : le type de base, 4 octets
- `double` : flottant 64 bits, 8 octets (x86 uniquement - le backend ARM64 ne le supporte pas)
- `char` : traite comme un `int` en interne (4 octets), ca simplifie pas mal de choses
- `void` : pour les fonctions qui ne retournent rien

La promotion implicite `int -> double` est geree automatiquement quand on melange les types
dans une expression. Par exemple `3 + 2.5` fait une conversion du `3` en double avant l'addition.
L'inverse (double -> int) se fait par troncature quand on affecte un double a une variable int.

### Operateurs

Tout ce qu'on utilise couramment en C est la :

**Arithmetique** : `+`, `-`, `*`, `/`, `%`
Ca marche sur les int et les double (sauf `%` qui est int-only, evidemment).

**Comparaisons** : `<`, `>`, `<=`, `>=`, `==`, `!=`
Retournent 0 ou 1, comme en C.

**Logique avec court-circuit** : `&&`, `||`, `!`
Le court-circuit est implemente correctement - si le premier operande de `&&` est faux,
le deuxieme n'est pas evalue (on genere des BasicBlocks separes pour ca).

**Bit-a-bit** : `&`, `|`, `^`, `<<`, `>>`
Le shift droit (`>>`) est arithmetique (signe etendu), comme pour les `int` signes en C.

**Affectation** : `=`, `+=`, `-=`, `*=`, `/=`, `%=`
Les operateurs composes utilisent un helper `emitCompoundAssign` qui factorise la logique.

**Increment/decrement** : `++x`, `--x` (pre, expressions), `x++`, `x--` (post, statements)
Note : les post-increments sont traites comme des statements dans la grammaire pour eviter
des problemes d'explosion d'etats dans le parser ANTLR. Ca veut dire qu'on ne peut pas ecrire
`a = b++` directement, mais `b++; a = b;` marche.

**Ternaire** : `cond ? val_vrai : val_faux`
Associativite droite, avec elimination du code mort si la condition est constante.

### Structures de controle

`if / else` - le classique, avec creation de 3 BasicBlocks (true, false, end).

`while` - une fois qu'on a le `if`, le `while` c'est juste un saut de retour vers la condition
en plus. Comme dit le sujet, c'est une question de minutes une fois l'infrastructure en place.

`do-while` - le corps est execute avant le premier test de condition. Structure en BB :
body -> cond -> (body si vrai | end si faux).

`for` - decompose en 4 blocs : init -> condition -> body -> update -> condition.
Les variables declarees dans l'init (`for (int i = 0; ...)`) ont leur propre scope.

`switch/case` - chaque case genere un BB avec un `cmp_eq`. Le `default` est un bloc de repli.
`break` dans un case saute au bloc de sortie (meme mecanisme que pour les boucles).

`break` et `continue` - on maintient une pile de `LoopContext` dans le visiteur IR.
Chaque boucle empile un contexte avec des pointeurs vers le bloc condition et le bloc end.
`break` saute vers `bb_end`, `continue` vers `bb_cond` (ou `bb_update` pour les `for`).
L'analyse semantique verifie qu'on n'est pas en dehors d'une boucle.

### Fonctions

On supporte les fonctions avec parametres types, la recursion, et la recursion mutuelle.

Les 6 premiers arguments passent par les registres ABI System V (`%edi`, `%esi`, `%edx`,
`%ecx`, `%r8d`, `%r9d` en x86). Au-dela de 6 arguments, les parametres supplementaires
sont empiles de droite a gauche, avec alignement 16 octets du stack pointer avant le `call`.
Cote appele, les parametres 7+ sont accedes via des offsets positifs depuis `%rbp`
(pseudo-registres `!param6`, `!param7`, etc. traduits par `IR_reg_to_asm`).

Fonctions externes reconnues : `putchar` et `getchar`.

Chaque fonction a son propre CFG avec sa propre table des symboles. Un exit_bb unique
par fonction collecte tous les return (pas de duplication de l'epilogue).

### Tableaux

Tableaux 1D a taille fixe, pour `int` et `double`. La declaration `int a[5]` alloue
`5 * 4 = 20` octets sur la pile. L'acces `a[i]` calcule l'adresse de base + offset
via l'instruction IR `add_addr` et fait un `rmem` pour lire ou un `wmem` pour ecrire.

Pas de tableaux multidimensionnels, pas de taille variable.

### Scoping

Les blocs `{ }` creent des scopes imbriques. On peut shadower une variable du scope parent.
Le renommage est gere dans IRGenVisitor via une pile de `scopeStack` : chaque variable
declaree recoit un nom unique (`x_0`, `x_1`, etc.) pour eviter les collisions.
SymbolTableVisitor a sa propre pile de scopes pour les verifications semantiques.

### Return multiples

Plusieurs `return` dans une meme fonction sont supportes. Chaque `return` copie la valeur
dans `!retval` et saute vers le `exit_bb`. Le code apres un `return` dans un bloc est du
code mort (on cree un BB fantome qui ne sera jamais atteint).


## Optimisations

### Constant folding

Chaque methode `visitXxxExpr` retourne un `ExprValue` qui est soit une constante
(`isConstant = true, value = N`), soit une variable temporaire. Si les deux operandes
d'une operation sont des constantes, le calcul est fait a la compilation directement.

Concretement, `2 * 3 + 4 * 5` ne genere aucune instruction arithmetique - juste
un `movl $26` dans le code assembleur.

Voici la couverture precise du constant folding :

| Operation | Types supportes | Exemple |
|-----------|----------------|---------|
| `+`, `-`, `*`, `/` | int, double | `3 * 4` -> `12` |
| `%` | int seulement | `17 % 5` -> `2` |
| `&`, `\|`, `^` | int seulement | `0xFF & 0x0F` -> `0x0F` |
| `<<`, `>>` | int seulement | `1 << 3` -> `8` |
| `==`, `!=`, `<`, `<=`, `>`, `>=` | int seulement (PAS double) | `3 < 5` -> `1` |
| Moins unaire `-` | int, double | `-(5)` -> `-5` |
| NOT logique `!` | int, double | `!0` -> `1` |

**Simplifications algebriques** en plus du folding pur :
- `x + 0` -> `x` (et `0 + x` -> `x`)
- `x * 1` -> `x`
- `x * 0` -> `0`
- `x - 0` -> `x`

**Ce qui n'est PAS folde** :
- L'indexation de tableaux : l'adresse est toujours materialisee
- Les arguments de fonctions : toujours materialises
- Les affectations composees (`+=`, etc.) : pas de folding sur l'operation

**Ternaire et logique** :
- Si la condition du ternaire est constante, seule la branche prise est generee
  (dead code elimination). `1 ? a : b` ne genere que le code pour `a`.
- `0 && x` -> `0` sans evaluer `x`. `1 || x` -> `1` sans evaluer `x`.
  C'est du short-circuit au moment de la compilation.

### Propagation de constantes

En plus du folding, on maintient une `constMap` (`map<string, int>`) pendant la generation IR
qui associe chaque variable a sa valeur constante connue.

Le principe :
- Quand on ecrit `x = 5`, on enregistre `constMap["x"] = 5`
- Quand on lit `x` et qu'il est dans la constMap, on retourne directement la constante
  au lieu de lire la variable. Ca permet au constant folding de se declencher en cascade.
- Quand on fait une affectation non-constante (`x = f()`), on invalide l'entree

Exemple concret :
```c
int x = 5;           // constMap: {x: 5}
int y = x + 3;       // x propage -> 5+3 -> folde en 8, constMap: {x:5, y:8}
int z = y * 2;       // y propage -> 8*2 -> folde en 16
return z;             // -> movl $16 directement, zero calcul a l'execution
```

**Limitations importantes** :
- INT seulement (pas de propagation pour les double)
- Le retour de fonction est toujours traite comme non-constant
- Pas de propagation a travers les appels de fonction

**Analyse de data-flow pour les structures de controle** :

C'est la partie la plus delicate. On ne peut pas juste propager les constantes a travers
un `if/else` ou une boucle sans precautions.

Pour `if/else` : on collecte statiquement (via `collectAssignedVars`) toutes les variables
modifiees dans les deux branches AVANT de les visiter. Chaque branche part d'une copie de
la constMap. Apres le if, on restaure l'etat d'avant et on invalide toutes les variables
potentiellement modifiees. Approche conservative mais sure.

Pour `while`, `for`, `do-while` : on collecte les variables modifiees dans le corps AVANT
d'entrer dans la boucle, et on les invalide immediatement (parce que le corps peut s'executer
0 ou N fois, et la condition est reevaluee a chaque iteration).


## Verifications semantiques (Passe 1)

Le `SymbolTableVisitor` fait ces checks avant toute generation de code :

- Variable utilisee avant declaration -> erreur
- Redeclaration dans le meme scope -> erreur
- Variable declaree mais jamais utilisee -> warning
- Fonction redefinie -> erreur
- Appel avec le mauvais nombre d'arguments -> erreur
- Pas de `main()` -> erreur
- `break` ou `continue` en dehors d'une boucle -> erreur
- Declaration d'une variable de type `void` -> erreur

Les fonctions externes (`putchar`, `getchar`) sont reconnues et ne declenchent pas
d'erreur "fonction non definie". Les fonctions definies mais jamais appelees (sauf `main`)
generent un warning.


## Backend ARM64

Le reciblage ARM64 a ete fait en ajoutant des methodes `gen_asm_arm64` a chaque niveau
(IRInstr, BasicBlock, CFG) qui sont appelees a la place des methodes x86 quand `cfg->target == "arm64"`.

Differences principales avec x86-64 :

| | x86-64 | ARM64 |
|---|--------|-------|
| Frame pointer | `%rbp` | `x29` |
| Link register | empile | `x30` |
| Prologue | `pushq %rbp; movq %rsp, %rbp` | `stp x29, x30, [sp, #-16]!; mov x29, sp` |
| Epilogue | `leave; ret` | `ldp x29, x30, [sp], #16; ret` |
| Load variable | `movl -8(%rbp), %eax` | `ldur w0, [x29, #-8]` |
| Store variable | `movl %eax, -8(%rbp)` | `stur w0, [x29, #-8]` |
| Appel | `call foo` | `bl _foo` |
| Arguments | `%edi`, `%esi`, ... | `w0`, `w1`, ... |
| Retour | `%eax` | `w0` |

Un point chiant avec ARM64 : `ldur`/`stur` n'acceptent pas d'offsets > 255.
Quand ca arrive, on passe par un registre temporaire `x11` :
```asm
sub x11, x29, #320
ldr w8, [x11]
```

**Le backend ARM64 ne supporte PAS le type double.** Quand une instruction double est
rencontree, un commentaire est emis dans l'assembleur mais aucun code n'est genere.
Ca veut dire que les programmes avec des `double` ne compilent correctement qu'en x86.


## Instructions IR

Voila le jeu d'instructions complet de notre IR. C'est du code 3 adresses classique.

| Instruction IR | Assembleur x86 genere | Description |
|---|---|---|
| `ldconst` | `movl $C, dest` | Charge une constante int |
| `ldconst_double` | `movsd label(%rip), dest` | Charge un double depuis `.rodata` |
| `copy` | `movl src, %eax; movl %eax, dest` | Copie int |
| `copy_double` | `movsd src, %xmm0; movsd %xmm0, dest` | Copie double |
| `add` | `movl a; addl b` | Addition int |
| `sub` | `movl a; subl b` | Soustraction int |
| `mul` | `movl a; imull b` | Multiplication int |
| `div_int` | `cltd; idivl` | Division entiere (quotient dans `%eax`) |
| `mod_int` | `cltd; idivl` | Modulo entier (reste dans `%edx`) |
| `add_double` | `addsd` | Addition double |
| `sub_double` | `subsd` | Soustraction double |
| `mul_double` | `mulsd` | Multiplication double |
| `div_double` | `divsd` | Division double |
| `int_to_double` | `cvtsi2sdl` | Conversion int -> double |
| `double_to_int` | `cvttsd2si` | Troncature double -> int |
| `cmp_eq` | `cmpl; sete` | Egalite |
| `cmp_neq` | `cmpl; setne` | Inegalite |
| `cmp_lt` | `cmpl; setl` | Inferieur strict |
| `cmp_le` | `cmpl; setle` | Inferieur ou egal |
| `cmp_gt` | `cmpl; setg` | Superieur strict |
| `cmp_ge` | `cmpl; setge` | Superieur ou egal |
| `bit_and` | `andl` | AND bit-a-bit |
| `bit_xor` | `xorl` | XOR bit-a-bit |
| `bit_or` | `orl` | OR bit-a-bit |
| `shl` | `sall %cl` | Shift left |
| `shr` | `sarl %cl` | Shift right (arithmetique) |
| `logical_not` | `cmpl $0; sete` | NOT logique |
| `call` | `movl args -> regs; call func` | Appel de fonction ABI System V |
| `lea` | `leaq src(%rbp), %rax` | Charge l'adresse effective d'une variable |
| `rmem` | `movq addr; movl (%rax)` | Lit un int depuis une adresse |
| `rmem_double` | `movq addr; movsd (%rax)` | Lit un double depuis une adresse |
| `wmem` | `movq addr; movl val, (%rax)` | Ecrit un int a une adresse |
| `wmem_double` | `movq addr; movsd val, (%rax)` | Ecrit un double a une adresse |
| `add_addr` | `addq` | Arithmetique d'adresse (pour les tableaux) |

Les constantes `double` sont stockees dans `.rodata` en representation IEEE 754 (`.quad`).
Chaque CFG maintient un vecteur de paires (label, valeur) pour ca.


## Architecture des lvalues

Un truc important dans le design : on a separe proprement lvalue et rvalue pour les affectations.

L'idee (tiree du cours) : le cote gauche du `=` est evalue pour produire une **adresse**
(via `lea`), le cote droit est evalue pour produire une **valeur**, et l'affectation se
resume a un `wmem(addr, val)`.

Ca rend le systeme extensible : pour ajouter les tableaux, il a suffi d'implementer
`visitLvalueArray` qui calcule l'adresse de l'element, et le reste du mecanisme d'affectation
marche tout seul. Si on devait ajouter les pointeurs un jour, ce serait le meme principe.

Le type `ADDR` (8 octets) dans `type.h` est la pour les temporaires qui contiennent des
adresses. Attention au stockage memoire : `movq` ecrit 8 octets vers les adresses
croissantes, donc on stocke dans `SymbolIndex` le bord superieur de l'allocation pour
eviter les chevauchements.


## Compatibilite ANTLR

Le passage d'ANTLR 4.9 a 4.10+ a change l'API : l'ancien `antlrcpp::Any` (avec `.as<T>()`)
a ete remplace par `std::any` (avec `std::any_cast<T>()`). Pour que le compilateur marche
sur toutes les versions sans modifier le code, on a un wrapper `castAny<T>` en haut de
`IRGenVisitor.cpp` qui utilise `if constexpr` pour detecter le bon appel. Ca garantit que
le CI GitHub Actions marche quelle que soit la version d'ANTLR installee.


## Tests

150 fichiers de test dans `testfiles/`, couvrant tous les features.

```bash
python3 ifcc-test.py testfiles/
```

Le script compare la sortie de `ifcc` + GCC assembleur avec la compilation directe par GCC.
Pour chaque test, il verifie que le code de retour et la sortie standard sont identiques.

La couverture inclut :
- Retour de constantes et variables
- Declarations, affectations simples, chaines, swap
- Arithmetique complete (+, -, *, /, %, parentheses, priorites, moins unaire)
- Bit-a-bit (&, ^, |, <<, >>)
- Logique (!, &&, ||, court-circuit)
- Comparaisons (<, >, <=, >=, ==, !=)
- Appels de fonctions (putchar, getchar, fonctions utilisateur, >6 args)
- Recursion, recursion mutuelle
- Boucles while, for, do-while (avec break/continue)
- switch/case avec break et default
- Ternaire ? :
- Type double avec conversions implicites
- Tableaux 1D (int et double)
- Scoping et shadowing
- Operateurs composes (+=, -=, *=, /=, %=)
- Increment/decrement (++, --)
- Constantes char ('a', '\n')
- Cas d'erreur (syntaxe invalide, main manquant, redeclaration, break hors boucle, etc.)
- Return multiples dans une meme fonction
- Programme complet (game.c)

Le CI/CD est configure via GitHub Actions.


## Demo : game.c

Un jeu de devinette de nombre qui utilise un peu de tout : fonctions, recursion,
boucle while, if/else, operateurs logiques, arithmetique, convention d'appel ABI.

```bash
compiler/ifcc testfiles/game.c > game.s
gcc game.s -o game
printf '10\n80\n42\n' | ./game
```

```
=== Devinez (1-100) ===
> Trop petit !
> Trop grand !
> Bravo ! Trouve en 3 essais.
```


## Fichiers du projet

| Fichier | Role |
|---------|------|
| `ifcc.g4` | Grammaire ANTLR4 du langage C supporte |
| `main.cpp` | Point d'entree : enchaine les 3 passes |
| `SymbolTableVisitor.h/.cpp` | Passe 1 - analyse semantique, scoping, registre de fonctions |
| `IRGenVisitor.h/.cpp` | Passe 2 - generation IR, constant folding/propagation |
| `IR.h/.cpp` | Structures IR : `IRInstr`, `BasicBlock`, `CFG` + generation assembleur |
| `CodeGenVisitor.h/.cpp` | Ancien generateur de code (conserve mais plus utilise) |
| `type.h` | Enum `Type` (INT, DOUBLE, VOID, ADDR), `typeSize()`, `promoteType()` |
| `symbole.h` | Placeholder (la table des symboles est dans CFG) |
| `config.mk` | Chemins ANTLR et options de compilation |
| `ifcc-test.py` | Script de test (compare ifcc avec GCC) |
| `testfiles/` | 150 programmes de test |
| `testfiles/game.c` | Programme de demo pour la soutenance |


## Limitations

Ce qu'on ne supporte pas :

- Pas de pointeurs ni d'arithmetique de pointeurs
- Pas de strings / `char*`
- Pas de `struct` ni `union`
- Pas de variables globales
- Pas de preprocesseur (les `#include` sont juste ignores)
- Pas de `float` (on a `double` mais pas `float`)
- Pas de cast explicite
- Pas de compilation separee (tout dans un seul fichier)
- Pas de types `unsigned`
- Tableaux : taille fixe seulement, 1D seulement
- ARM64 : pas de support `double`
- Post-increment/decrement : statements seulement, pas expressions
