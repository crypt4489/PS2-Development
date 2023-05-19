#include "psxmath.h"

int isin(int x) {

    int c, x2, y;

    c= x<<(30-qN);              // Semi-circle info into carry.
    x -= 1<<qN;                 // sine -> cosine calc

    x= x<<(31-qN);              // Mask with PI
    x= x>>(31-qN);              // Note: SIGNED shift! (to qN)

    x= x*x>>(2*qN-14);          // x=x^2 To Q14

    y= B1 - (x*C>>14);           // B - x^2*C
    y= (1<<qA)-(x*y>>16);       // A - x^2*(B-x^2*C)

    return c>=0 ? y : -y;

}

int icos(int x) {

    return isin( x+1024 );
}

int absolute(int x)
{
    if (x < 0)
    {
        return -x;
    }
    return x;
}

int my_floor(int x)
{
    return (int)(x & 0xFFFFF000);
}
