// Test: constant folding in if condition - (1>2) is false, dead code elimination
int main() {
    if (1 > 2) return 1;
    return 0;
}
