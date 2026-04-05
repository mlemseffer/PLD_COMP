# ifcc - Guide d'utilisation

Compilateur d'un sous-ensemble du langage C vers assembleur x86-64 / ARM64.

## Pre-requis

- g++ (C++17), cmake, make
- Java JDK (pour ANTLR4)
- ANTLR 4.13.2 (JAR + runtime C++)
- Python 3 (pour les tests)
- **Windows** : WSL avec Ubuntu installe
- **Mac** : Xcode Command Line Tools (`xcode-select --install`), Homebrew recommande

## Compilation du compilateur

### Linux

```bash
cd compiler
cp config-IF501.mk config.mk   # adapter les chemins ANTLR si necessaire
make
```

### Windows (via WSL)

```bash
wsl bash -c "cd /mnt/c/chemin/vers/PLD_COMP && cd compiler && make"
```

### Mac

```bash
# Installer les dependances avec Homebrew
brew install antlr4-cpp-runtime cmake

cd compiler
cp config-IF501.mk config.mk   # adapter les chemins ANTLR a votre installation
make
```

Sur Apple Silicon (M1/M2/M3), le compilateur genere de l'assembleur ARM64 par defaut. Pour forcer x86 (necessaire pour le support `double`), utiliser `--target=x86`.

## Utilisation

```bash
# Compiler un fichier C
./compiler/ifcc source.c > output.s

# Assembler et linker avec GCC
gcc output.s -o output

# Executer
./output
echo $?   # affiche le code de retour
```

### Choisir l'architecture cible

```bash
./compiler/ifcc --target=x86 source.c > output.s
./compiler/ifcc --target=arm64 source.c > output.s
```

Par defaut : x86 sur Linux/Windows (WSL), arm64 sur Mac Apple Silicon.

## Langage supporte

| Feature | Details |
|---------|---------|
| **Types** | `int`, `char`, `double` (x86 uniquement), `void` |
| **Operateurs** | `+ - * / %`, comparaisons, logique (`&& \|\| !`), bit-a-bit, ternaire `? :` |
| **Affectation** | `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `++x`, `--x` |
| **Controle** | `if/else`, `while`, `for`, `do-while`, `switch/case`, `break`, `continue` |
| **Fonctions** | Definition, appel, recursion, jusqu'a 6+ parametres |
| **Tableaux** | 1D a taille fixe (`int a[10]`) |
| **Scoping** | Blocs `{}` avec shadowing |
| **Externes** | `putchar()`, `getchar()` |

## Lancer les tests

### Linux / Mac

```bash
python3 ifcc-test.py testfiles/
```

### Windows (via WSL)

```bash
wsl bash -c "cd /mnt/c/chemin/vers/PLD_COMP && python3 ifcc-test.py testfiles/"
```

Le script compile chaque fichier `.c` avec GCC et ifcc, puis compare les resultats (code de retour + sortie standard).

Option `-v` pour le detail des echecs :

```bash
python3 ifcc-test.py testfiles/ -v
```

## Limitations principales

- Pas de pointeurs, strings, struct, union, enum, typedef
- Pas de preprocesseur (`#include` ignore)
- Pas de variables globales
- Pas de `float`, `unsigned`, `long`, `short`
- Pas de cast explicite
- `double` non supporte sur ARM64
- Tableaux : 1D, taille fixe uniquement
