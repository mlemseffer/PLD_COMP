/* division/modulo sur negatifs */
int main() {
    int a = -10 / 3;   /* tronque vers zero */
    int b = 10 / -3;   /* -3 */
    int c = -10 % 3;   /* signe suit le dividende (C99) */
    int d = 10 % -3;   /* 1 */
    /* a + b = -6, c + d = 0, total = -6, return as absolute: */
    int result = 0 - (a + b); /* 6 */
    return result + (c + d);  /* 6 + 0 = 6 */
}
