int main() {
    int a = 10;
    int res = 0;
    
    {
        int a = 20; /* shadows outer a */
        res = res + a; /* 20 */
        {
            int a = 30; /* shadows inner a */
            res = res + a; /* 50 */
        }
        res = res + a; /* 70 */
    }
    
    res = res + a; /* 80 */

    return res; /* 80 */
}
