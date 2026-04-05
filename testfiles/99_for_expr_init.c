/* for loop with expression init (not declaration) */
int main() {
    int i;
    int sum = 0;
    for (i = 1; i <= 10; i++) {
        sum += i;
    }
    /* 1+2+...+10 = 55 */
    /* i is still accessible after loop */
    return sum - i + 1; /* 55 - 11 + 1 = 45 */
}
