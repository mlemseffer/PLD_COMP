// Test: all 6 comparison operators, combine results
// (3 < 5)=1, (5 > 3)=1, (3 <= 3)=1, (5 >= 5)=1, (1 == 1)=1, (1 != 2)=1
// sum = 6
int main() {
    int a = (3 < 5);
    int b = (5 > 3);
    int c = (3 <= 3);
    int d = (5 >= 5);
    int e = (1 == 1);
    int f = (1 != 2);
    return a + b + c + d + e + f;
}
