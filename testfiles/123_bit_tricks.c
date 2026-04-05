/* Shift operators used to implement powers of 2 and bit checks */
int is_power_of_2(int n) {
    /* A power of 2 has exactly one bit set: n & (n-1) == 0 */
    if (n <= 0) {
        return 0;
    }
    return (n & (n - 1)) == 0;
}

int count_bits(int n) {
    int count = 0;
    while (n > 0) {
        count += n & 1;
        n = n >> 1;
    }
    return count;
}

int main() {
    int a = is_power_of_2(16);  /* 1 */
    int b = is_power_of_2(15);  /* 0 */
    int c = is_power_of_2(64);  /* 1 */
    int d = count_bits(255);    /* 8 (all bits set in byte) */
    int e = count_bits(170);    /* 4 (10101010) */
    return a + b + c + d + e; /* 1+0+1+8+4 = 14 */
}
