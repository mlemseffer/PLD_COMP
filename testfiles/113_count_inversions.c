/* Counting inversions in array (nested loops, conditionals) */
int main() {
    int a[6];
    a[0] = 3;
    a[1] = 1;
    a[2] = 4;
    a[3] = 1;
    a[4] = 5;
    a[5] = 9;

    int count = 0;
    for (int i = 0; i < 6; i++) {
        for (int j = i + 1; j < 6; j++) {
            if (a[i] > a[j]) {
                count++;
            }
        }
    }
    /* Inversions: (3,1), (3,1), (4,1) = 3 inversions */
    return count;
}
