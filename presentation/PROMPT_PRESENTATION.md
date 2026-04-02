# Prompt pour générer les supports de présentation

Copie tout ce qui suit et donne-le à Claude dans une nouvelle conversation.

---

## PROMPT

Tu vas m'aider à préparer une **soutenance orale de 56 minutes** (8 personnes × 7 minutes chacune) pour un projet de compilateur C réalisé à l'INSA Lyon en 4IF.

---

## CONTEXTE DU PROJET

Nous avons développé **ifcc**, un compilateur d'un sous-ensemble du langage C vers assembleur x86-64 (et ARM64), écrit en C++ avec ANTLR4.

### Architecture : 3 passes indépendantes
```
Source .c
   → [ANTLR4 Parser] → Arbre de dérivation (CST)
   → [SymbolTableVisitor] → Vérifications sémantiques + table des symboles
   → [IRGenVisitor] → IR 3 adresses + Graphe de Flot de Contrôle (CFG)
   → [gen_asm()] → Assembleur x86-64 ou ARM64
```

### Fichiers principaux
- `ifcc.g4` : grammaire ANTLR4
- `main.cpp` : point d'entrée, orchestre les 3 passes
- `SymbolTableVisitor.h/.cpp` : passe 1 — analyse sémantique
- `IRGenVisitor.h/.cpp` : passe 2 — génération IR + CFG
- `IR.h/.cpp` : structures IR (IRInstr, BasicBlock, CFG) + passe 3 (gen_asm)
- `CodeGenVisitor.h/.cpp` : ancien générateur direct (conservé, non utilisé)

---

## FONCTIONNALITÉS IMPLÉMENTÉES

