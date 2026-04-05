// Test: nested for loops - 3*3 = 9 iterations
int main() {
    int s = 0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            s++;
        }
    }
    return s;
}
