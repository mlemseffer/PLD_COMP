/* while et continue */
int main() {
    int i = 0;
    int somme = 0;
    while (i < 10) {
        i++;
        if (i % 2 != 0) {
            continue;
        }
        somme += i;
    }
    /* 2 + 4 + 6 + 8 + 10 = 30 */
    return sum;
}
