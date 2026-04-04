# Fiche P2 — Grammaire ANTLR4

## Ce que tu dois expliquer (7 minutes)

1. Pourquoi ANTLR4 (parser generator vs parser à la main)
2. Structure du fichier `ifcc.g4`
3. Gestion des priorités d'opérateurs via l'ordre des alternatives
4. Passage du code source à l'arbre (CST) → exemple concret
5. Pattern Visitor vs Listener : pourquoi on a choisi Visitor
6. Compatibilité ANTLR 4.9 / 4.10+ : wrapper `castAny`

---

## Déroulé minute par minute

- **0:00–1:00** : Pourquoi ANTLR4 ? (slides 11–12)
  > "Un compilateur commence par lire et comprendre la syntaxe. On aurait pu écrire un parser récursif à la main. On a préféré ANTLR4 pour une raison simple : la grammaire est déclarative, lisible, et ANTLR4 génère le code C++ automatiquement."

- **1:00–2:30** : Règles d'expression et priorités (slides 13–14)
  > Montrer la règle `expr` avec les alternatives dans l'ordre. Expliquer que l'ordre = priorité.

- **2:30–3:30** : Du code source à l'AST (slide 15)
  > Exemple concret : `x * 2 + 1` → arbre. Dessiner l'arbre si possible.

- **3:30–5:00** : Déclarations, affectations, structures de contrôle (slides 16–17)
  > Montrer les règles clés : `varDecl`, `ifStmt`, `whileStmt`, tableaux.

- **5:00–6:00** : Visitor vs Listener, gestion des erreurs (slides 18–19)
  > Expliquer pourquoi Visitor : on a besoin de retourner des valeurs (ExprValue, noms IR) à chaque nœud.

- **6:00–7:00** : Wrapper `castAny`, transition (slide 20)
  > "Ce point technique nous a évité une heure de débogage cryptique entre deux versions d'ANTLR4."

---

## Les concepts clés à maîtriser

### Pourquoi un générateur de parsers ?
Écrire un parser récursif descendant pour un langage avec 40+ opérateurs et règles de priorité est long et source d'erreurs. ANTLR4 génère automatiquement le Lexer (tokens) et le Parser (règles syntaxiques) depuis une grammaire `.g4`.

### Priorités d'opérateurs dans ANTLR4
ANTLR4 gère les priorités par l'**ordre des alternatives** dans une règle récursive gauche :
- L'alternative la plus haute dans la règle est celle avec la priorité la PLUS FORTE
- `*` avant `+` → alternative `mul` définie avant alternative `add`

```antlr
expr
  : expr ('*'|'/'|'%') expr   # mulDivMod  ← priorité forte
  | expr ('+'|'-') expr       # addSub
  | expr ('<'|'>'|...) expr   # comparison
  | '-' expr                  # unaryMinus
  | IDENTIFIER                # varExpr
  | INT_LITERAL               # intLit
  ;
```

### Pattern Visitor
ANTLR4 offre deux patterns :
- **Listener** : événements entrée/sortie, pas de valeur de retour → utile pour parcours simple
- **Visitor** : méthodes `visitXxx()` qui retournent une valeur et contrôlent la descente → notre choix

On a besoin du Visitor parce que chaque `visitExpr*()` retourne un `ExprValue` (valeur constante ou nom de variable IR).

### Wrapper castAny
```cpp
template<typename T>
T castAny(antlrcpp::Any val) {
#if ANTLR4_MINOR >= 10
    return std::any_cast<T>(val);
#else
    return val.as<T>();
#endif
}
```
ANTLR 4.9 utilisait `antlrcpp::Any`, ANTLR 4.10+ utilise `std::any`. Ce wrapper évite d'avoir des `#ifdef` partout dans le code métier.

### CST vs AST
- **CST** (Concrete Syntax Tree) : garde tous les tokens, y compris `;`, `{`, `}`
- **AST** (Abstract Syntax Tree) : ne garde que les nœuds utiles sémantiquement
- ANTLR4 produit un CST, mais on le traite comme un AST en ignorant les tokens ponctuels dans nos méthodes Visitor

---

## Exemples de code à connaître

