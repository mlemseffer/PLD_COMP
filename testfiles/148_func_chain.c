/* 4.11 Functions: chained function calls */
int double_it(int x) {
    return x * 2;
}
int inc(int x) {
    return x + 1;
}
int main() {
    return double_it(inc(20));
}
