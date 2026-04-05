/* Error: variable declared twice in same scope */
int main() {
    int x = 5;
    int x = 10; /* redeclaration error */
    return x;
}
