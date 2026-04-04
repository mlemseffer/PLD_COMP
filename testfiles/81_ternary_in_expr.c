/* ternaire dans expressions complexes */
int max2(int a, int b) {
    return a > b ? a : b;
}

int min2(int a, int b) {
    return a < b ? a : b;
}

int main() {
    int x = 7;
    int y = 3;
    /* ternary as function argument */
    int a = max2(x > 5 ? x : 0, y);  /* max(7, 3) = 7 */
    int b = min2(x, y > 0 ? y : 99); /* min(7, 3) = 3 */
    /* ternary in arithmetic */
    int c = (x > y ? x - y : y - x) * 2; /* (7-3)*2 = 8 */
    return a + b + c; /* 7 + 3 + 8 = 18 */
}
