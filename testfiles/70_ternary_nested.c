int abs_val(int x) {
    return x >= 0 ? x : -x;
}

int max(int a, int b) {
    return a > b ? a : b;
}

int main() {
    int a = abs_val(-42);   // 42
    int b = max(17, 42);    // 42
    return a + b;           // 84
}
