# Fiche P5 — Optimisations (Constant Folding & Propagation de Constantes)

## Ce que tu dois expliquer (7 minutes)

1. Vue d'ensemble : deux optimisations, réalisées pendant la génération IR
2. Structure `ExprValue` : comment propager l'information de constance
3. Constant folding : calculer `2+3*4` à la compilation
4. `constMap` : propagation de constantes pour les variables
5. Gestion des branches if et boucles while (invalidation conservative)
6. `collectAssignedVars()` : comment on détecte les variables modifiées
7. Court-circuit `&&` et `||` : optimisation de flot de contrôle

---

## Déroulé minute par minute

- **0:00–1:00** : Vue d'ensemble + ExprValue (slides 43–44)
  > "On fait deux types d'optimisations : le constant folding (calculer les expressions constantes à la compilation) et la propagation de constantes (savoir qu'une variable vaut toujours X à un point donné). La clé : la structure ExprValue."

- **1:00–2:30** : Constant folding : principe + implémentation (slides 45–46)
  > Montrer `2+3*4 → 14`. Montrer le code du visiteur. Insister sur la récursivité.

- **2:30–4:00** : constMap + propagation (slides 47–48)
  > Exemple avec `int x=5; int y=x+1;`. Montrer que `y` est calculé à 6 directement.

- **4:00–5:15** : Propagation dans les branches et boucles (slides 49–50)
  > Expliquer `collectAssignedVars()`. Montrer l'approche conservative : si modifié dans l'une des branches → invalidé.

- **5:15–6:15** : Court-circuit `&&` et `||` (slides 51–52)
  > Montrer le CFG en 2 blocs. Expliquer pourquoi c'est important (évite la division par zéro, etc.).

- **6:15–7:00** : Résultats + limites + transition (slides 53–54)

---

## Les concepts clés à maîtriser

### Structure ExprValue
```cpp
struct ExprValue {
    bool isConstant;   // vraie constante connue à la compilation ?
    int  value;        // valeur entière si isConstant && type==INT
    double dvalue;     // valeur double si isConstant && type==DOUBLE
    string varName;    // nom de la variable IR si !isConstant
    Type type;         // INT ou DOUBLE
};
```

Chaque `visitExpr*()` retourne un `ExprValue`. Si la valeur est connue à la compilation → `isConstant=true, value=...`. Sinon → `isConstant=false, varName="!tmp0"`.

**Point clé** : si les deux opérandes d'un `visitAddExpr` sont constants → on calcule directement et on retourne un `ExprValue` constant, **sans émettre aucune instruction IR**.

### Constant Folding
```cpp
antlrcpp::Any visitAddExpr(ctx) {
    ExprValue lhs = castAny<ExprValue>(visit(ctx->expr(0)));
    ExprValue rhs = castAny<ExprValue>(visit(ctx->expr(1)));
    if (lhs.isConstant && rhs.isConstant)
        return ExprValue{true, lhs.value + rhs.value};
    // Sinon, émettre une instruction IR
    string tmp = newTmp();
    cfg->current_bb->add_IRInstr(IRInstr::op::add, INT, {tmp, lhs.varName, rhs.varName});
    return ExprValue{false, 0, 0.0, tmp, INT};
}
```

Ceci fonctionne récursivement pour toute profondeur d'expression :
- `2 + 3 * 4` : d'abord `3*4=12`, puis `2+12=14` → une seule `ldconst 14`

### constMap : propagation de constantes
```cpp
map<string, int> constMap;  // varName IR → valeur entière connue
```

À chaque affectation :
- Si la RHS est constante → `constMap[varName] = value`
- Si la RHS n'est pas constante → `constMap.erase(varName)` (invalider)

À chaque lecture d'une variable :
- Si `constMap.count(varName)` → créer un `ExprValue` constant avec la valeur
- Sinon → créer un `ExprValue` avec le `varName`

**Exemple** :
```c
int x = 5;        // constMap["x_0"] = 5
int y = x + 3;    // x_0 est constant (5) → folding → y_0 = 8
int z = y * 2;    // y_0 est constant (8) → folding → z_0 = 16
```
→ Aucune instruction arithmétique générée !

