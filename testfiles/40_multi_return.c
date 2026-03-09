int abs(int x) {
    if (x < 0) {
        return 0 - x;
    }
    return x;
}

int classify(int x) {
    if (x == 0) {
        return 0;
    }
    if (x > 0) {
        return 1;
    }
    return 2;
}

int main() {
    int a = abs(0 - 42);
    int b = classify(0);
    int c = classify(10);
    int d = classify(0 - 5);
    return a + b + c + d;
}
