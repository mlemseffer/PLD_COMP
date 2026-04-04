/* fonction void */
void print_stars(int n) {
    for (int i = 0; i < n; ++i) {
        putchar('*');
    }
    putchar('\n');
}

void print_range(int lo, int hi) {
    int i = lo;
    while (i <= hi) {
        putchar('0' + i); /* affiche digit */
        i++;
    }
    putchar('\n');
}

int main() {
    print_stars(3);      /* affiche "***\n" */
    print_range(1, 5);   /* affiche "12345\n" */
    return 0;
}
