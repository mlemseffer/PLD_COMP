/* Uninitialized variables should at least be declared
   (compiler should accept declVarUninit grammar rule) */
int main() {
    int x;
    int y;
    int z;
    x = 10;
    y = 20;
    z = x + y;
    return z; /* 30 */
}
