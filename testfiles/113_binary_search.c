/* Binary search on a sorted array */
int binary_search(int target) {
    int a[10];
    /* Fill with sorted values: 0, 10, 20, ..., 90 */
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
    return 99; /* sentinel: not found */
}

int main() {
    int a = binary_search(30); /* index 3 */
    int b = binary_search(70); /* index 7 */
    int c = binary_search(0);  /* index 0 */
    return a + b + c; /* 3 + 7 + 0 = 10 */
}

