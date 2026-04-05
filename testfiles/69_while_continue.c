/* while + continue: sum only even numbers from 0 to 9 */
int main() {
    int i = 0;
    int sum = 0;
    while (i < 10) {
        i++;
        if (i % 2 != 0) {
            continue;
        }
        sum += i;
    }
    /* 2 + 4 + 6 + 8 + 10 = 30 */
    return sum;
}
