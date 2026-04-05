/* Scope: variable declared in loop body is local */
int main() {
    int s = 0;
    int i = 0;
    while (i < 3) {
        int temp = i * 10;
        s = s + temp;
        i = i + 1;
    }
    return s;
}
