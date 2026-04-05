/* Deep variable scope: variables in nested blocks shadow outer ones */
int main() {
    int x = 1;
    int total = 0;
    {
        int x = 2;
        total += x; /* 2 */
        {
            int x = 3;
            total += x; /* 5 */
            {
                int x = 4;
                total += x; /* 9 */
            }
            total += x; /* 12 — x=3 again */
        }
        total += x; /* 14 — x=2 again */
    }
    total += x; /* 15 — x=1 again */
    return total; /* 15 */
}
