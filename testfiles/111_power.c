/* Power function: recursive and iterative */
int pow_iter(int base, int exp) {
    int result = 1;
    while (exp > 0) {
        result *= base;
        exp--;
    }
    return result;
}

int pow_rec(int base, int exp) {
    if (exp == 0) {
        return 1;
    }
    return base * pow_rec(base, exp - 1);
}

int main() {
    int a = pow_rec(2, 6);   /* 64 */
    int b = pow_iter(3, 3);  /* 27 */
    int same = pow_rec(4, 3) == pow_iter(4, 3); /* 1 (both 64) */
    return a + b - same; /* 64 + 27 - 1 = 90 */
}

