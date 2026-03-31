/* switch avec default : x=99 ne correspond à aucun case → default → 42 */
int main() {
    int x = 99;
    int result = 0;
    switch (x) {
        case 1:
            result = 1;
            break;
        case 2:
            result = 2;
            break;
        default:
            result = 42;
            break;
    }
    return result;
}
