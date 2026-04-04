# Fiche P3 — Analyse Sémantique (SymbolTableVisitor)

## Ce que tu dois expliquer (7 minutes)

1. Rôle du SymbolTableVisitor : passe 1 sur le CST
2. Structure de la table des symboles (pile de scopes)
3. Renommage des variables pour gérer les scopes et le shadowing
4. Les 3 vérifications sémantiques : non déclarée, double déclaration, non utilisée
5. Vérification des appels de fonctions (nb d'arguments)
6. Types supportés (int, char, double, void)

---

## Déroulé minute par minute

- **0:00–1:00** : Rôle du SymbolTableVisitor (slide 21)
  > "Après le parsing, on sait que le programme est syntaxiquement correct. Mais est-ce qu'il est sémantiquement valide ? On a une variable utilisée sans être déclarée ? Deux fois le même nom dans le même scope ? C'est le travail du SymbolTableVisitor."

- **1:00–2:30** : Table des symboles + scoping (slides 22–23)
  > Dessiner la pile de scopes. Montrer comment une variable `x` en scope 0 et `x` en scope 1 coexistent.

- **2:30–3:30** : Shadowing : exemple concret (slide 24)
  > Code C avec deux `x`. Montrer le renommage en `x_0` et `x_1` dans l'IR.

- **3:30–5:00** : Les 3 vérifications sémantiques (slides 25–27)
  > Variable non déclarée → erreur fatale. Double déclaration → erreur fatale. Non utilisée → warning. Montrer le flag `used`.

- **5:00–6:15** : Vérification des appels + types (slides 28–29)
  > Table des fonctions, vérification arity. Tableau des types.

- **6:15–7:00** : Transition (slide 30)
  > "Une fois la passe 1 terminée, le CST est propre, les variables ont leurs noms IR définitifs, les offsets sont calculés. P4 peut générer l'IR sans ambiguïté."

---

## Les concepts clés à maîtriser

### Table des symboles : pile de scopes
La table des symboles est une **pile de maps** :
```
stack< map<string, SymbolInfo> >
```
- Entrée dans un bloc `{` → `push` d'une nouvelle map
- Sortie du bloc `}` → `pop` (les variables locales disparaissent)
- Résolution d'un identifiant : chercher de haut en bas dans la pile

`SymbolInfo` contient :
- `type` : int, double, void
- `offset` : position sur la pile (pour gen_asm)
- `used` : booléen (pour le warning "déclarée non utilisée")
- `irName` : nom IR final (ex: `x_0`)

### Renommage des variables
**Problème** : deux variables `x` dans des scopes différents ont le même nom → collision dans la table du CFG (qui est plate).

**Solution** : suffixe `_N` où N = numéro de scope.
- `x` déclaré en scope 0 → `x_0`
- `x` déclaré en scope 1 (bloc imbriqué) → `x_1`

Le renommage est effectué **lors de l'entrée dans le scope**. Ensuite, toutes les références à `x` dans ce scope sont résolues vers `x_1`.

### Shadowing
Le shadowing est légal en C : une variable dans un sous-scope peut avoir le même nom qu'une variable du scope parent. Le sous-scope "cache" le parent pour ce nom.

Notre système le gère naturellement : la recherche dans la pile s'arrête dès qu'elle trouve le nom → elle trouve `x_1` avant `x_0`.

### Les 3 vérifications sémantiques
1. **Variable non déclarée** : à chaque lecture d'un identifiant, cherche dans la pile. Si absent → `error: 'x' undeclared`. Arrêt de la compilation.

2. **Double déclaration** : à chaque déclaration, vérifie dans le scope **courant seulement** (pas les parents). Si trouvé → `error: 'x' already declared`. Shadowing est autorisé.

3. **Déclarée non utilisée** : à la fermeture de chaque scope, pour chaque variable avec `used == false` → `warning: 'x' declared but not used`.

### Vérification des appels
Table des fonctions déclarées : `map<string, FuncInfo>` avec nombre de paramètres et types.

À chaque appel `f(a, b, c)` :
- Vérifie que `f` est déclarée
- Vérifie `nb_args == nb_params`
- Si mismatch → erreur

### Types
| Type C | Type IR | Instructions asm |
|--------|---------|-----------------|
| `int` | INT | `movl`, `addl`, ... |
| `char` | INT (traité comme int) | `movl` |
| `double` | DOUBLE | `movsd`, `addsd`, ... |
| `void` | — | — |

`char` est simplifié : on le traite comme un `int` 32 bits. Pas de sémantique octet séparée.

---

## Exemples de code à connaître

### Table des symboles pour ce code
```c
int f(int x) {           // scope 0 : x → x_0
    int y = x + 1;       // scope 0 : y → y_0
    {
        int x = y * 2;   // scope 1 : x → x_1 (shadow de x_0)
        return x;        // résout vers x_1
    }
    return y;            // résout vers y_0
}
```

Pile de scopes à l'entrée du bloc imbriqué :
```
Scope 1 : { x_1 → {type:int, offset:-12, used:false} }
Scope 0 : { x_0 → {type:int, offset:-4, used:true},
            y_0 → {type:int, offset:-8, used:false} }
```

### Vérification de non-déclaration
```c
int main() {
    return z;  // ERROR: 'z' undeclared
}
```
→ `error: variable 'z' not declared in this scope`

### Double déclaration
```c
int main() {
    int x = 1;
    int x = 2;  // ERROR
}
```
→ `error: variable 'x' already declared in this scope`

### Shadowing (légal)
```c
int x = 1;
{
    int x = 2;  // OK : scope différent → x_1
}
```

### Vérification d'appel
```c
int add(int a, int b) { return a + b; }
int main() {
    return add(1, 2, 3);  // ERROR: 3 args, 2 expected
}
```

---

## Questions pièges possibles du jury

**Q : Comment gérez-vous le shadowing de paramètres de fonctions ?**
R : Les paramètres sont déclarés dans le scope de la fonction (scope 0). Un bloc imbriqué peut déclarer une variable du même nom → nouveau scope → renommage différent. Ça fonctionne exactement comme pour les variables locales.

**Q : Pourquoi renommer à l'IR plutôt que d'utiliser des pointeurs/offsets directement ?**
R : Le renommage permet à la table des symboles du CFG d'être plate (une seule map `name → offset`). C'est plus simple pour gen_asm qui n'a pas à retrouver le scope. L'offset est stocké dans `SymbolInfo` et associé au nom IR unique.

**Q : Que se passe-t-il si on utilise une variable dans sa propre initialisation ?**
R : En C, `int x = x + 1;` est un comportement indéfini. Notre compilateur ne le détecte pas explicitement — il résoudrait `x` vers la variable du scope parent (si elle existe) ou signalerait une erreur "not declared". Ce n'est pas un cas couvert par nos tests.

**Q : Vérifiez-vous les types des arguments dans les appels ?**
R : On vérifie le nombre d'arguments. Pour les types, on fait de la promotion implicite `int → double` si nécessaire. On ne lève pas d'erreur sur un mismatch int/double — on insère une conversion.

**Q : Comment calculez-vous les offsets sur la pile ?**
R : Chaque variable locale est allouée séquentiellement : première variable à `-4(%rbp)`, deuxième à `-8(%rbp)`, etc. La taille dépend du type (4 octets pour int, 8 pour double). L'offset total détermine `subq $N, %rsp` dans le prologue.

**Q : Le SymbolTableVisitor modifie-t-il l'arbre ?**
R : Non, il ne modifie pas le CST. Il construit une table externe et remplit les informations (offset, irName). Ces informations sont ensuite utilisées par l'IRGenVisitor lors de la deuxième traversée.

**Q : Que faire si une fonction est appelée avant d'être déclarée ?**
R : On fait une pré-passe pour collecter toutes les déclarations de fonctions avant de vérifier les appels. Ainsi, les appels forward (appeler `f` avant de déclarer `f`) sont supportés.

---

## Phrases d'accroche pour enchaîner avec P4

> "Maintenant qu'on a un programme sémantiquement valide et une table des symboles complète, on peut générer la représentation intermédiaire. C'est l'étape la plus riche du compilateur. [P4] va vous expliquer notre IR et le CFG."

> "La passe sémantique nous garantit un programme correct. La prochaine question : comment représenter ce programme de manière indépendante de la machine ? [P4] vous présente notre IR à 3 adresses et le CFG."