### Règle `expr` simplifiée
```antlr
expr
  : expr ('*'|'/'|'%') expr   # mulDivExpr
  | expr ('+'|'-') expr       # addSubExpr
  | expr ('=='|'!=') expr     # eqExpr
  | '!' expr                  # notExpr
  | '-' expr                  # unaryMinusExpr
  | IDENTIFIER '(' exprList? ')' # callExpr
  | IDENTIFIER '[' expr ']'   # arrayAccessExpr
  | IDENTIFIER                # varExpr
  | INT_LITERAL               # intLiteralExpr
  | DOUBLE_LITERAL            # doubleLiteralExpr
  ;
```

### Règle `varDecl`
```antlr
varDecl
  : type IDENTIFIER ';'
  | type IDENTIFIER '=' expr ';'
  | type IDENTIFIER '[' INT_LITERAL ']' ';'
  ;
```

### Arbre pour `x * 2 + 1`
```
addSubExpr
├── mulDivExpr
│   ├── varExpr(x)
│   └── intLiteralExpr(2)
└── intLiteralExpr(1)
```

### Usage du Visitor
```cpp
antlrcpp::Any IRGenVisitor::visitAddSubExpr(ifccParser::AddSubExprContext* ctx) {
    ExprValue lhs = castAny<ExprValue>(visit(ctx->expr(0)));
    ExprValue rhs = castAny<ExprValue>(visit(ctx->expr(1)));
    if (lhs.isConstant && rhs.isConstant)
        return ExprValue{true, lhs.value + rhs.value};
    // émettre instruction IR add...
}
```

---

## Questions pièges possibles du jury

**Q : ANTLR4 gère-t-il l'associativité des opérateurs ?**
R : Oui. Par défaut, les règles récursives gauches sont associatives à gauche (`a - b - c` = `(a-b)-c`). Pour l'associativité droite (ex: affectation), on peut annoter `<assoc=right>`.

**Q : Votre grammaire est-elle ambiguë ?**
R : Non, pour les constructions qu'on supporte. ANTLR4 détecte les ambiguïtés et les signale. La gestion de l'associativité et des priorités par ordre d'alternatives élimine les ambiguïtés classiques.

**Q : Comment gérez-vous les erreurs syntaxiques ?**
R : ANTLR4 a un error listener par défaut. On l'a surchargé pour écrire les erreurs sur `stderr` avec un message propre, puis on arrête la compilation avec un code retour d'erreur.

**Q : Pourquoi ne pas avoir utilisé Bison/Flex plutôt qu'ANTLR4 ?**
R : Bison génère des parsers LALR(1) en C. ANTLR4 génère des parsers LL(*) en C++, avec un Visitor orienté objet directement utilisable. Pour un projet C++, ANTLR4 s'intègre plus naturellement.

**Q : Qu'est-ce qu'un LL(*) par rapport à un LL(k) ?**
R : LL(k) regarde k tokens à l'avance. LL(*) regarde autant de tokens que nécessaire pour lever toute ambiguïté. ANTLR4 adapte automatiquement le lookahead selon la grammaire.

**Q : Avez-vous eu des conflits de règles dans votre grammaire ?**
R : Le principal piège était `if-else` : quelle branche `else` appartient à quel `if` ? C'est le "dangling else". On l'a résolu en appariant `else` au `if` le plus proche, ce qu'ANTLR4 fait naturellement par son algorithme LL(*).

**Q : Le Visitor peut-il provoquer une StackOverflow sur du code très imbriqué ?**
R : Théoriquement oui : chaque appel récursif à `visit()` empile une frame. En pratique, pour les programmes de test (quelques centaines de lignes), ce n'est pas un problème. Un compilateur industriel utiliserait une pile explicite.

---

## Phrases d'accroche pour enchaîner avec P3

> "On sait maintenant parser le code C et produire un arbre. Mais parser ne suffit pas : il faut vérifier que le programme est **sémantiquement** correct. Est-ce que toutes les variables sont déclarées ? Y a-t-il des conflits de noms ? C'est ce que [P3] va vous expliquer avec le SymbolTableVisitor."

> "La grammaire nous donne la forme du programme. La prochaine étape, c'est en vérifier le sens. [P3] prend la suite avec l'analyse sémantique."
