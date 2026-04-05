// Test: || short-circuit evaluation
// 1 || (b=1) should not evaluate right side, b stays 0
// a becomes 1, result = a*10+b = 10
int main() {
    int a = 0;
    int b = 0;
    if (1 || (b = 1)) {
        a = 1;
    }
    return a * 10 + b;
}
