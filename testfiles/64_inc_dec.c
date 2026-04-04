int main() {
    int a = 5;
    int b = ++a;  // a=6, b=6
    int c = a;    // c=6
    --a;          // a=5 (statement)
    a++;          // a=6 (statement)
    a--;          // a=5 (statement)
    return a + b + c; // 5 + 6 + 6 = 17
}
