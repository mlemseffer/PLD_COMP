// Test: array indexed by expression/variable - should return 42
int main() {
    int a[5];
    int i = 2;
    a[i] = 42;
    return a[2];
}
