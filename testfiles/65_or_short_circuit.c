/* court-circuit || : zero()||one()=1, one()||zero()=1, zero()||zero()=0 → 1*4+1*2+0 = 6 */
int zero() { return 0; }
int one()  { return 1; }

int main() {
    int r1 = zero() || one();
    int r2 = one()  || zero();
    int r3 = zero() || zero();
    return r1 * 4 + r2 * 2 + r3;
}
