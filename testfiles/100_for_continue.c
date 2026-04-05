/* continue inside for loop: skip multiples of 3 */
int main() {
    int total = 0;
    for (int i = 1; i <= 20; i++) {
        if (i % 3 == 0) {
            continue;
        }
        total += i;
    }
    /* sum 1..20 = 210, minus (3+6+9+12+15+18) = 63 → 147 */
    return total;
}
