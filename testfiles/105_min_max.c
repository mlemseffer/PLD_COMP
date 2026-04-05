/* Min/max functions + array search */
int max3(int a, int b, int c) {
    int m = a;
    if (b > m) {
        m = b;
    }
    if (c > m) {
        m = c;
    }
    return m;
}

int min3(int a, int b, int c) {
    int m = a;
    if (b < m) {
        m = b;
    }
    if (c < m) {
        m = c;
    }
    return m;
}

int main() {
    int big = max3(17, 42, 8);    /* 42 */
    int small = min3(17, 42, 8);  /* 8  */
    return big - small;           /* 34 */
}
