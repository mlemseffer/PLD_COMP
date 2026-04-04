/* boucles imbriquees */
int main() {
    int count = 0;
    int i = 0;
    while (i < 5) {
        int j = 0;
        while (j < 5) {
            if (i + j == 5) {
                count++;
                break; /* break boucle interne seulement */
            }
            j++;
        }
        i++;
    }
    /* pairs: (0,5 not valid), (1,4), (2,3), (3,2), (4,1) => 4 pairs */
    return count;
}
