// Test: continue in while loop - count odd numbers from 1 to 10
// odd: 1,3,5,7,9 -> s=5
int main() {
    int s = 0;
    int i = 0;
    while (i < 10) {
        i++;
        if (i % 2 == 0) continue;
        s++;
    }
    return s;
}
