/*
 **********************************************************************************
 *  Copyright (C) 2010  Massachusetts Institute of Technology
 *  Copyright (C) 2010  Yuan Tang <yuantang@csail.mit.edu>
 * 		                Charles E. Leiserson <cel@mit.edu>
 * 	 
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Suggestsions:                  yuantang@csail.mit.edu
 *   Bugs:                          yuantang@csail.mit.edu
 *
 *********************************************************************************
 */

#include <cstdio>
#include <cstddef>
#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <cmath>
#include <string>

#include "expr_stencil.hpp"

using namespace std;
#define SIMPLE 0
/* N_RANK includes both time and space dimensions */
#define N_RANK 2
#define TOLERANCE (1e-6)
#define TIMES 3

int main(int argc, char * argv[])
{
	const int BASE = 1024;
	int t;
	struct timeval start, end;
    int N_SIZE=0, T_SIZE=0;
    if (argc < 3) {
        printf("argc < 3, quit!\n");
        exit(1);
    }
    N_SIZE = StrToInt(argv[1]);
    T_SIZE = StrToInt(argv[2]);
    printf("N_SIZE = %d, T_SIZE = %d\n", N_SIZE, T_SIZE);
	/* data structure of Pochoir - row major */
	Pochoir_SArray<double, N_RANK> b(N_SIZE, N_SIZE);

	for (int i = 0; i < N_SIZE; ++i) {
	for (int j = 0; j < N_SIZE; ++j) {
        if (i == 0 || i == N_SIZE-1
            || j == 0 || j == N_SIZE-1) {
            b(0, i, j) = b(1, i, j) = 0;
        } else {
#if DEBUG 
		    b(0, i, j) = i * N_SIZE + j;
		    b(1, i, j) = 0;
#else
            b(0, i, j) = 1.0 * (rand() % BASE); 
            b(1, i, j) = 0; 
#endif
        }
	} }

#if SIMPLE
	cout << endl << "\na(T+1, J, I) = 0.01 + a(T, J, I)" << endl;

	gettimeofday(&start, 0);
    for (int _t = 0; _t < TIMES; ++_t){
    for (int t = 0; t < T_SIZE; ++t) {
	for (int i = 1; i <= N_SIZE-2; ++i) {
	for (int j = 1; j <= N_SIZE-2; ++j) {
        b.interior(t+1, i, j) = 0.01 + b.interior(t, i-1, j+1);
        // b.interior(t+1, i, j) = 0.01 + b.interior(t, i, j);
	} } } }
	gettimeofday(&end, 0);
	std::cout << "Naive Loop: consumed time :" << 1.0e3 * tdiff(&end, &start)/TIMES << "ms" << std::endl;

#else
	cout << "a(T+1, J, I) = 0.125 * (a(T, J+1, I) - 2.0 * a(T, J, I) + a(T, J-1, I)) + 0.125 * (a(T, J, I+1) - 2.0 * a(T, J, I) + a(T, J, I-1)) + a(T, J, I)" << endl;
	gettimeofday(&start, 0);
    for (int _t = 0; _t < TIMES; ++_t) {
	for (int t = 0; t < T_SIZE; ++t) {
    cilk_for (int i = 1; i <= N_SIZE-2; ++i) {
	for (int k = 1; k <= N_SIZE-2; ++k) {
        b.interior(t+1, i, k) = 0.125 * (b.interior(t, i+1, k) - 2.0 * b.interior(t, i, k) + b.interior(t, i-1, k)) + 0.125 * (b.interior(t, i, k+1) - 2.0 * b.interior(t, i, k) + b.interior(t, i, k-1)) + b.interior(t, i, k); } } } }
	gettimeofday(&end, 0);
	std::cout << "Naive Loop: consumed time :" << 1.0e3 * tdiff(&end, &start) / TIMES << "ms" << std::endl;

#endif

#if 0
//	cout << "I = " << I << endl;
//	cout << "I+1 = " << I+1 << endl;
//	cout << "I-1 = " << I-1 << endl;
	cout << "a = " << a << endl;
	cout << "b = " << b << endl;
	cout << "c = " << c << endl;
//	cout << "P1 = " << P1 << endl;
//	cout << "P2 = " << P2 << endl;
	cout << "d = " << d << endl;
#endif
	return 0;
}
