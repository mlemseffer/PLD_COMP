/* Division and modulo with negative numbers (C99 truncates toward zero) */
int main() {
    int a = -10 / 3;   /* -3 (truncated toward zero) */
    int b = 10 / -3;   /* -3 */
    int c = -10 % 3;   /* -1 (sign follows dividend in C99) */
    int d = 10 % -3;   /* 1 */
    /* a + b = -6, c + d = 0, total = -6, return as absolute: */
    int result = 0 - (a + b); /* 6 */
    return result + (c + d);  /* 6 + 0 = 6 */
}
