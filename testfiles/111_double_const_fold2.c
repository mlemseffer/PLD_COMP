/* Double constant folding: verify compile-time computation */
int main() {
    /* These should all be folded at compile time (both operands constant) */
    double a = 3.0 + 4.0;    /* 7.0 */
    double b = 10.0 - 2.5;   /* 7.5 */
    double c = 2.0 * 3.0;    /* 6.0 */
    double d = 15.0 / 2.0;   /* 7.5 */
    /* Mixed constant folding */
    double e = 3.0 * 3.0 + 4.0 * 4.0; /* 9.0 + 16.0 = 25.0 */
    int ia = a;   /* 7 */
    int ib = b;   /* 7 */
    int ic = c;   /* 6 */
    int id = d;   /* 7 */
    int ie = e;   /* 25 */
    return ia + ib + ic + id + ie; /* 7+7+6+7+25 = 52 */
}
