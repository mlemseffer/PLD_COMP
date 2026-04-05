# Manuel Utilisateur - ifcc

## 1. Presentation

ifcc est un compilateur pour un sous-ensemble du langage C. Il prend en entree un fichier source `.c` et produit de l'assembleur x86-64 ou ARM64 sur la sortie standard.

Le compilateur est construit avec ANTLR4 pour le parsing et C++17 pour le reste de la chaine de compilation. L'architecture interne repose sur une representation intermediaire (IR) sous forme de Control Flow Graphs (CFG), ce qui permet de generer du code pour differentes architectures cibles a partir du meme IR.

Ce projet a ete realise dans le cadre du PLD Compilateur, INSA Lyon, departement 4IF.

## 2. Installation et compilation du compilateur

### Pre-requis

- g++ avec support C++17
- Java JDK (necessaire pour ANTLR4, qui genere le lexer/parser)
- ANTLR 4.13.2 : le fichier JAR + le runtime C++
- Python 3 (pour lancer les tests)
- cmake, make

### Etapes de compilation

```bash
cd compiler
cp config-IF501.mk config.mk   # ou config-wsl-2025.mk selon votre machine
make clean && make
```

Le fichier `config.mk` doit definir trois chemins :

- `ANTLRJAR` : chemin vers le fichier `antlr-4.13.2-complete.jar`
- `ANTLRINC` : chemin vers les headers du runtime C++ ANTLR4
- `ANTLRLIB` : chemin vers la librairie du runtime C++ ANTLR4

Si vous etes sur les machines de l'IF, le fichier `config-IF501.mk` devrait fonctionner directement. Sinon, copiez-le et adaptez les chemins a votre installation.

Le `make` va d'abord appeler ANTLR4 pour generer le lexer et le parser C++ a partir de la grammaire `ifcc.g4`, puis compiler le tout. L'executable `ifcc` est produit dans le dossier `compiler/`.

### Visualisation de l'arbre de parsing

Pour debugger la grammaire, on peut afficher l'arbre de parsing dans une fenetre graphique :

```bash
make gui FILE=../testfiles/1_return42.c
```

Cela necessite un environnement graphique (X11 ou equivalent).

## 3. Utilisation

### Usage de base

```bash
./compiler/ifcc source.c > output.s
gcc output.s -o output
./output
echo $?    # affiche le code de retour
```

ifcc ecrit l'assembleur genere sur stdout. On redirige dans un fichier `.s`, puis on utilise gcc (ou as + ld) pour assembler et linker. Le code de retour du programme est la valeur retournee par `main()`.

### Selection de l'architecture cible

```bash
./compiler/ifcc --target=x86 source.c > output.s
./compiler/ifcc --target=arm64 source.c > output.s
```

Par defaut, l'architecture est detectee automatiquement a la compilation du compilateur :

- Sur Mac Apple Silicon (aarch64) : arm64
- Sur x86 Linux : x86

On peut forcer la cible avec `--target=x86` ou `--target=arm64`.

### Format de la ligne de commande

```
ifcc [--target=x86|arm64] chemin/vers/fichier.c
```

Si aucun fichier n'est donne, ifcc affiche un message d'usage et quitte avec le code 1.

## 4. Langage supporte

### 4.1 Types

| Type | Taille | Description |
|------|--------|-------------|
| `int` | 4 octets | Entier signe 32 bits |
| `double` | 8 octets | Flottant IEEE 754 64 bits (x86 seulement) |
| `char` | 4 octets | Traite comme un int en interne |
| `void` | - | Type de retour uniquement, pas de variables void |

Note : `char` est stocke sur 4 octets en interne, comme un `int`. C'est un choix de simplification.

### 4.2 Variables

**Declaration avec initialisation :**
```c
int x = 5;
double z = 3.14;
char c = 'a';
```

**Declaration sans initialisation :**
```c
int x;
```

Les variables non initialisees sont mises a 0 par defaut (ce qui est utile notamment pour la valeur de retour de main).

**Portee (scoping) :**

Les blocs `{ }` creent de nouvelles portees. Une variable declaree dans un bloc interne masque (shadowing) une variable de meme nom dans un bloc externe :

```c
int x = 1;
{
    int x = 2;  // OK, masque le x exterieur
    // ici x vaut 2
}
// ici x vaut 1
```

### 4.3 Tableaux

**Declaration :**
```c
int a[10];
double d[5];   // fonctionne sur x86
```

**Acces :**
```c
a[0] = 42;
int val = a[i + 1];
```

Limitations des tableaux :

- Taille fixe uniquement (pas de VLA)
- Pas de tableaux multi-dimensionnels
- Pas d'initialisation a la declaration (`int a[3] = {1,2,3}` n'est pas supporte)

