// Test: && short-circuit evaluation
// 0 && (b=1) should not evaluate right side, b stays 0
// result = a*10+b = 0*10+0 = 0
int main() {
    int a = 0;
    int b = 0;
    if (0 && (b = 1)) {
        a = 1;
    }
    return a * 10 + b;
}
