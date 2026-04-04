/* Fibonacci with for loop (iterative) */
int fibo_iter(int n) {
    if (n <= 1) {
        return n;
    }
    int a = 0;
    int b = 1;
    for (int i = 2; i <= n; ++i) {
        int tmp = a + b;
        a = b;
        b = tmp;
    }
    return b;
}

int main() {
    int f8 = fibo_iter(8);   /* fib(8) = 21 */
    int f10 = fibo_iter(10); /* fib(10) = 55 */
    return f8 + f10; /* 76 */
}
