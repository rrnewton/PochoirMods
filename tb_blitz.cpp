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
#include <cmath>

#include <blitz/array.h>

/* include timing facility */
#include "expr_common.hpp"

BZ_USING_NAMESPACE(blitz)

#define SIMPLE 0
/* N_RANK includes both time and space dimensions */
#define N_RANK 3
#define N_SIZE 555
#define T_SIZE 505

int main(void)
{
	const int BASE = 1024;
	int t;
	struct timeval start, end;
	/* data structure of Blitz - row major */
	Array<double, N_RANK-1> P1, P2;
	allocateArrays(shape(N_SIZE, N_SIZE), P1, P2);
	Range K(1, N_SIZE-2), L(1, N_SIZE-2);

	for (int i = 0; i < N_SIZE; ++i) {
	for (int j = 0; j < N_SIZE; ++j) {
#if DEBUG 
		P1(i, j) = i * N_SIZE + j;
		P2(i, j) = 0;
#else
		P1(i, j) = 1.0 * (rand() % BASE); 
		P2(i, j) = 0; 
#endif
	} }

#if SIMPLE
	cout << endl << "\na(T+1, J, I) = 0.01 + a(T, J, I)" << endl;

	gettimeofday(&start, 0);
	for (t = 0; t < T_SIZE+1; t += 2) {
		P2(K, L) = 0.01 + P1(K, L);
		P1(K, L) = 0.01 + P2(K, L);
	}
	gettimeofday(&end, 0);
	std::cout << "Blitz ET: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

#else
	cout << "a(T+1, J, I) = 0.125 * (a(T, J+1, I) - 2.0 * a(T, J, I) + a(T, J-1, I)) + 0.125 * (a(T, J, I+1) - 2.0 * a(T, J, I) + a(T, J, I-1)) + a(T, J, I)" << endl;

	gettimeofday(&start, 0);
	for (t = 0; t < T_SIZE+1; t += 2) {
		P2(K, L) = 0.125 * (P1(K, L+1) - 2.0 * P1(K, L) + P1(K, L-1)) +
				   0.125 * (P1(K+1, L) - 2.0 * P1(K, L) + P1(K-1, L)) +
				   P1(K, L);
		P1(K, L) = 0.125 * (P2(K, L+1) - 2.0 * P2(K, L) + P2(K, L-1)) +
				   0.125 * (P2(K+1, L) - 2.0 * P2(K, L) + P2(K-1, L)) +
				   P2(K, L);
	}
	gettimeofday(&end, 0);
	std::cout << "Blitz ET: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

#endif

#if 0 
//	cout << "I = " << I << endl;
//	cout << "I+1 = " << I+1 << endl;
//	cout << "I-1 = " << I-1 << endl;
	cout << "a = " << a << endl;
//	cout << "b = " << b << endl;
	cout << "c = " << c << endl;
	cout << "P1 = " << P1 << endl;
	cout << "P2 = " << P2 << endl;
//	cout << "d = " << d << endl;
#endif
	return 0;
}
