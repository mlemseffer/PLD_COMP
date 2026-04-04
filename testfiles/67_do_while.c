int main() {
    int i = 1;
    int fact = 1;
    do {
        fact *= i;
        ++i;
    } while (i <= 5);
    return fact; // 120
}
