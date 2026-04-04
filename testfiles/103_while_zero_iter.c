/* while with condition false from start: body never executes */
int main() {
    int x = 99;
    while (0) {
        x = 0; /* never reached */
    }
    int y = 42;
    while (y > 100) {
        y = 0; /* never reached */
    }
    return x + y; /* 99 + 42 = 141, but 141 mod 256 = 141 */
}
