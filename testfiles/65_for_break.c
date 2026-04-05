int main() {
    int result = 0;
    for (int i = 0; i < 100; ++i) {
        if (i >= 10) {
            break;
        }
        result += i;
    }
    return result; // 0+1+2+...+9 = 45
}
