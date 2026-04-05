/* Constant folding on comparisons at compile time */
int main() {
    /* These should all be folded at compile time */
    int a = 5 > 3;    /* 1 */
    int b = 5 < 3;    /* 0 */
    int c = 5 == 5;   /* 1 */
    int d = 5 != 5;   /* 0 */
    int e = 3 <= 3;   /* 1 */
    int f = 4 >= 5;   /* 0 */
    /* Also test with propagated constants */
    int x = 10;
    int y = 10;
    int g = x == y;   /* 1, x and y both known at compile time */
    return a + b + c + d + e + f + g; /* 1+0+1+0+1+0+1 = 4 */
}
