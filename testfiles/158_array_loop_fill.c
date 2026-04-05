/* 4.19 Arrays: fill array in a loop and sum */
int main() {
    int a[5];
    int i = 0;
    while (i < 5) {
        a[i] = i * 2;
        i = i + 1;
    }
    return a[0] + a[1] + a[2] + a[3] + a[4];
}
