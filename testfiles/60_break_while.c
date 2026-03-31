/* break dans un while : s'arrête à i==5, cumule 0+1+2+3+4 = 10 */
int main() {
    int i = 0;
    int result = 0;
    while (i < 10) {
        if (i == 5) {
            break;
        }
        result = result + i;
        i = i + 1;
    }
    return result;
}
