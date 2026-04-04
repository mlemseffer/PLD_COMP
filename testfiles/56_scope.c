int main() {
    int a = 10;
    int res = 0;
    
    {
        int a = 20; /* shadow a externe */
        res = res + a; /* 20 */
        {
            int a = 30; /* shadow a interne */
            res = res + a; /* 50 */
        }
        res = res + a; /* 70 */
    }
    
    res = res + a; /* 80 */

    return res; /* 80 */
}
