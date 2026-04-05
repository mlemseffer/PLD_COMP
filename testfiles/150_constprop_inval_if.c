/* 4.16 Constant propagation: invalidated by if branch */
int main() {
    int x = 5;
    if (1) {
        x = 10;
    }
    return x;
}
