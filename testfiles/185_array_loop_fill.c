// Test: fill array with for loop and sum it
// a[i]=i+1 for i=0..4, sum = 1+2+3+4+5 = 15
int main() {
    int a[5];
    for (int i = 0; i < 5; i++) {
        a[i] = i + 1;
    }
    int s = 0;
    for (int i = 0; i < 5; i++) {
        s += a[i];
    }
    return s;
}
