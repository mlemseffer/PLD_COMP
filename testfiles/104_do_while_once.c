/* do-while: body always runs at least once even if condition is initially false */
int main() {
    int x = 0;
    int count = 0;
    do {
        x += 10;
        count++;
    } while (x < 0); /* condition is false immediately, but body ran once */
    /* x = 10, count = 1 */
    return x + count; /* 11 */
}
