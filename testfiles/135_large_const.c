// Test: large constant - exit code is 8-bit, so 100000 % 256 = 160
int main() {
    int x = 100000;
    return x % 256;
}
