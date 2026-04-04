/* Multiple arrays and 2D-like access via flat index */
int main() {
    int row0[4];
    int row1[4];
    /* Initialize */
    for (int i = 0; i < 4; ++i) {
        row0[i] = i;        /* 0, 1, 2, 3 */
        row1[i] = i * 10;   /* 0, 10, 20, 30 */
    }
    /* Compute dot product: sum(row0[i] * row1[i]) */
    int dot = 0;
    for (int i = 0; i < 4; ++i) {
        dot += row0[i] * row1[i];
    }
    /* dot = 0*0 + 1*10 + 2*20 + 3*30 = 0+10+40+90 = 140 */
    return dot;
}
