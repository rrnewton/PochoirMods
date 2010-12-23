/*
 * ============================================================================
 *
 *       Filename:  t.cpp
 *
 *    Description:  test bench
 *
 *        Version:  1.0
 *        Created:  11/18/2010 11:44:36 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Yuan Tang (), yuantang@csail.mit.edu
 *        Company:  CSAIL, M.I.T.
 *
 * ============================================================================
 */

#include <stdio.h>
int amlive (int N, int L) {
    int x = ((N-2)>>31) | ~((N-3)>>31); // x==0 iff N==2 (otherwise -1)
    int y = ((N-3)>>31) | ~((N-4)>>31); // y==0 iff N==3 (otherwise -1)
    int F = 1&((~x&L)|~y);
    printf("N = %d, L = %d, X = %d, Y = %d, F = %d\n", N, L, x, y, F);
    return F;
}

int main (int argc, char *argv[]) {
    printf("  D L\n");
    for (int N=0; N<=8; N++) {
      amlive(N,0); 
      amlive(N,1);
    }
    return 0;
}
