/* chain assignment: a = b = c = value */
int main() {
    int a;
    int b;
    int c;
    a = b = c = 42;
    /* a=42, b=42, c=42 */
    int x;
    int y;
    x = y = a + b; /* 84 */
    return c + x - y; /* 42 + 84 - 84 = 42 */
}
