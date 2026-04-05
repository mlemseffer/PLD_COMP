// Test: for loop with all empty parts (infinite loop broken by break)
int main() {
    int i = 0;
    for (;;) {
        if (i >= 5) break;
        i++;
    }
    return i;
}
