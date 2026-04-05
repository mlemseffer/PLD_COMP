// Test: constant propagation chain - a=5, b=a+3=8, c=b*2=16
int main() {
    int a = 5;
    int b = a + 3;
    int c = b * 2;
    return c;
}
