int main() {
    int a = 1;
    int b = 0;
    int c = 5;

    int r1 = a && c;   // 1 && 5 -> 1
    int r2 = a && b;   // 1 && 0 -> 0
    int r3 = b || c;   // 0 || 5 -> 1
    int r4 = b || b;   // 0 || 0 -> 0

    return r1 * 8 + r2 * 4 + r3 * 2 + r4; // 8 + 0 + 2 + 0 = 10
}
