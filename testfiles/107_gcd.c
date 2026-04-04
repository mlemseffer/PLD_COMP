/* PGCD algorithme d'Euclide */
int gcd(int a, int b) {
    while (b != 0) {
        int tmp = b;
        b = a % b;
        a = tmp;
    }
    return a;
}

int main() {
    int a = gcd(48, 18);   /* gcd = 6 */
    int b = gcd(100, 75);  /* gcd = 25 */
    int c = gcd(17, 13);   /* gcd = 1 */
    return a + b + c;      /* 32 */
}
