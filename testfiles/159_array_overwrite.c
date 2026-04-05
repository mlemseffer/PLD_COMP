/* 4.19 Arrays: overwrite element */
int main() {
    int a[2];
    a[0] = 5;
    a[1] = 10;
    a[0] = 99;
    return a[0];
}
