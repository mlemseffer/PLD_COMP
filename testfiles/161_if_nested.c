// Test: nested if/else, 3 levels deep
// x=15: x>10 -> true, x>20 -> false, x>12 -> true, result = 3
int main() {
    int x = 15;
    int r = 0;
    if (x > 10) {
        if (x > 20) {
            r = 1;
        } else {
            if (x > 12) {
                r = 3;
            } else {
                r = 2;
            }
        }
    } else {
        r = 4;
    }
    return r;
}
