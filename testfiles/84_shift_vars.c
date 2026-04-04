/* shifts avec variables */
int main() {
    int x = 1;
    int n = 4;
    int a = x << n;    /* 1 << 4 = 16 */
    int b = 256 >> n;  /* 256 >> 4 = 16 */
    int c = a << 2;    /* 16 << 2 = 64 */
    int d = b >> 1;    /* 16 >> 1 = 8 */
    /* test decalage avec arithmetique */
    int e = (x << 3) + (x << 2); /* 8 + 4 = 12 */
    return a + b + c + d + e; /* 16+16+64+8+12 = 116 */
}
