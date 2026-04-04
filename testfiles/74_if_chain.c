/* Tests if without else, and else-if chains */
int grade(int score) {
    if (score >= 90) {
        return 4; /* A */
    } else if (score >= 80) {
        return 3; /* B */
    } else if (score >= 70) {
        return 2; /* C */
    } else if (score >= 60) {
        return 1; /* D */
    } else {
        return 0; /* F */
    }
}

int main() {
    int a = grade(95); /* 4 */
    int b = grade(85); /* 3 */
    int c = grade(75); /* 2 */
    int d = grade(65); /* 1 */
    int e = grade(55); /* 0 */
    return a + b + c + d + e; /* 10 */
}
