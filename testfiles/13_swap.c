/* Swap two variables using a temp */
int main() {
    int a = 10;
    int b = 20;
    int tmp = 0;
    tmp = a;
    a = b;
    b = tmp;
    return a;
}
