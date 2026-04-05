// Test: do-while executes at least once even with false condition
int main() {
    int x = 0;
    do {
        x = 42;
    } while (0);
    return x;
}
