int main() {
    int sum = 0;
    for (int i = 0; i < 10; ++i) {
        if (i % 2 == 0) {
            continue;
        }
        sum += i;
    }
    return sum; // 1+3+5+7+9 = 25
}
