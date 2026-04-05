/* 4.12 Comparisons: verify all relational results */
int main() {
    int r = 0;
    if (3 < 5) r = r + 1;
    if (5 > 3) r = r + 10;
    if (3 <= 3) r = r + 100;
    return r;
}
