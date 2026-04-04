/* recursion croisee */
/* les fonctions sont enregistrees en passe 1 */
int is_odd(int n) {
    if (n == 0) {
        return 0;
    }
    return is_even(n - 1);
}

int is_even(int n) {
    if (n == 0) {
        return 1;
    }
    return is_odd(n - 1);
}

int main() {
    int a = is_even(4); /* 1 */
    int b = is_even(7); /* 0 */
    int c = is_odd(3);  /* 1 */
    int d = is_odd(6);  /* 0 */
    return a + b * 2 + c * 4 + d * 8; /* 1+0+4+0 = 5 */
}
