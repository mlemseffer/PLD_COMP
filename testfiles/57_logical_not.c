int main() {
    int a = !0; /* 1 */
    int b = !5; /* 0 */
    int c = !a; /* 0 */
    int d = !!10; /* 1 */

    int res = 0;
    if (!b) { /* if (1) */
        res = 42;
    }
    
    return res + d; /* 43 */
}
