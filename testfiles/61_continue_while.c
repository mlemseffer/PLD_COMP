/* continue dans un while : saute i==3, cumule 1+2+4+5 = 12 */
int main() {
    int i = 0;
    int result = 0;
    while (i < 5) {
        i = i + 1;
        if (i == 3) {
            continue;
        }
        result = result + i;
    }
    return result;
}
