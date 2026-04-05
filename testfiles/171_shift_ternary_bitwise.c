/* Extra: shift, ternary, bitwise operators */
int main() {
    int a = 3 << 2;
    int b = 48 >> 3;
    int c = (a > b) ? a : b;
    int d = a & b;
    int e = a | b;
    return (c + d + e) % 256;
}
