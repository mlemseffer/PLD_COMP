# Jeu de devinette - `game.c`

Programme de démonstration compilé avec **ifcc**, notre compilateur C maison.
Uniquement des fonctionnalités supportées par ifcc : pas de `printf`, pas de variables globales, pas de pointeurs.

---

## Lancer le jeu

### Compilation + exécution

```bash
# Depuis la racine du projet
compiler/ifcc testfiles/game.c > game.s   # génère l'assembleur
gcc game.s -o game                         # assemble + lie
./game                                     # joue !
```

### Mode automatique (pour les tests / démo)

```bash
# Deviner en 3 coups : 10 (trop petit), 80 (trop grand), 42 (gagné)
printf '10\n80\n42\n' | ./game
```

Sortie attendue :
```
=== Devinez (1-100) ===
> Trop petit !
> Trop grand !
> Bravo ! Trouve en 3 essais.
```

La valeur de retour du programme (`echo $?`) est le nombre d'essais.

---

## Règles du jeu

- Le programme choisit un nombre secret (ici : **42**).
- Le joueur saisit des entiers au clavier.
- Le programme répond `Trop petit !` ou `Trop grand !` jusqu'à trouver.
- À la victoire, il affiche le nombre d'essais et le retourne comme code de sortie.

---

## Fonctionnalités du compilateur illustrées

### 1. Fonctions et appels de fonctions

Le programme est découpé en **4 fonctions** :

| Fonction | Rôle |
|---|---|
| `nl()` | Affiche un saut de ligne |
| `print_int(n)` | Affiche un entier (récursif) |
| `read_int()` | Lit un entier au clavier |
| `check(guess, secret)` | Compare deux entiers, retourne -1 / 0 / 1 |
| `main()` | Boucle principale du jeu |

Chaque appel démontre la **convention d'appel System V AMD64** :
- Arguments passés dans `%edi`, `%esi`, ...
- Valeur de retour dans `%eax`
- Pile alignée sur 16 octets avant chaque `call`

---

### 2. Prologue / Epilogue et gestion de la pile

Chaque fonction génère :

```asm
pushq %rbp          ; sauvegarde du frame pointer de l'appelant
movq %rsp, %rbp     ; nouveau frame pointer pour cette fonction
subq $N, %rsp       ; réservation des variables locales (N multiple de 16)
...
movl -4(%rbp), %eax ; valeur de retour dans %eax
leave               ; restaure rsp et rbp
ret                 ; retour à l'appelant
```

Les variables locales sont toutes sur la pile, accessibles via des offsets négatifs par rapport à `%rbp`.

---

### 3. Boucle `while` et structure en blocs de base

Dans `read_int()` et `main()`, les boucles `while` se traduisent par **3 blocs de base** :

```
┌─────────────────────┐
│  .LBB_xxx_cond      │  ← évaluation de la condition
│  jmp corps / sortie │
└──────────┬──────────┘
           │ vrai
┌──────────▼──────────┐
│  .LBB_xxx_body      │  ← corps de la boucle
│  jmp .LBB_xxx_cond  │  ← retour inconditionnel à la condition
└─────────────────────┘
           │ faux
┌──────────▼──────────┐
│  .LBB_xxx_end       │  ← suite du programme
└─────────────────────┘
```

Assembleur correspondant à `while (guess != secret)` :
```asm
.LBB_main_1:                       ; début condition
    movl -8(%rbp), %eax            ; charge guess
    cmpl -12(%rbp), %eax           ; compare avec secret
    setne %al                      ; 1 si !=, 0 sinon
    movzbl %al, %eax
    cmpl $0, %eax
    je .LBB_main_3                 ; faux → sortie de boucle
    jmp .LBB_main_2                ; vrai → corps
.LBB_main_2:                       ; corps du while
    ...
    jmp .LBB_main_1                ; retour à la condition
.LBB_main_3:                       ; après la boucle
```

---

### 4. Évaluation paresseuse de `&&`

Dans `read_int()`, la condition `c >= '0' && c <= '9'` est compilée en **2 blocs séparés** avec court-circuit :

