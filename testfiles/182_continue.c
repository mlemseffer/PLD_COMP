/* Extra: continue skips to next iteration */
int main() {
    int s = 0;
    int i;
    for (i = 0; i < 10; ++i) {
        if (i % 2 == 0) continue;
        s = s + i;
    }
    return s;
}
