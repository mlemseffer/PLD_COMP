/* Sum of digits of a number */
int sum_digits(int n) {
    int s = 0;
    while (n > 0) {
        s += n % 10;
        n = n / 10;
    }
    return s;
}

int main() {
    int a = sum_digits(12345); /* 1+2+3+4+5 = 15 */
    int b = sum_digits(9999);  /* 9+9+9+9 = 36 */
    int c = sum_digits(100);   /* 1+0+0 = 1 */
    return a + b + c;          /* 52 */
}
