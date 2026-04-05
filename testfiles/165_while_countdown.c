// Test: while loop summing 10+9+...+1 = 55
int main() {
    int i = 10;
    int s = 0;
    while (i > 0) {
        s = s + i;
        i = i - 1;
    }
    return s;
}
