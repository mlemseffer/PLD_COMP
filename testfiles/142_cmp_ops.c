/* 4.12 Comparisons: all 6 comparison operators */
int main() {
    int r = 0;
    if (1 < 2) r = r + 1;
    if (2 > 1) r = r + 1;
    if (3 <= 3) r = r + 1;
    if (3 >= 3) r = r + 1;
    if (5 == 5) r = r + 1;
    if (5 != 6) r = r + 1;
    return r;
}
