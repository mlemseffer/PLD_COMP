# Fiche P1 — Introduction, Architecture, Bilan

## Ce que tu dois expliquer (7 minutes)

1. Présenter le projet ifcc en une phrase percutante
2. Annoncer le bilan : 75/75 tests, 100% features
3. Expliquer l'architecture 3 passes (pipeline)
4. Montrer l'avantage multi-cible (même IR, deux backends)
5. Présenter l'organisation de la soutenance

---

## Déroulé minute par minute

- **0:00–0:45** : Accroche + présentation du projet (slides 1–2)
  > "Nous avons construit ifcc, un compilateur C vers assembleur x86-64 et ARM64, from scratch, en C++ et ANTLR4. Résultat : 75 tests sur 75. Voici comment on y est arrivés."

- **0:45–2:00** : Bilan chiffré + vue d'ensemble du pipeline (slides 3–4)
  > Montrer le schéma pipeline. Insister sur les 3 passes indépendantes.

- **2:00–3:30** : Détail des 3 passes (slides 5–7)
  > Passe 1 = sémantique, Passe 2 = IR/CFG, Passe 3 = asm. Chacune enchaîne sur la suivante.

- **3:30–5:00** : Génération de code + avantage multi-cible (slides 8–9)
  > Schéma en T : même IR, deux backends. Montrer que le front-end n'a pas bougé pour ARM64.

- **5:00–6:30** : Ce qu'on n'a pas fait / organisation soutenance (slide 10)
  > "P2 va maintenant entrer dans le détail de la grammaire ANTLR4..."

- **6:30–7:00** : Transition vers P2

---

## Les concepts clés à maîtriser

### Pipeline de compilation
Le compilateur exécute 3 passes séquentielles :
1. **ANTLR4 Parser** : lit le code C, produit un arbre de dérivation (CST = Concrete Syntax Tree)
2. **SymbolTableVisitor** : visite le CST, construit la table des symboles, détecte les erreurs sémantiques
3. **IRGenVisitor** : produit l'IR 3 adresses organisée en CFG (Graphe de Flot de Contrôle)
4. **gen_asm()** : traduit l'IR en assembleur x86-64 ou ARM64

Ces passes sont **indépendantes** : chacune consomme la sortie de la précédente et ne modifie pas les précédentes.

### Pourquoi une IR ?
L'IR (Représentation Intermédiaire) découple le front-end (C) du back-end (asm). Avantage concret : pour ajouter ARM64, on a juste ajouté ~500 lignes de `gen_asm_arm64()` sans toucher au parser ni à l'analyse sémantique.

### Les chiffres clés
- 75/75 tests automatisés
- ~40 types d'instructions IR
- 2 backends (x86-64 et ARM64)
- 100% features obligatoires + facultatives

---

## Exemples de code à connaître

### Pipeline (à dessiner au tableau si besoin)
```
Source .c
   → [ANTLR4 Parser]       → CST
   → [SymbolTableVisitor]  → table des symboles
   → [IRGenVisitor]        → IR + CFG
   → [gen_asm()]           → .s (x86-64 ou ARM64)
```

### Fichiers principaux
```
ifcc.g4              ← grammaire
main.cpp             ← orchestre les 3 passes
SymbolTableVisitor   ← passe 1
IRGenVisitor         ← passe 2
IR.h/.cpp            ← structures IR + passe 3
```

---

## Questions pièges possibles du jury

**Q : Pourquoi 3 passes séparées et pas une seule traversée de l'AST ?**
R : La séparation permet de détecter toutes les erreurs sémantiques avant de générer du code. Si on mélange les deux, on génère du code IR pour un programme qui sera ensuite rejeté. De plus, c'est plus maintenable : chaque passe a une responsabilité unique.

**Q : Pourquoi ANTLR4 et pas un parser fait à la main ?**
R : ANTLR4 offre une grammaire déclarative lisible, la gestion automatique des priorités d'opérateurs, et des messages d'erreur. Un parser récursif descendant fait à la main aurait pris beaucoup plus de temps pour le même résultat.

**Q : Qu'est-ce qu'un CST par rapport à un AST ?**
R : Un CST (Concrete Syntax Tree) conserve tous les tokens, y compris les ponctuation et mots-clés. Un AST (Abstract Syntax Tree) ne garde que les nœuds sémantiquement utiles. ANTLR4 produit un CST ; notre Visitor le traite comme un AST en ignorant les tokens non pertinents.

**Q : Votre compilateur produit-il du code optimisé ?**
R : Partiellement. On fait du constant folding et de la propagation de constantes. On ne fait pas d'allocation de registres (tout en mémoire) ni de Dead Code Elimination. Le code produit est correct mais pas aussi compact que GCC -O2.

**Q : Comment avez-vous testé que le compilateur est correct ?**
R : Suite de 75 tests automatisés : on compile chaque programme avec GCC et avec ifcc, on compare le code de retour et stdout. Si les deux produisent le même résultat, le test passe.

**Q : Pourquoi ARM64 en plus de x86-64 ?**
R : Pour valider que l'architecture IR est bien indépendante de la cible. Si l'IR était polluée par des détails x86, on n'aurait pas pu ajouter ARM64 proprement. Le fait que ça marche prouve que l'abstraction est correcte.

**Q : Qu'est-ce qui vous a pris le plus de temps ?**
R : La gestion correcte de l'ABI (passage des arguments, alignement de pile) et les cas limites du scoping (shadowing, renommage de variables). Le dernier bug corrigé était `!retval` non initialisé à 0 dans le prologue.

---

## Phrases d'accroche pour enchaîner avec P2

> "Vous avez vu l'architecture globale. Maintenant, entrons dans le détail de la première étape : comment on parse le code C source. Je passe la parole à [P2] qui va vous expliquer notre grammaire ANTLR4."

> "Le point d'entrée de tout compilateur, c'est le parser. [P2] va vous montrer comment on a décrit le langage C dans notre grammaire et ce qu'ANTLR4 nous offre."
