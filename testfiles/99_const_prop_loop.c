/* propagation de constantes complexe */
int main() {
    int a = 3;
    int b = a + 2;  /* b = 5 (propagated from a=3) */
    int c = b * b;  /* c = 25 (propagated) */
    /* x invalidee car modifiee dans la boucle */
    int x = 10;
    int i = 0;
    while (i < 3) {
        x += i;
        i++;
    }
    /* x = 10+0+1+2 = 13 (not compile-time constant, runtime value) */
    /* c is still 25 (not touched by loop) */
    return c + x; /* 25 + 13 = 38 */
}
