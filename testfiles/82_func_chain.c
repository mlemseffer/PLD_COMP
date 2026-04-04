/* chaine d'appels de fonctions */
int triple(int x) {
    return x * 3;
}

int add_one(int x) {
    return triple(x) + 1;
}

int double_add_one(int x) {
    return add_one(x) + add_one(x);
}

int main() {
    /* double_add_one(5) = add_one(5) + add_one(5)
                        = (triple(5)+1) + (triple(5)+1)
                        = 16 + 16 = 32 */
    return double_add_one(5);
}
