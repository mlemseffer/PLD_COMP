/* fall-through : case 1 tombe dans case 2 → 10 + 20 = 30 */
int main() {
    int x = 1;
    int result = 0;
    switch (x) {
        case 1:
            result = result + 10;
            /* pas de break : fall-through */
        case 2:
            result = result + 20;
            break;
        case 3:
            result = result + 30;
            break;
    }
    return result;
}
