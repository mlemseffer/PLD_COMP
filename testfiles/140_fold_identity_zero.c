/* 4.9 Constant folding: identity elimination (x+0, x*1) */
int main() {
    int x = 42;
    return x + 0 + x * 1;
}
