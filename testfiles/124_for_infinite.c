/* for with infinite loop (no condition) and break */
int main() {
    int i = 0;
    int sum = 0;
    for ( ; ; ) {
        if (i >= 10) {
            break;
        }
        sum += i;
        i++;
    }
    /* sum 0..9 = 45 */
    return sum;
}
