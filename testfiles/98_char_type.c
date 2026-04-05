/* char type variable declarations and arithmetic */
int main() {
    char a = 'A';        /* 65 */
    char b = 'Z';        /* 90 */
    char newline = '\n'; /* 10 */
    int diff = b - a;    /* 25 */
    int sum = a + diff;  /* 65 + 25 = 90 */
    /* lowercase conversion: add 32 */
    char lower_a = 'a';  /* 97 */
    int dist = lower_a - a; /* 97 - 65 = 32 */
    return diff + dist; /* 25 + 32 = 57 */
}
