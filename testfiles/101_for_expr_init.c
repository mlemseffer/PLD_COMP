/* for avec expression init */
int main() {
    int i;
    int somme = 0;
    for (i = 1; i <= 10; ++i) {
        somme += i;
    }
    /* 1+2+...+10 = 55 */
    /* i is still accessible after loop */
    return somme - i + 1; /* 55 - 11 + 1 = 45 */
}
