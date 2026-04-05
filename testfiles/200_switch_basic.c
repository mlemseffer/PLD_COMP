// Test: basic switch/case - x=2 matches case 2, returns 20
int main() {
    int x = 2;
    switch (x) {
        case 1: return 10;
        case 2: return 20;
        case 3: return 30;
    }
    return 0;
}
