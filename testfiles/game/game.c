/* =====================================================
   Jeu : Devinez le nombre secret (entre 1 et 100)
   Compile avec ifcc - utilise uniquement putchar/getchar
   ===================================================== */

/* --- Affichage --- */

void nl() {
    putchar('\n');
}

void print_int(int n) {
    if (n < 0) {
        putchar('-');
        n = -n;
    }
    if (n / 10 != 0) {
        print_int(n / 10);
    }
    putchar(n % 10 + '0');
}

/* --- Saisie --- */

int read_int() {
    int n;
    int c;
    n = 0;
    c = getchar();
    while (c >= '0' && c <= '9') {
        n = n * 10 + (c - '0');
        c = getchar();
    }
    return n;
}

/* --- Logique du jeu --- */

int check(int guess, int secret) {
    if (guess < secret) {
        return -1;
    }
    if (guess > secret) {
        return 1;
    }
    return 0;
}

int main() {
    int secret;
    int guess;
    int tries;
    int result;

    secret = 42;
    tries  = 0;
    guess  = 0;

    /* "=== Devinez le nombre (1-100) ===" */
    putchar('='); putchar('='); putchar('='); putchar(' ');
    putchar('D'); putchar('e'); putchar('v'); putchar('i');
    putchar('n'); putchar('e'); putchar('z'); putchar(' ');
    putchar('('); putchar('1'); putchar('-');
    putchar('1'); putchar('0'); putchar('0'); putchar(')');
    putchar(' '); putchar('='); putchar('='); putchar('=');
    nl();

    while (guess != secret) {
        putchar('>'); putchar(' ');
        guess  = read_int();
        tries  = tries + 1;
        result = check(guess, secret);

        if (result == -1) {
            /* "Trop petit !" */
            putchar('T'); putchar('r'); putchar('o'); putchar('p');
            putchar(' ');
            putchar('p'); putchar('e'); putchar('t'); putchar('i');
            putchar('t'); putchar(' '); putchar('!');
            nl();
        } else {
            if (result == 1) {
                /* "Trop grand !" */
                putchar('T'); putchar('r'); putchar('o'); putchar('p');
                putchar(' ');
                putchar('g'); putchar('r'); putchar('a'); putchar('n');
                putchar('d'); putchar(' '); putchar('!');
                nl();
            }
        }
    }

    /* "Bravo ! Trouve en X essais." */
    putchar('B'); putchar('r'); putchar('a'); putchar('v'); putchar('o');
    putchar(' '); putchar('!'); putchar(' ');
    putchar('T'); putchar('r'); putchar('o'); putchar('u'); putchar('v');
    putchar('e'); putchar(' '); putchar('e'); putchar('n'); putchar(' ');
    print_int(tries);
    putchar(' ');
    putchar('e'); putchar('s'); putchar('s'); putchar('a'); putchar('i');
    putchar('s'); putchar('.');
    nl();

    return tries;
}
