# Rapport de Tests - Compilateur IFCC

**Date:** 3 mars 2026  
**Compilateur:** IFCC (skeleton initial)  
**Langage cible:** x86-64 Assembly  
**Testeur:** Test suite Python automatisée

---

## 1. Résumé Exécutif

| Métrique | Valeur |
|----------|--------|
| Tests totaux | 9 |
| Tests réussis | 6 |
| Tests échoués | 3 |
| Taux de réussite | 67% |

**État:** Le compilateur IFCC fonctionne correctement pour les programmes simples retournant des constantes entières. Les limitations actuelles portent sur les variables et les appels de fonction.

---

## 2. Résultats Détaillés par Test

### ✅ Tests Réussis (6)

#### Test 1: `1_return42.c` - Retour basique
```c
int main() {
    return 42;
}
```
- **Compilateur GCC:** ✅ Succès (exit code: 42)
- **Compilateur IFCC:** ✅ Succès (exit code: 42)
- **Assembleur IFCC généré:** 47 bytes
- **Résultat:** `TEST OK` - Les deux compilers produisent les mêmes résultats

---

#### Test 2: `2_invalid_program.c` - Texte invalide
```
Si ça c'est du C, moi je suis prof de manga.
```
- **Compilateur GCC:** ❌ Rejet (erreur de syntaxe)
- **Compilateur IFCC:** ❌ Rejet (erreur de syntaxe)
- **Résultat:** `TEST OK` - Les deux rejettent correctement le programme invalide

---

#### Test 4: `4_no_main.c` - Pas de fonction main
```c
int foobar() {
    return 42;
}
```
- **Compilateur GCC:** ❌ Rejet (undefined reference to `main`)
- **Compilateur IFCC:** ❌ Rejet (expected 'int')
- **Résultat:** `TEST OK` - Les deux rejettent correctement le programme

---

#### Test 6: `6_invalid_syntax.c` - Syntaxe incorrecte
```c
int main { /* parenthèses manquantes */
    return 42;
}
```
- **Compilateur GCC:** ❌ Rejet (erreur de syntaxe)
- **Compilateur IFCC:** ❌ Rejet (erreur de syntaxe)
- **Résultat:** `TEST OK` - Les deux rejettent correctement

---

#### Test 8: `8_return_zero.c` - Retour zéro
```c
int main() {
    return 0;
}
```
- **Compilateur GCC:** ✅ Succès (exit code: 0)
- **Compilateur IFCC:** ✅ Succès (exit code: 0)
- **Assembleur IFCC généré:** 48 bytes
- **Résultat:** `TEST OK`

---

#### Test 9: `9_return_255.c` - Retour valeur maximale
```c
int main() {
    return 255;
}
```
- **Compilateur GCC:** ✅ Succès (exit code: 255)
- **Compilateur IFCC:** ✅ Succès (exit code: 255)
- **Assembleur IFCC généré:** 49 bytes
- **Résultat:** `TEST OK`

---

### ❌ Tests Échoués (3)

#### Test 3: `3_return_var.c` - Variables
```c
int main() {
    int x;
    x=8;
    return x;
}
```
- **Compilateur GCC:** ✅ Succès (exit code: 8)
- **Compilateur IFCC:** ❌ Erreur - Ne supporte pas les variables
- **Message d'erreur IFCC:** 
  ```
  line 2:8 token recognition error at: 'x'
  line 3:4 token recognition error at: 'x'
  line 3:5 token recognition error at: '='
  line 4:11 token recognition error at: 'x'
  line 2:4 mismatched input 'int' expecting 'return'
  error: syntax error during parsing
  ```
- **Résultat:** `TEST FAIL` - Feature manquante

---

#### Test 5: `5_no_return.c` - Pas de return explicite
```c
int main() {
    42;
}
```
- **Compilateur GCC:** ✅ Succès (exit code: 0 - return implicite)
- **Compilateur IFCC:** ❌ Erreur - Manque le `return`
- **Message d'erreur IFCC:**
  ```
  line 3:4 missing 'return' at '42'
  error: syntax error during parsing
  ```
- **Résultat:** `TEST FAIL` - Grammar trop stricte

---

