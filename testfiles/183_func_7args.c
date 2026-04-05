// Test: function with 7 arguments (>6, requires stack passing on x86-64)
// 1+2+3+4+5+6+7 = 28
int f(int a, int b, int c, int d, int e, int f_, int g) {
    return a + b + c + d + e + f_ + g;
}
int main() {
    return f(1, 2, 3, 4, 5, 6, 7);
}
