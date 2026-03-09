#!/bin/bash
# Test runner for ifcc compiler

cd /mnt/c/Users/Nospace/PLD-COMP/compiler

PASS=0
FAIL=0
TOTAL=0

run_test() {
    local file="$1"
    local expected="$2"
    local name=$(basename "$file")
    TOTAL=$((TOTAL + 1))

    ./ifcc "$file" > /tmp/ifcc_test.s 2>/tmp/ifcc_stderr.txt
    if [ $? -ne 0 ]; then
        echo "FAIL: $name - ifcc compilation failed"
        cat /tmp/ifcc_stderr.txt
        FAIL=$((FAIL + 1))
        return
    fi

    gcc -o /tmp/ifcc_test_bin /tmp/ifcc_test.s -static 2>/tmp/gcc_stderr.txt
    if [ $? -ne 0 ]; then
        echo "FAIL: $name - gcc assembly failed"
        cat /tmp/gcc_stderr.txt
        FAIL=$((FAIL + 1))
        return
    fi

    /tmp/ifcc_test_bin
    local actual=$?

    if [ "$actual" -eq "$expected" ]; then
        echo "PASS: $name (expected=$expected, got=$actual)"
        PASS=$((PASS + 1))
    else
        echo "FAIL: $name (expected=$expected, got=$actual)"
        FAIL=$((FAIL + 1))
    fi
}

echo "========================================="
echo "  ifcc Test Suite"
echo "========================================="

TESTDIR="../testfiles"

# --- Tests variables & affectations ---
run_test "$TESTDIR/1_return42.c"           42
run_test "$TESTDIR/8_return_zero.c"        0
run_test "$TESTDIR/9_return_255.c"         255
run_test "$TESTDIR/10_decl_return_var.c"   7
run_test "$TESTDIR/11_two_vars.c"          20
run_test "$TESTDIR/12_affectation_const.c" 99
run_test "$TESTDIR/13_affectation_var.c"   55
run_test "$TESTDIR/14_chain_assign.c"      17
run_test "$TESTDIR/15_swap.c"              20
run_test "$TESTDIR/16_many_vars.c"         4
run_test "$TESTDIR/17_return_const.c"      100
run_test "$TESTDIR/18_overwrite.c"         50
run_test "$TESTDIR/19_init_zero.c"         0
run_test "$TESTDIR/20_self_assign.c"       77
run_test "$TESTDIR/21_decl_from_var.c"     33

# --- Tests expressions arithmetiques ---
run_test "$TESTDIR/22_add.c"               5
run_test "$TESTDIR/23_sub.c"               7
run_test "$TESTDIR/24_mul.c"               20
run_test "$TESTDIR/25_priority.c"          14
run_test "$TESTDIR/26_parens.c"            20
run_test "$TESTDIR/27_left_assoc.c"        5
run_test "$TESTDIR/28_combo.c"             26
run_test "$TESTDIR/29_unary_minus.c"       3
run_test "$TESTDIR/30_var_add.c"           15
run_test "$TESTDIR/31_var_mul.c"           21
run_test "$TESTDIR/32_complex_expr.c"      91
run_test "$TESTDIR/33_decl_expr.c"         30
run_test "$TESTDIR/34_chain_mul.c"         24
run_test "$TESTDIR/35_unary_paren.c"       251
run_test "$TESTDIR/36_priority_sub.c"      4

echo "========================================="
echo "  Results: $PASS/$TOTAL passed, $FAIL failed"
echo "========================================="

rm -f /tmp/ifcc_test.s /tmp/ifcc_test_bin /tmp/ifcc_stderr.txt /tmp/gcc_stderr.txt
exit $FAIL
