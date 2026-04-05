/* Extra: break exits loop early */
int main() {
    int i = 0;
    while (i < 100) {
        if (i == 7) break;
        i = i + 1;
    }
    return i;
}
