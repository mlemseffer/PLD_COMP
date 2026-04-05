/* 4.20 Recursion: recursive sum using function */
int rec_sum(int n) {
    if (n <= 0) return 0;
    return n + rec_sum(n - 1);
}
int main() {
    return rec_sum(10);
}
