// Test: break only exits inner loop
// outer runs 3 times, inner breaks at j==1 each time -> s=3
int main() {
    int s = 0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 10; j++) {
            if (j == 1) break;
            s++;
        }
    }
    return s;
}