### Gestion des branches : approche conservative
**Problème** : après un `if/else`, on ne sait plus si une variable est constante (elle peut avoir été modifiée dans l'une des branches).

**Solution** : avant de visiter chaque branche, on collecte les variables modifiées dans **toutes** les branches, et on les invalide dans `constMap`.

```cpp
// Avant de visiter if/else :
auto modified = collectAssignedVars(ctx->bloc(0));   // then
modified += collectAssignedVars(ctx->bloc(1));       // else
for (auto& v : modified) constMap.erase(v);
// Maintenant, visiter les branches
```

C'est **conservatif** : on invalide même si la variable n'est modifiée que dans une branche. On perd quelques opportunités d'optimisation, mais on ne génère jamais de code incorrect.

### collectAssignedVars()
```cpp
set<string> collectAssignedVars(tree::ParseTree* node) {
    set<string> result;
    // Si c'est une affectation (lhsExpr '=' expr)
    //   → ajouter le nom de la variable gauche
    // Sinon → récurser sur tous les enfants
    //          union des résultats
    return result;
}
```

Traversée pure de l'AST, sans effets de bord, pour collecter tous les noms assignés.

### Court-circuit && (lazy evaluation)
```c
if (a != 0 && b/a > 1) { ... }
```

Sans court-circuit : `b/a` est toujours évalué → division par zéro si `a==0`.
Avec court-circuit : deux BasicBlocks séparés.

```
bb_left:
  cmp_neq !tmp0, a_0, 0
  test: !tmp0
    ↙ vrai → bb_right
    ↘ faux → bb_false

bb_right:
  div_int !tmp1 b_0 a_0
  cmp_gt !tmp2 !tmp1 1
  test: !tmp2
    ↙ vrai → bb_true
    ↘ faux → bb_false
```

### Court-circuit || (lazy evaluation)
```c
if (a == 0 || b > 0) { ... }
```
```
bb_left:
  cmp_eq !tmp0 a_0 0
  test: !tmp0
    ↙ vrai → bb_true   (court-circuité !)
    ↘ faux → bb_right

bb_right:
  cmp_gt !tmp1 b_0 0
  test: !tmp1 → bb_true / bb_false
```

---

## Exemples de code à connaître

### Constant folding profond
```c
int x = 2 + 3 * 4;  // → ldconst x_0 14

// Sans optimisation : 6 instructions
ldconst !tmp0 2
ldconst !tmp1 3
ldconst !tmp2 4
mul     !tmp3 !tmp1 !tmp2
add     !tmp4 !tmp0 !tmp3
copy    x_0 !tmp4

// Avec constant folding : 1 instruction
ldconst x_0 14
```

### Propagation de constantes en pratique
```c
int a = 10;
int b = 20;
int c = a + b;   // constMap: a=10, b=20 → c = 30 directement
putchar(c);      // ldconst c_0 30 → call putchar(30)
```

### Invalidation conservative
```c
int x = 5;        // constMap: x_0=5
if (cond) {
    x = 10;       // modifie x dans la branche then
}
// Après le if : constMap["x_0"] invalidé
// même si cond est toujours faux à l'exécution,
// on ne peut pas le savoir statiquement (en général)
int y = x + 1;  // x_0 non constant → instruction IR add
```

---

## Questions pièges possibles du jury

**Q : Votre propagation de constantes est-elle inter-procédurale ?**
R : Non, elle est intra-procédurale. On ne propage pas les constantes entre fonctions. Pour ça, il faudrait une analyse inter-procédurale, beaucoup plus complexe.

**Q : Pourquoi ne pas faire de folding des expressions booléennes (dead branch elimination) ?**
R : C'est une extension naturelle : si `if (1 < 2)` → on pourrait supprimer la branche `else`. On ne l'a pas implémenté, mais `constMap` pourrait être étendu pour ça.

**Q : Votre approche conservative ne rate-t-elle pas des opportunités d'optimisation ?**
R : Oui. Par exemple, si `x = 5` dans les deux branches then et else, on pourrait propager `x=5` après le if. `collectAssignedVars` invalide `x` dans les deux cas. C'est une simplification consciente.

**Q : Le constant folding fonctionne-t-il pour les doubles ?**
R : Oui. `ExprValue` a un champ `dvalue` pour les constantes double. Les opérations sur deux `ExprValue` double constants produisent un résultat constant double.

**Q : Pouvez-vous plier `x / 0` ?**
R : Non, on détecte le diviseur nul et on génère quand même l'instruction IR (pour ne pas changer le comportement indéfini — c'est au runtime de crasher). On aurait pu lever une erreur de compilation, mais ce n'est pas ce que fait GCC par défaut non plus.

**Q : Le court-circuit est-il requis par le standard C ?**
R : Oui, le standard C99 et C11 exigent que `&&` et `||` soient des opérateurs de court-circuit. C'est une garantie sémantique, pas juste une optimisation.

**Q : Quel est l'impact sur les performances du code généré ?**
R : Mesurable sur des expressions constantes lourdes. Pour du code général, l'impact principal est que tout est en mémoire (pas d'allocation de registres), ce qui domine le coût. Le folding réduit les stores/loads inutiles.

---

## Phrases d'accroche pour enchaîner avec P6

> "Vous avez vu comment on génère et on optimise l'IR. Il reste à la traduire en vrai assembleur. P6 va vous montrer comment on respecte l'ABI System V AMD64 pour générer un code x86-64 correct."

> "L'IR est optimisée. Maintenant le moment de vérité : la traduire en instructions x86-64, en respectant scrupuleusement les conventions d'appel. [P6] vous explique ça."