### 4.4 Operateurs

Les operateurs sont listes par ordre de priorite decroissante (le plus prioritaire en premier).

| Priorite | Operateur | Description | Associativite |
|----------|-----------|-------------|---------------|
| 1 | `()` | Parentheses / appel de fonction | gauche a droite |
| 1 | `[]` | Acces tableau | gauche a droite |
| 2 | `++` `--` (prefixe) | Pre-increment, pre-decrement | droite a gauche |
| 2 | `-` (unaire) | Negation | droite a gauche |
| 2 | `!` | NON logique | droite a gauche |
| 2 | `~` | NON bit a bit | droite a gauche |
| 3 | `*` `/` `%` | Multiplication, division, modulo | gauche a droite |
| 4 | `+` `-` | Addition, soustraction | gauche a droite |
| 5 | `<<` `>>` | Decalages bit a bit | gauche a droite |
| 6 | `<` `<=` `>` `>=` | Comparaisons | gauche a droite |
| 7 | `==` `!=` | Egalite, difference | gauche a droite |
| 8 | `&` | ET bit a bit | gauche a droite |
| 9 | `^` | XOR bit a bit | gauche a droite |
| 10 | `\|` | OU bit a bit | gauche a droite |
| 11 | `&&` | ET logique (court-circuit) | gauche a droite |
| 12 | `\|\|` | OU logique (court-circuit) | gauche a droite |
| 13 | `? :` | Ternaire | droite a gauche |
| 14 | `=` | Affectation | droite a gauche |
| 14 | `+=` `-=` `*=` `/=` `%=` | Affectations composees | droite a gauche |

Les operateurs `&&` et `||` evaluent en court-circuit : le deuxieme operande n'est evalue que si necessaire.

### 4.5 Structures de controle

**if / else :**
```c
if (condition) {
    // ...
} else if (autre_condition) {
    // ...
} else {
    // ...
}
```

**while :**
```c
while (condition) {
    // ...
}
```

**do-while :**
```c
do {
    // ...
} while (condition);
```

**for :**
```c
for (int i = 0; i < 10; i++) {
    // ...
}
```

La clause d'initialisation peut etre une declaration ou une expression. Chacune des trois clauses peut etre omise (sauf que `for(;;)` fait une boucle infinie).

**switch / case :**
```c
switch (x) {
    case 1:
        // ...
        break;
    case 2:
        // ...
        break;
    default:
        // ...
        break;
}
```

Le `break` est necessaire pour eviter le fall-through vers le case suivant.

**break et continue :**

