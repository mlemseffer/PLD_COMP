/* Complex double expressions: pi approximation, exponential */
double my_sqrt_newton(double n) {
    /* Newton-Raphson: x_{n+1} = (x + n/x) / 2 */
    double x = n;
    int i = 0;
    while (i < 20) {
        x = (x + n / x) / 2.0;
        i++;
    }
    return x;
}

int main() {
    /* sqrt(144) should be close to 12 */
    double r = my_sqrt_newton(144.0);
    int result = r; /* truncate: 12 */
    return result;  /* 12 */
}
