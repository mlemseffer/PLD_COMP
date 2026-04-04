# Fiche P8 — Tests, Démo, Conclusion

## Ce que tu dois expliquer (7 minutes)

1. La suite de tests automatisés : principe, couverture, résultats
2. 75/75 : graphique de progression, dernier bug corrigé
3. CI/CD avec GitHub Actions
4. Démo scriptée de `game.c` compilé avec ifcc
5. Ce qu'on n'a pas fait et pourquoi (choix délibérés)
6. Améliorations futures
7. Bilan et conclusion

---

## Déroulé minute par minute

- **0:00–1:30** : Suite de tests + principe de validation (slides 83–85)
  > "Comment on prouve que notre compilateur est correct ? On compile chaque programme avec GCC et avec ifcc, et on compare les sorties. Si c'est identique, c'est correct."

- **1:30–2:30** : 75/75 graphique + dernier bug (slide 84)
  > Raconter la progression. Le dernier bug est une bonne histoire à raconter : `!retval` non initialisé → fix dans le prologue.

- **2:30–3:15** : CI/CD GitHub Actions (slide 86)
  > Montrer le pipeline. Badge vert. Feedback immédiat sur les régressions.

- **3:15–5:00** : Démo game.c (slides 87–88)
  > C'est le moment le plus dynamique. Avoir les commandes prêtes. Taper ou simuler l'exécution. Commenter la sortie.

- **5:00–5:45** : Ce qu'on n'a pas fait (slide 89)
  > Aller vite. Être honnête : "On a fait des choix de scope. Voici ce qu'on n'a pas implémenté et pourquoi."

- **5:45–6:30** : Améliorations futures + bilan (slides 90–91)

- **6:30–7:00** : Conclusion + remerciements (slide 92)

---

## Les concepts clés à maîtriser

### Principe de la suite de tests
```
Pour chaque fichier .c dans testfiles/ :
1. Compiler avec GCC     → gcc prog.c -o ref
2. Compiler avec ifcc    → ./ifcc prog.c > prog.s; gcc prog.s -o test
3. Exécuter les deux avec les mêmes entrées
4. Comparer :
   - le code de sortie (exit code)
   - la sortie stdout
Si identiques → TEST PASSÉ ✓
```

Le script `ifcc-test.py` automatise cette procédure pour les 75 fichiers.

**Pourquoi cette approche ?** Elle teste le compilateur bout en bout sans avoir à écrire manuellement le résultat attendu pour chaque programme. GCC est la référence.

### Les 75 tests : couverture
| Catégorie | Exemples de fichiers |
|-----------|---------------------|
| Expressions arithmétiques | `1_add`, `2_mul`, `3_div`, `4_mod` |
| Comparaisons | `5_cmp_eq`, `6_cmp_lt` |
| Structures de contrôle | `10_if`, `11_while`, `12_for`, `13_switch` |
| Fonctions | `20_func_args`, `21_recursive`, `22_void` |
| Types | `30_double`, `31_char`, `32_type_coerce` |
| Tableaux | `40_array_basic`, `41_array_loop` |
| Break/Continue | `50_break`, `51_continue`, `52_nested` |
| Scoping | `60_scope`, `61_shadow`, `62_multi_scope` |
| Edge cases | `70_no_return`, `71_compound_op`, `72_ternary` |

### Le dernier bug : `5_no_return`
```c
int main() {
    // pas de return explicite
}
```
- **Symptôme** : le code de sortie était aléatoire (valeur de `%eax` non définie)
- **Cause** : `!retval` n'était pas initialisé dans le prologue → `%eax` contenait une valeur garbage
- **Fix** : ajouter `movl $0, -4(%rbp)` dans le prologue (où `-4(%rbp)` = offset de `!retval`)
- **Leçon** : C99 spécifie que `main()` sans `return` retourne 0. Notre fix respecte ce comportement.

### CI/CD GitHub Actions
Pipeline déclenché sur chaque `git push` :
1. **Checkout** du code
2. **Install** ANTLR4 runtime
3. **Build** : `make`
4. **Test** : `python3 ifcc-test.py testfiles/`
5. **Rapport** : affiche le nb de tests passés

Si un test régresse → le push est marqué en rouge → feedback immédiat.

### Démo game.c : ce que le programme fait
Le jeu de devinette `game.c` :
- Le programme "choisit" un nombre entre 1 et 100 (hardcodé à 42 dans la démo)
- Le joueur entre des nombres ; le programme répond "Trop petit !", "Trop grand !", "Bravo !"
- L'affichage se fait entièrement via `putchar` (pas de `printf`) → contrainte ifcc
- Utilise : `while`, `if/else`, `&&` court-circuit, fonctions, arithmétique, récursion

### Démo game.c : les commandes exactes
```bash
# 1. Vérifier que le compilateur est compilé
make

# 2. Compiler game.c avec NOTRE compilateur
./ifcc compiler/tests/game.c > /tmp/game.s

# 3. Voir l'assembleur généré (optionnel, si le jury demande)
cat /tmp/game.s | head -30

# 4. Assembler et lier
gcc /tmp/game.s -o /tmp/game

# 5. Lancer avec des inputs scriptés
printf '10\n80\n42\n' | /tmp/game
```

**Sortie attendue :**
```
Trop petit !
Trop grand !
Bravo ! Trouvé en 3 essais.
```

