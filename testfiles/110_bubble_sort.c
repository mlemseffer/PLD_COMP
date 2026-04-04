/* Bubble sort on an array */
int main() {
    int a[6];
    a[0] = 64;
    a[1] = 34;
    a[2] = 25;
    a[3] = 12;
    a[4] = 22;
    a[5] = 11;

    /* Bubble sort */
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5 - i; ++j) {
            if (a[j] > a[j + 1]) {
                int tmp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = tmp;
            }
        }
    }
    /* sorted: 11, 12, 22, 25, 34, 64 */
    /* check: a[0]=11, a[5]=64 */
    return a[0] + a[5]; /* 11 + 64 = 75 */
}
