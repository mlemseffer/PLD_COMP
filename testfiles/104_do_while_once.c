/* do-while s'execute tjs une fois */
int main() {
    int x = 0;
    int count = 0;
    do {
        x += 10;
        count++;
    } while (x < 0); /* exec au moins une fois */
    /* x = 10, count = 1 */
    return x + count; /* 11 */
}
