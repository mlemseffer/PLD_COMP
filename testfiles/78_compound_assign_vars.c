/* affectation composee avec variables */
int main() {
    int x = 100;
    int y = 7;
    x += y;      /* 107 */
    x -= y * 2;  /* 107 - 14 = 93 */
    x *= 2;      /* 186 */
    x /= y + y;  /* 186 / 14 = 13 */
    x %= y;      /* 13 % 7 = 6 */
    return x;    /* 6 */
}
