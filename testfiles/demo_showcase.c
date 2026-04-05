#include <stdio.h>

// ---- Utility: print a positive integer to stdout ----
void print_int(int x) {
    if (x < 0) {
        putchar('-');
        x = -x;
    }
    if (x / 10 != 0) {
        print_int(x / 10);
    }
    putchar(x % 10 + '0');
}

void print_ln() {
    putchar(10);
    return;
}

void print_str_ok() {
    putchar('O');
    putchar('K');
}

void print_str_fail() {
    putchar('F');
    putchar('A');
    putchar('I');
    putchar('L');
}

// ---- Feature: functions, recursion, if/else ----
int fibonacci(int n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// ---- Feature: while loop, compound assign ----
int factorial_iter(int n) {
    int result = 1;
    int i = 2;
    while (i <= n) {
        result *= i;
        ++i;
    }
    return result;
}

// ---- Feature: for loop, arrays, break ----
int is_prime(int n) {
    if (n < 2) return 0;
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0) return 0;
    }
    return 1;
}

int count_primes(int limit) {
    int count = 0;
    for (int i = 2; i <= limit; ++i) {
        if (is_prime(i)) {
            count++;
        }
    }
    return count;
}

// ---- Feature: ternary operator ----
int abs_val(int x) {
    return x >= 0 ? x : -x;
}

int max(int a, int b) {
    return a > b ? a : b;
}

int min(int a, int b) {
    return a < b ? a : b;
}

// ---- Feature: do-while, continue ----
int sum_odd_digits(int n) {
    int sum = 0;
    do {
        int digit = n % 10;
        n = n / 10;
        if (digit % 2 == 0) {
            continue;
        }
        sum += digit;
    } while (n > 0);
    return sum;
}

// ---- Feature: arrays, for loop ----
int bubble_sort_sum(int size) {
    int arr[10];
    // Remplir le tableau en ordre inverse
    for (int i = 0; i < size; ++i) {
        arr[i] = size - i;
    }

    // Tri a bulles
    for (int i = 0; i < size - 1; ++i) {
        for (int j = 0; j < size - 1 - i; ++j) {
            if (arr[j] > arr[j + 1]) {
                int tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }

    // Verifier que le tableau est trie : retourner la somme
    int sum = 0;
    for (int i = 0; i < size; ++i) {
        sum += arr[i] * (i + 1);
    }
    return sum;
}

// ---- Feature: bitwise ops, shifts, logical ops ----
int bit_tricks(int x) {
    int is_even = !(x & 1);
    int doubled = x << 1;
    int halved = x >> 1;
    int masked = x & 255;
    int toggled = x ^ 255;
    return is_even + doubled + halved + (masked & 1) + (toggled > 0 ? 1 : 0);
}

// ---- Feature: >6 arguments ----
int sum_8(int a, int b, int c, int d, int e, int f, int g, int h) {
    return a + b + c + d + e + f + g + h;
}

// ---- Feature: scope / shadowing ----
int test_scope() {
    int x = 10;
    {
        int x = 20;
        {
            int x = 30;
            if (x != 30) return 0;
        }
        if (x != 20) return 0;
    }
    if (x != 10) return 0;
    return 1;
}

// ---- Feature: char constants ----
int test_chars() {
    char newline = '\n';
    char tab = '\t';
    char zero = '\0';
    char a = 'A';
    if (a != 65) return 0;
    if (newline != 10) return 0;
    if (zero != 0) return 0;
    return 1;
}

// === MAIN : run all tests and display results ===

void check(int condition, int test_id) {
    putchar('[');
    if (condition) {
        print_str_ok();
    } else {
        print_str_fail();
    }
    putchar(']');
    putchar(' ');
    putchar('#');
    print_int(test_id);
    print_ln();
}

int main() {
    // Header
    putchar('=');
    putchar('=');
    putchar('=');
    putchar(' ');
    putchar('I');
    putchar('F');
    putchar('C');
    putchar('C');
    putchar(' ');
    putchar('D');
    putchar('E');
    putchar('M');
    putchar('O');
    putchar(' ');
    putchar('=');
    putchar('=');
    putchar('=');
    print_ln();

    int ok = 1;

    // Test 1: fibonacci
    int fib8 = fibonacci(8);
    check(fib8 == 21, 1);
    if (fib8 != 21) ok = 0;

    // Test 2: factorial
    int fact6 = factorial_iter(6);
    check(fact6 == 720, 2);
    if (fact6 != 720) ok = 0;

    // Test 3: primes
    int primes = count_primes(30);
    check(primes == 10, 3);
    if (primes != 10) ok = 0;

    // Test 4: ternary + abs
    int a = abs_val(-42) + abs_val(17);
    check(a == 59, 4);
    if (a != 59) ok = 0;

    // Test 5: min/max
    int m = max(13, 7) + min(13, 7);
    check(m == 20, 5);
    if (m != 20) ok = 0;

    // Test 6: do-while + continue (sum odd digits of 12345)
    int sod = sum_odd_digits(12345);
    check(sod == 9, 6);
    if (sod != 9) ok = 0;

    // Test 7: bubble sort
    int bs = bubble_sort_sum(8);
    check(bs == 204, 7);
    if (bs != 204) ok = 0;

    // Test 8: bit tricks
    int bt = bit_tricks(42);
    check(bt == 107, 8);
    if (bt != 107) ok = 0;

    // Test 9: >6 args
    int s8 = sum_8(1, 2, 3, 4, 5, 6, 7, 8);
    check(s8 == 36, 9);
    if (s8 != 36) ok = 0;

    // Test 10: scope
    check(test_scope(), 10);
    if (!test_scope()) ok = 0;

    // Test 11: char constants
    check(test_chars(), 11);
    if (!test_chars()) ok = 0;

    // Test 12: compound assign
    int x = 100;
    x += 20;
    x -= 5;
    x *= 2;
    x /= 10;
    x %= 7;
    check(x == 2, 12);
    if (x != 2) ok = 0;

    // Test 13: logical operators (lazy)
    int lazy = (0 && 999) + (1 || 999) + (1 && 1) + (0 || 0);
    check(lazy == 2, 13);
    if (lazy != 2) ok = 0;

    // Test 14: shifts
    int sh = (1 << 8) + (256 >> 4);
    check(sh == 272, 14);
    if (sh != 272) ok = 0;

    // Final
    print_ln();
    if (ok) {
        return 0;
    }
    return 1;
}
