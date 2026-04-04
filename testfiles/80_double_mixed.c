/* Mixed int/double comparisons and arithmetic */
int main() {
    double a = 5.5;
    int b = 3;
    /* int promoted to double for comparison */
    int r1 = a > b;   /* 5.5 > 3.0 -> 1 */
    int r2 = a < b;   /* 5.5 < 3.0 -> 0 */
    double c = a + b; /* 5.5 + 3.0 = 8.5 */
    int r3 = c == 8.5; /* 1 */
    double d = b * 2.0; /* 6.0 */
    int r4 = d < a;    /* 6.0 < 5.5 -> 0 */
    return r1 + r2 + r3 + r4; /* 1+0+1+0 = 2 */
}
