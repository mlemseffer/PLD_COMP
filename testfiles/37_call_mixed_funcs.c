#include <stdio.h>

int returnConstant() {
    return 100;
}

int add(int a, int b) {
    return a + b;
}

int addFour(int a, int b, int c, int d) {
    int sum1 = add(a, b);
    int sum2 = add(c, d);
    return add(sum1, sum2);
}

int main() {
    int v1 = returnConstant();
    int v2 = add(50, 50);
    int v3 = addFour(10, 20, 30, 40); /* 100 */
    
    /* Total should be 300 */
    return v1 + v2 + v3;
}
