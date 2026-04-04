/* tableau rempli par for, somme */
int main() {
    int a[10];
    for (int i = 0; i < 10; ++i) {
        a[i] = i + 1;  /* 1, 2, ..., 10 */
    }
    int somme = 0;
    for (int i = 0; i < 10; ++i) {
        somme += a[i];
    }
    /* 1+2+3+4+5+6+7+8+9+10 = 55 */
    return sum;
}
