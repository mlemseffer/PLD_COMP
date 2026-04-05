/* Extra: short-circuit || (right not evaluated when left is 1) */
int main() {
    int a = 0;
    int b = 0;
    if (1 || (b = 1)) {
        a = 1;
    }
    return a * 10 + b;
}
