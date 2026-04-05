/* Function that returns double, implicit conversions */
double square_d(double x) {
    return x * x;
}

double add_d(double a, double b) {
    return a + b;
}

int main() {
    double r = square_d(3.0);     /* 9.0 */
    double s = add_d(r, 6.5);    /* 15.5 */
    int result = s;               /* truncate to 15 */
    return result;
}
