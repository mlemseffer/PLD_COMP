/* arithmetique double iterative */
int main() {
    /* calcule 1024 via des doubles */
    double val = 1.0;
    int i = 0;
    while (i < 10) {
        val = val * 2.0;
        ++i;
    }
    int result = val; /* tronque: 1024, but mod 256 = 0 ... */
    /* exp plus petit */
    double val2 = 1.0;
    int j = 0;
    while (j < 7) {
        val2 = val2 * 2.0;
        ++j;
    }
    int r2 = val2; /* 128 */
    return r2;     /* 128 */
}
