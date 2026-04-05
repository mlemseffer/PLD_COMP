/* Same variable names in different function scopes (no interference) */
int compute(int x) {
    int result = x * x;
    int temp = result + x;
    return temp;
}

int transform(int x) {
    int result = x + 10;
    int temp = result * 2;
    return temp;
}

int main() {
    int result = 0;     /* 'result' here is different from inside functions */
    int temp = 0;       /* same for 'temp' */
    result = compute(4); /* 4*4 + 4 = 20 */
    temp = transform(3); /* (3+10)*2 = 26 */
    return result + temp; /* 46 */
}
