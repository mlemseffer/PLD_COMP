/* Scope: variable declared in if block is local */
int main() {
    int x = 10;
    if (1) {
        int y = 20;
        x = x + y;
    }
    return x;
}
