// Test: array element overwrite - should return 2
int main() {
    int a[1];
    a[0] = 1;
    a[0] = 2;
    return a[0];
}