### Ce qu'on n'a pas fait (à connaître)
| Feature | Pourquoi pas |
|---------|-------------|
| **Pointeurs** | Arithmétique de pointeurs + déréférencement → complexe, hors scope du projet |
| **Variables globales** | Segment `.data` + problèmes de linkage entre fonctions → hors scope |
| **Chaînes de caractères** | Nécessite pointeurs + `.rodata` + fonctions de manipulation |
| **Tableaux multidimensionnels** | Extension directe de 1D mais temps limité |
| **Allocation de registres** | Algorithme de coloration de graphe → sujet de thèse en soi |
| **Struct/Union** | Pas dans le scope du projet |
| **printf** | Variadique, ABI différente (AL = nb registres XMM) |

### Améliorations futures
1. **Allocation de registres** : algorithme de Chaitin (coloration de graphe) → moins de load/store
2. **Dead Code Elimination** : supprimer les BasicBlocks non atteignables après les branches toujours vraies/fausses
3. **Forme SSA** : Static Single Assignment → analyses et optimisations plus puissantes (GVN, LICM...)
4. **Inlining** : appels de petites fonctions → substitution inline
5. **RISC-V backend** : 3ème cible pour valider l'abstraction IR
6. **Meilleur reporting d'erreurs** : numéros de ligne, messages contextuels

---

## Script de démo détaillé (à lire avant la soutenance)

```bash
# Étape 1 : montrer le code source
cat compiler/tests/game.c

# Étape 2 : compiler avec ifcc (sur la machine de demo)
./ifcc compiler/tests/game.c > /tmp/game.s

# Étape 3 : montrer l'assembleur (premiers slides)
head -40 /tmp/game.s

# Étape 4 : linker
gcc /tmp/game.s -o /tmp/game

# Étape 5 : exécuter de manière interactive (si besoin)
/tmp/game
# > 10  (entrer manuellement)
# Trop petit !
# > 80
# Trop grand !
# > 42
# Bravo ! Trouvé en 3 essais.

# OU exécuter en mode scripté (plus fiable pour une démo)
printf '10\n80\n42\n' | /tmp/game
```

**Plan B si ça ne marche pas :**
- Avoir les sorties copiées dans un fichier texte
- Dire "voici la sortie que vous auriez sur une machine Linux" et l'afficher

---

## Questions pièges possibles du jury

**Q : Est-ce que vos 75 tests couvrent tous les cas d'angle (corner cases) ?**
R : Ils couvrent les cas documentés dans nos specs. Certains comportements indéfinis du C (accès hors tableau, `int` overflow) ne sont pas testés — comme GCC, on génère le code sans vérification à l'exécution.

**Q : Si GCC produit une sortie différente d'un autre compilateur, votre test passe quand même ?**
R : Oui, pour les comportements définis. Pour les comportements indéfinis, GCC et ifcc peuvent produire des résultats différents — mais nos tests évitent délibérément les comportements indéfinis.

**Q : Le test compare-t-il le code assembleur généré ou le comportement ?**
R : Le **comportement** (code de sortie + stdout). On ne compare pas l'assembleur lui-même — qui serait différent. Seul le résultat observable compte, ce qui est la bonne approche.

**Q : Pourquoi ne pas avoir utilisé Google Test ou un framework de tests C++ ?**
R : Le testeur compare deux exécutables, pas des fonctions internes. Un framework de tests unitaires aurait testé des fonctions individuelles du compilateur. Notre approche bout-en-bout est plus réaliste : elle teste ce qui compte vraiment.

**Q : Comment avez-vous géré les conflits de merge dans votre équipe de 8 ?**
R : Chaque personne travaillait sur une branche feature. On faisait des PR avec review avant merge. La suite de tests permettait de détecter les régressions immédiatement après chaque merge.

**Q : Combien de temps aurait-il fallu pour implémenter les pointeurs ?**
R : Probablement une semaine supplémentaire. L'IR a déjà `wmem`/`rmem` pour l'accès mémoire indirect, et `lea` pour les adresses. Il faudrait ajouter la grammaire (`*`, `&`, déréférencement), la sémantique (types pointeur), et la génération IR correspondante.

**Q : Votre compilateur génère-t-il du code plus lent que GCC ?**
R : Oui, significativement. GCC -O2 fait de l'allocation de registres, du loop unrolling, de l'inlining... Notre code met tout en mémoire, donc beaucoup plus de load/store. C'est correct, pas optimal.

---

## Bilan général à mémoriser

- **75/75 tests** : preuve de correction sur l'ensemble du sous-ensemble C implémenté
- **Architecture propre** : 3 passes, IR découplée, multi-cible
- **100%** des features obligatoires ET facultatives implémentées
- **Dernier bug** : `!retval` non initialisé — trouvé et corrigé grâce aux tests automatisés
- **Enseignement clé** : les tests automatisés nous ont sauvé plusieurs fois lors des merges

---

## Phrases de conclusion

> "75 tests sur 75. Un compilateur qui compile du vrai C, vers du vrai assembleur, pour deux architectures. On est fiers du résultat, et surtout de l'architecture : propre, extensible, testée. Merci pour votre attention."

> "Ce projet nous a appris autant sur la compilation que sur l'ingénierie logicielle : l'importance de l'abstraction, des tests automatisés, et de séparer clairement les responsabilités. Merci."
