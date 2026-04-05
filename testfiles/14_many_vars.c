/* Many variables to test stack space */
int main() {
    int a = 1;
    int b = 2;
    int c = 3;
    int d = 4;
    int e = 5;
    e = d;
    d = c;
    c = b;
    b = a;
    return e;
}
