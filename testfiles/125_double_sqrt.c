/* Double arithmetic in a while loop (iterative computation) */
int main() {
    /* Compute 2^10 = 1024 using repeated doubling with doubles */
    double val = 1.0;
    int i = 0;
    while (i < 10) {
        val = val * 2.0;
        ++i;
    }
    int result = val; /* truncate: 1024, but mod 256 = 0 ... */
    /* Use smaller exponent: 2^7 = 128 */
    double val2 = 1.0;
    int j = 0;
    while (j < 7) {
        val2 = val2 * 2.0;
        ++j;
    }
    int r2 = val2; /* 128 */
    return r2;     /* 128 */
}