`break` sort de la boucle (while, for, do-while) ou du switch courant. `continue` saute a l'iteration suivante de la boucle. Les deux ne sont valides qu'a l'interieur d'une boucle (ou d'un switch pour break).

### 4.6 Fonctions

**Definition :**
```c
int addition(int a, int b) {
    return a + b;
}

void afficher(int x) {
    putchar(x + '0');
}
```

**Passage de parametres :**

Les 6 premiers parametres sont passes dans les registres (convention System V ABI). A partir du 7e parametre, les arguments sont passes sur la pile. C'est transparent pour l'utilisateur.

**Fonctions externes disponibles :**

- `putchar(int c)` : ecrit un caractere sur stdout
- `getchar()` : lit un caractere depuis stdin

Ces fonctions sont liees automatiquement par gcc lors du linking.

**Recursion :**

La recursion et la recursion mutuelle fonctionnent normalement :

```c
int factorielle(int n) {
    if (n <= 1) return 1;
    return n * factorielle(n - 1);
}
```

### 4.7 Constantes

**Entiers :**
```c
42
0
255
```

**Caracteres :**
```c
'a'
'Z'
'\n'    // retour a la ligne
'\t'    // tabulation
'\0'    // caractere nul
'\\'    // backslash
'\''    // apostrophe
```

Les constantes caractere sont converties en leur valeur ASCII (int).

**Flottants :**
```c
3.14
.5     // equivalent a 0.5
2.     // equivalent a 2.0
```

## 5. Optimisations

Le compilateur effectue plusieurs optimisations pendant la generation de l'IR.

### 5.1 Constant folding

Le constant folding evalue les expressions constantes directement a la compilation, sans generer de code pour les calculer a l'execution.

**Operations supportees sur les int :**

- Arithmetique : `+`, `-`, `*`, `/`
- Modulo : `%`
- Bit a bit : `&`, `|`, `^`
- Decalages : `<<`, `>>`
- Comparaisons : `==`, `!=`, `<`, `<=`, `>`, `>=`
- Moins unaire : `-x`
- NON logique : `!x`

**Operations supportees sur les double :**

- Arithmetique : `+`, `-`, `*`, `/`
- Moins unaire : `-x`
- NON logique : `!x`

**Cas ou le constant folding ne s'applique PAS :**

- Comparaisons de double : les instructions `cmp` sont toujours generees
- Acces tableau : meme si l'indice est constant, l'IR est genere normalement
- Arguments de fonction : toujours materialises dans des registres/la pile
- Affectations composees (`+=`, `-=`, etc.) : toujours un load/store

**Simplifications algebriques :**

Le compilateur reconnait aussi certains cas particuliers :

- `x + 0` -> `x`
- `x - 0` -> `x`
- `x * 1` -> `x`
- `x * 0` -> `0`

Ces simplifications evitent de generer des instructions inutiles.

### 5.2 Propagation de constantes

Le compilateur maintient une `constMap` qui associe chaque variable int a sa valeur connue (quand c'est possible). Quand on lit une variable dont la valeur est connue, on utilise directement la constante au lieu de generer un load.

**Exemple :**
```c
int x = 5;
int y = x + 3;   // x est connu comme 5, donc y = 5 + 3 = 8 a la compilation
```

La propagation ne fonctionne que pour les `int`, pas pour les `double`.

**Regles d'invalidation :**

La valeur connue d'une variable est invalidee (retiree de la constMap) dans les cas suivants :

- **Affectation non-constante** : `x = f()` invalide x car on ne connait pas la valeur de retour
- **if/else** : toutes les variables affectees dans l'une ou l'autre branche sont invalidees apres le if/else (on ne sait pas quelle branche a ete prise)
- **Boucles (while, for, do-while)** : toutes les variables affectees dans le corps de la boucle sont invalidees *avant* d'entrer dans la boucle (car le corps peut s'executer 0 ou N fois)
- **Appels de fonction** : la valeur de retour n'est jamais consideree constante

### 5.3 Dead code dans les ternaires

Si la condition d'une expression ternaire est une constante, seule la branche selectionnee est compilee :

```c
int x = (1 ? a : b);   // seul 'a' est evalue, 'b' est ignore
int y = (0 ? a : b);   // seul 'b' est evalue
```

### 5.4 Court-circuit sur constantes

Pour les operateurs logiques `&&` et `||`, si le resultat est determine par le premier operande (qui est une constante), le deuxieme n'est meme pas evalue :

- `0 && anything` -> `0` directement
- `1 || anything` -> `1` directement

## 6. Erreurs detectees

### 6.1 Erreurs semantiques

Le compilateur detecte les erreurs suivantes et quitte avec un code d'erreur :

| Erreur | Message |
|--------|---------|
| Variable non declaree | `error: variable 'x' not declared` |
| Variable redeclaree dans la meme portee | `error: variable 'x' already declared in this scope` |
| Variable declaree comme void | `error: variable 'x' declared void` |
| Parametre de type void | `error: parameter 'x' cannot have type void` |
| Fonction appelee mais jamais definie | `error: function 'f' called but never defined` |
| Mauvais nombre d'arguments | `error: wrong number of arguments for function 'f'` |
| Fonction definie deux fois | `error: function 'f' already defined` |
| main() absente | `error: undefined reference to 'main'` |
| break en dehors d'une boucle | `error: break outside loop/switch` |
| continue en dehors d'une boucle | `error: continue outside loop` |
| Erreur de syntaxe | `error: syntax error during parsing` |

Les fonctions `putchar` et `getchar` sont reconnues comme fonctions externes et ne declenchent pas l'erreur "called but never defined".

### 6.2 Warnings

Ces avertissements sont affiches sur stderr mais ne bloquent pas la compilation :

| Warning | Message |
|---------|---------|
| Variable declaree mais jamais utilisee | `warning: variable 'x' declared but never used in function f` |
| Fonction definie mais jamais appelee | `warning: function 'f' defined but never called` |

Le warning "function never called" ne s'applique pas a `main`.

## 7. Architecture cible ARM64

### Principe

Le meme IR est utilise pour les deux architectures. Seule la phase de generation de code (backend) change. Le choix se fait au niveau de chaque CFG via le champ `target`.

### Registres utilises

- `w0` a `w7` : registres 32 bits pour les calculs et le passage de parametres
- `x29` : frame pointer
- `x30` : link register (adresse de retour)
- `sp` : stack pointer

### Convention d'appel

Les 8 premiers arguments sont passes dans `w0` a `w7`. Les arguments supplementaires vont sur la pile.

### Limitations ARM64

- **Pas de support double** : les types flottants ne sont pas generes pour arm64. Utiliser `--target=x86` si le programme utilise des double.
- **Grands offsets** : quand un offset depasse 255, le compilateur utilise un registre temporaire (`x11`) pour charger l'adresse, car les instructions ARM n'acceptent que des immediats limites.

## 8. Limitations

Voici la liste de ce que ifcc ne supporte **pas** :

- **Pas de preprocesseur** : pas de `#include`, `#define`, `#ifdef`, etc.
- **Pas de `struct` ni `union`**
- **Pas de `enum`**
- **Pas de `typedef`**
- **Pas de pointeurs** : pas de `*`, `&` (adresse de), `->`, pas de malloc/free
- **Pas de `string`** ni de chaines de caracteres (`"hello"`)
- **Pas de tableaux multi-dimensionnels** : `int a[3][4]` ne compile pas
- **Pas de VLA** (variable-length arrays)
- **Pas d'initialisation de tableau** : `int a[3] = {1, 2, 3}` n'est pas supporte
- **Pas de `static`, `extern`, `const`** (mots-cles ignores ou non reconnus)
- **Pas de `unsigned`**
- **Pas de `long`, `short`, `float`**
- **Pas de cast explicite** : `(int)x` ne fonctionne pas (les conversions int/double sont implicites)
- **Pas de virgule comme operateur** : `a = (1, 2)` ne compile pas
- **Pas de `goto` ni de labels**
- **Pas de `sizeof`**
- **Pas de double sur ARM64**
- **Pas de variables globales** : toutes les variables sont locales
- **Pas de fonctions variadiques** (comme printf)

## 9. Tests

### Suite de tests

Le projet contient environ 150 fichiers de test dans le dossier `testfiles/`. Chaque fichier est un programme C autonome qui teste une fonctionnalite precise.

### Lancer les tests

```bash
python3 ifcc-test.py testfiles/
```

Le script fait la chose suivante pour chaque fichier `.c` :

1. Compile le fichier avec gcc et execute le binaire -> note le code de retour et la sortie stdout
2. Compile le fichier avec ifcc, assemble avec gcc, execute -> note le code de retour et la sortie stdout
3. Compare les deux resultats

Un test passe si le code de retour ET la sortie stdout sont identiques entre gcc et ifcc.

### Nommage des tests

Les fichiers de test suivent la convention `NNN_description.c`. Les fichiers dont le nom contient `err` sont des tests de programmes incorrects (le compilateur doit echouer).

### CI/CD

Les tests sont automatises via GitHub Actions sur Ubuntu 22.04. A chaque push, la suite complete est executee.

## 10. Exemples

### Exemple 1 : Hello World (avec putchar)

```c
void putchar(int c);

int main() {
    putchar('H');
    putchar('e');
    putchar('l');
    putchar('l');
    putchar('o');
    putchar('\n');
    return 0;
}
```

Sortie attendue :
```
Hello
```
Code retour : 0

### Exemple 2 : Factorielle

```c
int factorielle(int n) {
    if (n <= 1) return 1;
    return n * factorielle(n - 1);
}

int main() {
    return factorielle(6);
}
```

Code retour : `echo $?` affiche 208 (720 % 256, car le code retour est sur 8 bits).

Pour recuperer la vraie valeur, on peut utiliser putchar pour afficher les chiffres.

### Exemple 3 : Tri a bulles

```c
void putchar(int c);

void print_array(int a[], int n) {
    int i;
    for (i = 0; i < n; i = i + 1) {
        putchar(a[i] + '0');
        putchar(' ');
    }
    putchar('\n');
}

int main() {
    int a[5];
    a[0] = 5;
    a[1] = 3;
    a[2] = 1;
    a[3] = 4;
    a[4] = 2;

    int n = 5;
    int i;
    int j;
    for (i = 0; i < n - 1; i = i + 1) {
        for (j = 0; j < n - i - 1; j = j + 1) {
            if (a[j] > a[j + 1]) {
                int tmp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = tmp;
            }
        }
    }

    print_array(a, n);
    return 0;
}
```

Sortie attendue :
```
1 2 3 4 5
```

### Exemple 4 : Utilisation de double (x86 uniquement)

```c
int main() {
    double pi = 3.14;
    double r = 2.0;
    double aire = pi * r * r;
    int result = aire;
    return result;
}
```

Code retour : 12 (la partie entiere de 3.14 * 4 = 12.56)

### Exemple 5 : Switch / case

```c
void putchar(int c);

int main() {
    int x = 2;
    switch (x) {
        case 1:
            putchar('A');
            break;
        case 2:
            putchar('B');
            break;
        case 3:
            putchar('C');
            break;
        default:
            putchar('?');
            break;
    }
    putchar('\n');
    return 0;
}
```

Sortie attendue :
```
B
```
