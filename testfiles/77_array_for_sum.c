/* Array filled with for loop, sum all elements */
int main() {
    int a[10];
    for (int i = 0; i < 10; i++) {
        a[i] = i + 1;  /* 1, 2, ..., 10 */
    }
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += a[i];
    }
    /* 1+2+3+4+5+6+7+8+9+10 = 55 */
    return sum;
}
