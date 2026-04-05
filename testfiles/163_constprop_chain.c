/* 4.16 Constant propagation: chain of assignments */
int main() {
    int a = 10;
    int b = a;
    int c = b;
    return c;
}
