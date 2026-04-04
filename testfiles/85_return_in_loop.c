/* return premature */
int find_first_even(int start, int end) {
    int i = start;
    while (i <= end) {
        if (i % 2 == 0) {
            return i;  /* return dans while */
        }
        i++;
    }
    return -1;
}

int find_in_array(int target) {
    int a[8];
    for (int i = 0; i < 8; ++i) {
        a[i] = i * i;  /* 0, 1, 4, 9, 16, 25, 36, 49 */
    }
    for (int i = 0; i < 8; ++i) {
        if (a[i] == target) {
            return i;  /* return dans for */
        }
    }
    return -1;
}

int main() {
    int a = find_first_even(3, 10); /* 4 */
    int b = find_in_array(25);      /* indice 5 */
    return a + b;                   /* 9 */
}
