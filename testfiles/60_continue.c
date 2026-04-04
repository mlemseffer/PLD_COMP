int main() {
    int somme = 0;
    for (int i = 0; i < 10; ++i) {
        if (i % 2 == 0) {
            continue;
        }
        somme += i;
    }
    return sum; // 1+3+5+7+9 = 25
}
