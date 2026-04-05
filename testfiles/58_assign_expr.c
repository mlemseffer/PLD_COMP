int main() {
    int a = 0;
    int b = 0;
    int c = 0;
    a = b = c = 5; /* right-associative */

    int d = 10;
    int e = (d = 20) + 5; /* d gets 20, e gets 25 */

    return a + b + c + d + e; /* 5 + 5 + 5 + 20 + 25 = 60 */
}