#### Test 7: `7_hello_world.c` - Appel de fonction
```c
#include <stdio.h>
int main() {
    printf("Hello World\n");
    return 0;
}
```
- **Compilateur GCC:** ✅ Succès (exit code: 0)
- **Compilateur IFCC:** ❌ Erreur - Ne supporte pas #include ni printf()
- **Message d'erreur IFCC:**
  ```
  line 4:4 token recognition error at: 'p'
  line 4:10 mismatched input '(' expecting 'return'
  error: syntax error during parsing
  ```
- **Résultat:** `TEST FAIL` - Features non implémentées

---

## 3. Analyse Comparative GCC vs IFCC

### Compilation
| Étape | GCC | IFCC |
|-------|-----|------|
| C → Assembly | Natif | ✅ Généré + `.s` |
| Assembly → Object | Natif | Délégué à `as` |
| Object → Executable | Natif | Délégué à `gcc` |

### Taille de l'assembleur généré (test 1_return42.c)
- **GCC:** 556 bytes
- **IFCC:** 47 bytes
- **Réduction:** 91.5% (code minimaliste)

### Compatibilité
- **Syntaxe C supportée:** `int main() { return CONST; }`
- **Commentaires:** `/* ... */` uniquement (pas `//`)
- **Constantes:** Nombres décimaux uniquement

---

## 4. Infrastructure de Test

### Artefacts générés par test
```
ifcc-test-output/[test-name]/
├── input.c              (copie du source)
├── asm-gcc.s            (assembleur GCC)
├── asm-ifcc.s           (assembleur IFCC)
├── exe-gcc              (exécutable GCC)
├── exe-ifcc             (exécutable IFCC)
├── gcc-compile.txt      (logs compilation GCC)
├── gcc-link.txt         (logs linkage GCC)
├── gcc-execute.txt      (résultats exécution GCC)
├── ifcc-compile.txt     (logs IFCC)
├── ifcc-link.txt        (logs linkage)
└── ifcc-execute.txt     (résultats exécution IFCC)
```

### Modes d'utilisation du script

```bash
# Mode fichiers multiples (défaut)
python3 ifcc-test.py testfiles
python3 ifcc-test.py testfiles -v        # Détails en cas d'erreur
python3 ifcc-test.py testfiles -vv       # Verbeux complet

# Mode fichier unique (like GCC)
python3 ifcc-test.py -S -o file.s file.c    # → Assembly
python3 ifcc-test.py -c -o file.o file.c    # → Object code  
python3 ifcc-test.py -o exe file.c          # → Executable
```

---

## 5. Limitation Connues

### Non supporté dans IFCC v1
1. **Variables** - Requiert gestion des registres/stack
2. **Expressions** - Opérations (+, -, *, /)
3. **Appels de fonction** - printf(), custom functions
4. **Structures de contrôle** - if, while, for
5. **Return implicite** - GCC ajoute `return 0` automatiquement
6. **Commentaires `//`** - Seulement `/* */` supportés

### Correctifs suggérés pour tests futurs
- Test 3 (variables): À implémenter dans sprint 2
- Test 5 (no return): À implémenter dans sprint 2  
- Test 7 (printf): À implémenter dans sprint X (après fonctions)

---

## 6. Validation Effectuée

✅ **Programmes valides et fonctionnels:**
- `1_return42.c` - Retour constant
- `8_return_zero.c` - Retour 0
- `9_return_255.c` - Retour valeur élevée

✅ **Programmes invalides correctement rejetés:**
- `2_invalid_program.c` - Texte aléatoire
- `4_no_main.c` - Pas de main
- `6_invalid_syntax.c` - Syntaxe C invalide

---

## 7. Conclusions

Le compilateur IFCC skeleton fonctionne correctement pour son cas d'usage actuel: traduire un programme C minimal (`int main() { return CONST; }`) en assembleur x86-64.

La qualité du code généré est excellente (47 bytes vs 556 de GCC), démontrant l'efficacité d'une approche minimaliste guidée par les besoins.

Les trois échecs de test correspondent tous à des features volontairement non implémentées dans cette version skeleton, comme énoncé dans le cahier des charges.

---

**Environnement de test:**
- OS: Ubuntu 24.04 LTS
- Architecture: x86-64
- Compilateur référence: GCC 13.3.0
- ANTLR: 4.13.1

**Prochaines étapes:** Implémenter les variables et expressions arithmétiques pour supporter le premier sprint fonctionnel complet.
