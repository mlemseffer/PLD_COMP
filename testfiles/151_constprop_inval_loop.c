/* 4.16 Constant propagation: invalidated by loop */
int main() {
    int x = 1;
    int i = 0;
    while (i < 3) {
        x = x + 1;
        i = i + 1;
    }
    return x;
}
