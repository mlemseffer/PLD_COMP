/* 4.19 Arrays: expression as index */
int main() {
    int a[4];
    a[0] = 1;
    a[1] = 2;
    a[2] = 4;
    a[3] = 8;
    int i = 1;
    return a[i + 1];
}
