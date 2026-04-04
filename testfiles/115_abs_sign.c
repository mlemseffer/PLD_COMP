/* fonction valeur absolue */
int my_abs(int x) {
    return x >= 0 ? x : 0 - x;
}

int my_sign(int x) {
    if (x > 0) {
        return 1;
    } else if (x < 0) {
        return -1;
    }
    return 0;
}

int main() {
    int a = my_abs(42);     /* 42 */
    int b = my_abs(0 - 42); /* 42 */
    int c = my_abs(0);      /* 0 */
    int s1 = my_sign(100);  /* 1 */
    int s2 = my_sign(0 - 5);/* attention au code de retour */
    int s3 = my_sign(0);    /* 0 */
    /* Encode sign results: s2 = -1 → abs is 1 */
    int sign_somme = s1 + my_abs(s2) + s3; /* 1+1+0 = 2 */
    return a + b + c + sign_sum; /* 42+42+0+2 = 86 */
}
