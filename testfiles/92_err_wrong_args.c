/* Error: wrong number of arguments to function */
int add(int a, int b) {
    return a + b;
}

int main() {
    return add(1, 2, 3); /* too many args */
}
