/* pre-inc sur elts tableau */
int main() {
    int a[4];
    a[0] = 10;
    a[1] = 20;
    a[2] = 30;
    a[3] = 40;
    /* pre-inc sur VAR uniquement */
    /* utiliser affectation composee */
    a[0] += 1; /* 11 */
    a[1] -= 1; /* 19 */
    a[2] *= 2; /* 60 */
    a[3] /= 4; /* 10 */
    return a[0] + a[1] + a[2] + a[3]; /* 11+19+60+10 = 100 */
}
