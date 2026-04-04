/* Chained unary minus and logical not */
int main() {
    int x = 5;
    int a = -(-x);      /* 5 */
    int b = !(!x);      /* !!5 -> !0 -> 1 */
    int c = !x;         /* 0 */
    int d = !(x == 5);  /* !(1) -> 0 */
    int e = !(x != 5);  /* !(0) -> 1 */
    int f = -x + x;     /* 0 */
    return a + b + c + d + e + f; /* 5+1+0+0+1+0 = 7 */
}
