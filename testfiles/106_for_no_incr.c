/* for loop with no increment (empty 3rd part) */
int main() {
    int sum = 0;
    for (int i = 0; i < 5; ) {
        sum += i;
        i += 2; /* increment inside the body */
    }
    /* i takes values 0, 2, 4 → sum = 6 */
    return sum;
}
