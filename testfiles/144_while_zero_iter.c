/* 4.14 while: loop runs exact number of times */
int main() {
    int x = 0;
    int i = 5;
    while (i > 0) {
        x = x + 1;
        i = i - 1;
    }
    return x;
}
