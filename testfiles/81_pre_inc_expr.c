/* Pre-increment used directly in expressions */
int main() {
    int a = 5;
    int b = ++a + ++a;  /* a becomes 6, then 7; b = 6+7 = 13 */
    int c = a;          /* c = 7 */
    int d = --c;        /* c becomes 6, d = 6 */
    int e = a + d;      /* 7 + 6 = 13 */
    return e;           /* 13 */
}
