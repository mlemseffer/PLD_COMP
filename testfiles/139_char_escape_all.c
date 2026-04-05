// Test: various escape sequences as char constants
// '\n'=10, '\t'=9, '\0'=0, '\\'=92, '\''=39
// sum = 10+9+0+92+39 = 150
int main() {
    int a = '\n';
    int b = '\t';
    int c = '\0';
    int d = '\\';
    int e = '\'';
    return a + b + c + d + e;
}
