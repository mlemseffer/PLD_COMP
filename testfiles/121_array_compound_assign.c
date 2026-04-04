/* Compound assign operators on array elements */
int main() {
    int a[5];
    for (int i = 0; i < 5; ++i) {
        a[i] = i * 10; /* 0, 10, 20, 30, 40 */
    }
    a[0] += 5;   /* 5 */
    a[1] -= 3;   /* 7 */
    a[2] *= 2;   /* 40 */
    a[3] /= 5;   /* 6 */
    a[4] %= 7;   /* 5 */
    /* sum: 5+7+40+6+5 = 63 */
    int sum = 0;
    for (int i = 0; i < 5; ++i) {
        sum += a[i];
    }
    return sum;
}