```asm
; Bloc 1 : c >= '0'  (ASCII 48)
movl $48, -44(%rbp)
movl -12(%rbp), %eax        ; charge c
cmpl -44(%rbp), %eax
setge %al                   ; 1 si c >= '0'
movzbl %al, %eax
movl %eax, -48(%rbp)
cmpl $0, -48(%rbp)
je .LBB_read_int_5          ; FAUX → court-circuit, skip la 2e condition

; Bloc 2 : c <= '9'  (ASCII 57), seulement évalué si c >= '0'
movl $57, -56(%rbp)
movl -12(%rbp), %eax        ; recharge c
cmpl -56(%rbp), %eax
setle %al                   ; 1 si c <= '9'
```

Si `c < '0'`, le second test n'est **jamais exécuté** → sémantique C respectée.

---

### 5. Recursion avec `print_int`

```c
void print_int(int n) {
    if (n < 0) { putchar('-'); n = -n; }
    if (n / 10 != 0) { print_int(n / 10); }  // appel récursif
    putchar(n % 10 + '0');
}
```

Pour afficher `42` :
1. `42 / 10 = 4 ≠ 0` → appel récursif `print_int(4)`
2. `4 / 10 = 0` → pas d'appel récursif, affiche `'4'`
3. Retour au niveau 1 → affiche `'2'`

Chaque appel crée un **nouveau frame** sur la pile. En assembleur, chaque `call print_int` empile `%rip` (adresse de retour) puis un nouveau `%rbp`.

---

### 6. Opérations arithmétiques

Dans `read_int()`, la ligne `n = n * 10 + (c - '0')` génère :

```asm
; n * 10
movl $10, -68(%rbp)
movl -8(%rbp), %eax        ; charge n
imull -68(%rbp), %eax      ; multiplication signée 32 bits
movl %eax, -72(%rbp)       ; stocke résultat temporaire

; c - '0'  (c - 48)
movl $48, -76(%rbp)
movl -12(%rbp), %eax        ; charge c
subl -76(%rbp), %eax        ; soustraction
movl %eax, -80(%rbp)

; addition finale
movl -72(%rbp), %eax
addl -80(%rbp), %eax        ; n*10 + (c-'0')
movl %eax, -84(%rbp)        ; stocke dans n
```

Montre : `imull` (multiplication), `subl` (soustraction), `addl` (addition), tout sur 32 bits.

---

### 7. `if / else if / else` et branchements conditionnels

Dans `main()`, le `if (result == -1) ... else if (result == 1)` génère :

```asm
cmpl $-1, -16(%rbp)         ; result == -1 ?
setne %al
... 
je .LBB_main_trop_petit     ; oui → branche "Trop petit"
jmp .LBB_main_else          ; non → else

.LBB_main_else:
cmpl $1, -16(%rbp)          ; result == 1 ?
je .LBB_main_trop_grand
jmp .LBB_main_fin_if
```

Chaque branche `if/else` devient un **bloc de base** avec un saut conditionnel en entrée.

---

### 8. Valeur de retour de `main`

```c
return tries;
```

Le nombre d'essais est retourné dans `%eax` et devient le **code de sortie du processus** :

```bash
./game <<< "42"
echo $?   # affiche 1 (trouvé en 1 essai)
```

Attention : le shell traite la valeur de retour comme un entier non signé 8 bits (0–255).

---

## Vue d'ensemble du graphe de flot de contrôle

```
main()
  │
  ├─ prologue (init secret=42, tries=0, guess=0)
  │
  ▼
[while guess != secret] ──faux──► [épilogue → return tries]
  │ vrai
  ▼
[read_int()] ──────────────────────────────────────────────┐
  │                                                         │
  │  [while c in '0'..'9']  ─faux─► [return n]            │
  │    │ vrai                                               │
  │    └─► n = n*10 + (c-'0'), c = getchar()               │
  │        └─────────────────────────────────► [retour] ───┘
  │
  ▼
[check(guess, secret)] → résultat: -1, 0, ou 1
  │
  ├─ -1 → affiche "Trop petit !"
  ├─  1 → affiche "Trop grand !"
  └─  0 → (while se termine au prochain test)
```

---

## Pourquoi ce programme est un bon exemple

| Ce qu'on montre | Où dans le code |
|---|---|
| Prologue/épilogue | Toutes les fonctions |
| Convention d'appel (registres) | Appels `putchar`, `getchar`, `check` |
| Boucle while → blocs de base + sauts | `read_int`, `main` |
| Court-circuit `&&` | Condition de `read_int` |
| Récursion + pile | `print_int` |
| Arithmétique (`imull`, `addl`, `subl`) | Corps de `read_int` |
| `if/else` → branchements | `main`, `check` |
| Valeur de retour entière | `return tries` dans `main` |
