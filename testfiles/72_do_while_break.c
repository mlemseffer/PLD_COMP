/* do-while continue/break */
int main() {
    int i = 0;
    int somme = 0;
    do {
        i++;
        if (i % 3 == 0) {
            continue;
        }
        if (i > 10) {
            break;
        }
        somme += i;
    } while (1);
    /* 1+2+4+5+7+8+10 = 37 */
    return sum;
}
