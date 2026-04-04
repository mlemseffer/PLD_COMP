/* do-while + continue: skip multiples of 3, break at 15 */
int main() {
    int i = 0;
    int sum = 0;
    do {
        i++;
        if (i % 3 == 0) {
            continue;
        }
        if (i > 10) {
            break;
        }
        sum += i;
    } while (1);
    /* 1+2+4+5+7+8+10 = 37 */
    return sum;
}
