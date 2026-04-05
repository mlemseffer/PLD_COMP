#include <stdio.h>

int my_sum_10(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {
    return a + b + c + d + e + f + g + h + i + j;
}

int main() {
    /* sum = 55 */
    int sum = my_sum_10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    putchar(sum + 10); /* 55 + 10 = 65 = 'A' */
    putchar(10);
    return 0;
}
