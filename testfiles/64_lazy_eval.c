int main() {
    // Test évaluation paresseuse
    // 0 && anything -> 0 (le second opérande n'est pas évalué)
    int a = 0 && 42;
    // 1 || anything -> 1 (le second opérande n'est pas évalué)
    int b = 1 || 0;
    // Cas normaux
    int c = 1 && 1;   // 1
    int d = 0 || 0;   // 0

    return a + b * 10 + c * 100 + d; // 0 + 10 + 100 + 0 = 110
}
