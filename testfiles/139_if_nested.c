/* 4.12 if/else: nested if */
int main() {
    int x = 5;
    if (x > 3) {
        if (x < 10) return 1;
        else return 2;
    }
    return 3;
}
