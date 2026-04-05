// Test: break in for loop - should return 7
int main() {
    int r = 0;
    for (int i = 0; i < 100; i++) {
        if (i == 7) {
            r = i;
            break;
        }
    }
    return r;
}
