/* recherche dichotomique */
int binary_search(int target) {
    int a[10];
    /* init tableau trie */
    for (int i = 0; i < 10; ++i) {
        a[i] = i * 10;
    }
    int lo = 0;
    int hi = 9;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (a[mid] == target) {
            return mid;
        } else if (a[mid] < target) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return 99; /* non trouve */
}

int main() {
    int a = binary_search(30); /* indice 3 */
    int b = binary_search(70); /* indice 7 */
    int c = binary_search(0);  /* indice 0 */
    return a + b + c; /* 3 + 7 + 0 = 10 */
}

