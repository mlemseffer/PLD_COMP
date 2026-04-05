/* 4.14 while: basic loop summing 1 to 5 */
int main() {
    int i = 1;
    int s = 0;
    while (i <= 5) {
        s = s + i;
        i = i + 1;
    }
    return s;
}
