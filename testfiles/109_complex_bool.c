/* Complex boolean expressions combining &&, ||, !, comparisons */
int between(int x, int lo, int hi) {
    return x >= lo && x <= hi;
}

int is_digit(int c) {
    return c >= '0' && c <= '9';
}

int is_letter(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int main() {
    int a = between(5, 1, 10);    /* 1 */
    int b = between(15, 1, 10);   /* 0 */
    int c = is_digit('7');        /* 1 */
    int d = is_digit('x');        /* 0 */
    int e = is_letter('k');       /* 1 */
    int f = is_letter('3');       /* 0 */
    return a + b + c + d + e + f; /* 3 */
}
