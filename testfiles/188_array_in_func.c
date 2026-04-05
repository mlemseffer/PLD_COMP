// Test: local array inside a function - should return 60
int compute() {
    int a[3];
    a[0] = 10;
    a[1] = 20;
    a[2] = 30;
    return a[0] + a[1] + a[2];
}
int main() {
    return compute();
}
