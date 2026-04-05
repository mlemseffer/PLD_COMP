#include <stdio.h>

int my_sum_8(int a, int b, int c, int d, int e, int f, int g, int h) {
    return a + b + c + d + e + f + g + h;
}

int main() {
    int sum = my_sum_8(1, 2, 3, 4, 5, 6, 7, 8);
    putchar(sum + 44); /* 36 + 44 = 80 = 'P' */
    putchar(10);
    return 0;
}