### Obligatoires (O) — 100% faites
- Type `int` (32 bits), `void`, `char` (traité comme int)
- Expressions : `+`, `-`, `*`, `/`, `%`, parenthèses, unaire `-` et `!`
- Opérations bit-à-bit : `&`, `|`, `^`, `<<`, `>>`
- Comparaisons : `==`, `!=`, `<`, `>`, `<=`, `>=`
- Déclaration de variables n'importe où
- Affectation qui retourne une valeur
- `putchar` et `getchar` pour les I/O
- Fonctions avec paramètres (int/void), retour int ou void
- Vérification cohérence appels (nombre d'arguments)
- Blocs `{}`  et portées imbriquées (scopes + shadowing)
- `if`, `else if`, `else`
- `while`
- `return` n'importe où dans une fonction
- Vérifications sémantiques : variable non déclarée, déclarée deux fois, déclarée non utilisée

### Facultatifs (F) — 100% faits
- **ARM64** : reciblage complet, même IR, backend séparé
- **`double`** : type flottant 64 bits, `movsd`/`addsd`/`mulsd`/`divsd`, constantes en `.rodata`
- **Inférence de type** : conversions implicites `int↔double` (`cvtsi2sdl`, `cvttsd2si`)
- **Propagation de constantes** : `constMap` avec analyse data-flow (if/while)
- **Constant folding** : `2+3*4` → `movl $14` directement
- **Tableaux 1D** : déclaration `int arr[N]`, accès `arr[i]` via `leaq` + calcul offset
- **`break` et `continue`** : pile de contextes `loopStack` avec `bb_end`/`bb_cond`
- **`switch/case`** : blocs de base avec comparaisons chaînées, `break` = `jmp end`
- **`&&` et `||` paresseux** : court-circuit = 2 blocs séparés, jump si faux/vrai
- **`+=`, `-=`, `*=`, `/=`, `%=`** : opérateurs composés
- **`++`, `--`** : pré-incrémentation (expression) et post-incrémentation (statement)

### Non prioritaires (NP) — faits en bonus
- `for` : 4 BasicBlocks (init → cond → body → update → cond)
- `do-while` : corps avant condition
- Opérateur ternaire `? :` : 3 blocs, résultat dans temporaire commun
- `<=`, `>=` déjà dans O
- `<<`, `>>` déjà ci-dessus

---

## DÉTAILS TECHNIQUES IMPORTANTS

### IR (Représentation Intermédiaire)
- Instructions à 3 adresses : `[opération, type, dest, src1, src2]`
- ~40 types d'opérations : `ldconst`, `add`, `sub`, `mul`, `div_int`, `mod_int`, `cmp_eq`, `cmp_lt`, `call`, `lea`, `wmem`, `rmem`, `add_addr`, `int_to_double`, `double_to_int`, etc.
- Variables spéciales : `!retval` (valeur de retour), `!tmpN` (temporaires), `!edi`/`!esi`/... (pseudo-registres ABI)

### CFG (Graphe de Flot de Contrôle)
- Un CFG par fonction
- BasicBlock = liste d'instructions IR + `exit_true` + `exit_false` + `test_var_name`
- Bloc de sortie unique `exit_bb` : tous les `return` sautent ici (→ un seul épilogue)
- `has_return` flag : évite les sauts vers bb_end après un return

### Scoping
- Renommage des variables à l'IR : `x` en scope 0 → `x_0`, même `x` en scope 1 → `x_1`
- Pas de collision dans la table des symboles du CFG

### Optimisations
- `ExprValue` : struct avec `isConstant`, `value`, `dvalue`, `varName`, `type`
- Si les deux opérandes d'une expression sont constants → calcul à la compilation
- `constMap` : `map<string, int>` maintenu pendant la visite
- `collectAssignedVars()` : parcourt l'AST récursivement pour invalider les variables modifiées dans les branches

### ABI System V AMD64
- 6 premiers args entiers : `%edi`, `%esi`, `%edx`, `%ecx`, `%r8d`, `%r9d`
- 7+ args : poussés sur la pile (droite → gauche)
- Retour int : `%eax`, retour double : `%xmm0`
- Pile alignée à 16 octets avant chaque `call`
- Prologue : `pushq %rbp; movq %rsp, %rbp; subq $N, %rsp`
- Épilogue : `movl !retval, %eax; leave; ret`
- `!retval` initialisé à 0 dans le prologue (C99 : `main` sans `return` → return 0)

### ARM64
- Frame pointer : `x29`, link register : `x30`
- Prologue : `stp x29, x30, [sp, #-16]!; mov x29, sp; sub sp, sp, #N`
- Épilogue : `mov sp, x29; ldp x29, x30, [sp], #16; ret`
- Chargement : `ldur w8, [x29, #-8]` (offset < 256) ou via `x11` si offset > 255
- Args : `w0`–`w7`, retour : `w0`
- Dispatch dans `IRInstr::gen_asm()` : `if (target == "arm64") gen_asm_arm64(o); else gen_asm_x86(o);`

### Compatibilité ANTLR
- Wrapper `castAny<T>` pour compatibilité ANTLR 4.9 (`antlrcpp::Any::as<T>()`) et 4.10+ (`std::any_cast<T>()`)

### Tests
- Suite : `python3 ifcc-test.py testfiles/`
- Principe : compile avec GCC + compile avec ifcc → compare code de sortie + stdout
- **75/75 tests passent**
- Dernier bug corrigé : `5_no_return` → `!retval` non initialisé → fix : init à 0 dans prologue

### Démo — game.c
- Jeu de devinette compilable avec ifcc
- Utilise uniquement `putchar`/`getchar`
- Montre : fonctions, récursion, while, `&&` court-circuit, if/else, arithmétique
- Lancement : `printf '10\n80\n42\n' | ./game`

---

## CE QUE TU DOIS GÉNÉRER

### 1. Plan de 90 slides (environ)

Génère un plan **slide par slide** avec :
- Numéro de slide
- Titre
- Contenu bullet points (ce qui apparaît sur le slide)
- Qui parle (P1 à P8)
- Durée estimée pour ce slide

Répartition des personnes :
- **P1** (slides 1–10) : Intro, bilan 75/75, architecture 3 passes, multi-cible, équipe
- **P2** (slides 11–20) : Grammaire ANTLR4, règles, priorités opérateurs, AST, erreurs syntaxiques
- **P3** (slides 21–30) : SymbolTableVisitor, scopes, shadowing, renommage variables, vérifications sémantiques
- **P4** (slides 31–42) : IR 3 adresses, structure IRInstr, CFG, BasicBlocks, exit_bb unique, exemple while en CFG
- **P5** (slides 43–54) : ExprValue, constant folding, constMap, propagation data-flow, court-circuit &&/||
- **P6** (slides 55–67) : ABI AMD64, prologue/épilogue, registres, alignement 16 octets, exemple Fibonacci source→asm
- **P7** (slides 68–82) : double+inférence de type, tableaux, switch/case, for/do-while, ternaire, ARM64
- **P8** (slides 83–92) : 75/75 tests graphique, démo game.c, gestion projet CI/CD, améliorations futures, conclusion

### 2. Fiche de révision par personne (8 fiches)

Pour chacune des 8 personnes, génère une fiche markdown `FICHE_P{N}.md` contenant :

**Structure de chaque fiche :**
```
# Fiche P{N} — {Sujet}

## Ce que tu dois expliquer (7 minutes)
[Liste des concepts à couvrir dans l'ordre]

## Déroulé minute par minute
- 0:00–1:00 : ...
- 1:00–2:30 : ...
- 2:30–4:00 : ...
- 4:00–5:30 : ...
- 5:30–7:00 : ...

## Les concepts clés à maîtriser
[Explications détaillées de chaque concept, comme si tu l'expliquais à quelqu'un qui ne connaît pas]

## Exemples de code à connaître
[Extraits de code source C et/ou assembleur à citer de mémoire]

## Questions pièges possibles du jury
[5–8 questions difficiles avec les réponses]

## Phrases d'accroche pour enchaîner avec le suivant
[Comment passer la parole naturellement à P{N+1}]
```

### 3. Fiche commune "À savoir absolument"

Un fichier `FICHE_COMMUNE.md` avec :
- Les 10 chiffres clés du projet (nb lignes de code, nb tests, nb instructions IR, etc.)
- Le vocabulaire technique à maîtriser (CFG, BasicBlock, IR, ABI, lvalue, rvalue, constant folding, etc.)
- Les questions générales que le jury peut poser à n'importe qui
- Les 5 points de design les plus originaux à défendre
- Ce qu'on n'a PAS fait et pourquoi (pointeurs, variables globales, etc.)

---

## CONTRAINTES DE FORME

- Les slides doivent être **visuels** : peu de texte, beaucoup de schémas décrits (CFG, pipeline, tableaux comparatifs)
- Chaque personne parle **exactement 7 minutes** : prévoir le minutage
- Le ton est **technique mais pédagogique** : le jury connaît la compilation mais pas notre implémentation spécifique
- Privilégier les **exemples concrets** : montrer du code C d'un côté, de l'assembleur de l'autre
- La démo (P8) doit être **scriptée** : commandes exactes à taper, sorties attendues

---

Génère maintenant dans l'ordre :
1. Le plan des 90 slides (slide par slide)
2. Les 8 fiches de révision (FICHE_P1 à FICHE_P8)
3. La fiche commune (FICHE_COMMUNE)
