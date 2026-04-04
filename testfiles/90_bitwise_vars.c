/* ops bit-a-bit avec variables */
int main() {
    int mask = 15;      /* 0x0F = 15 */
    int val  = 170;     /* 0xAA = 10101010 */
    int r1 = val & mask;  /* 170 & 15 = 10 (0000 1010) */
    int r2 = val ^ mask;  /* 170 ^ 15 = 165 (1010 0101) */
    int r3 = r1 | r2;     /* 10 | 165 = 175 */
    /* swap XOR */
    int x = 42;
    int y = 17;
    x = x ^ y;  /* x = 42^17 = 59 */
    y = x ^ y;  /* y = 59^17 = 42 */
    x = x ^ y;  /* x = 59^42 = 17 */
    /* apres swap: x=17, y=42 */
    int check = x + y; /* 59 */
    return r3 - check; /* 175 - 59 = 116 */
}
