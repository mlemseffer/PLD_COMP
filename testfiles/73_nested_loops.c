/* Nested loops: count pairs (i,j) where i+j == 5, i in [0,5), j in [0,5) */
int main() {
    int count = 0;
    int i = 0;
    while (i < 5) {
        int j = 0;
        while (j < 5) {
            if (i + j == 5) {
                count++;
                break; /* break inner loop only */
            }
            j++;
        }
        i++;
    }
    /* pairs: (0,5 not valid), (1,4), (2,3), (3,2), (4,1) => 4 pairs */
    return count;
}
