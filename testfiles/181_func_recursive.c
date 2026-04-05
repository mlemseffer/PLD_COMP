// Test: recursive function - sum(10) = 55
int sum(int n) {
    if (n <= 0) return 0;
    return n + sum(n - 1);
}
int main() {
    return sum(10);
}
