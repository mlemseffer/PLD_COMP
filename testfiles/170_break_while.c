// Test: break in while loop - should return 5
int main() {
    int i = 0;
    while (1) {
        if (i == 5) break;
        i++;
    }
    return i;
}
