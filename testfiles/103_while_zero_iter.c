/* while non-exec */
int add(int a, int b) {
    return a + b;
}

int main() {
    /* pas de constant folding, valeurs issues de fonctions */
    int small = add(3, 4);   /* 7 */
    int big = add(50, 50);   /* 100 */
    int count = 0;
    while (small > big) {    /* 7 > 100 → false, never executes */
        count = count + 1;
    }
    /* count stays 0, small stays 7, big stays 100 */
    return small + count; /* 7 + 0 = 7 */
}
