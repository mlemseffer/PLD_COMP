// Test: continue in for loop - sum even numbers 0..9
// 0+2+4+6+8 = 20
int main() {
    int s = 0;
    for (int i = 0; i < 10; i++) {
        if (i % 2 != 0) continue;
        s += i;
    }
    return s;
}
