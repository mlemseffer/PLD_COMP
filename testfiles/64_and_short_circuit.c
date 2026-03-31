/* court-circuit && : zero()&&one()=0, one()&&one()=1, one()&&zero()=0 → 0*4+1*2+0 = 2 */
int zero() { return 0; }
int one()  { return 1; }

int main() {
    int r1 = zero() && one();
    int r2 = one()  && one();
    int r3 = one()  && zero();
    return r1 * 4 + r2 * 2 + r3;
}
