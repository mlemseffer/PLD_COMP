/* Extra: short-circuit && (right not evaluated when left is 0) */
int main() {
    int a = 0;
    int b = 0;
    if (0 && (b = 1)) {
        a = 1;
    }
    return a * 10 + b;
}
