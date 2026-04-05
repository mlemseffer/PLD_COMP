/* Fait appel a putchar pour avoir des side effects sans return value */
void print_A() {
    putchar('A');
    putchar('\n');
}

int main() {
    print_A();
    return 0;
}
