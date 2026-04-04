/* ternaire avec effets de bord */
int double_it(int x) {
    return x * 2;
}

int negate(int x) {
    return 0 - x;
}

int main() {
    int cond = 1;
    /* seule branche true evaluee */
    int a = cond ? double_it(5) : negate(5); /* 10 */
    cond = 0;
    int b = cond ? double_it(7) : negate(7); /* -7, but... */
    /* b = -7 causes problem for exit code, convert: */
    int b_abs = b < 0 ? 0 - b : b;           /* 7 */
    /* ternaire imbrique */
    int x = 15;
    int category = x < 10 ? 0 : x < 20 ? 1 : 2; /* 1 */
    return a + b_abs + category; /* 10 + 7 + 1 = 18 */
}
