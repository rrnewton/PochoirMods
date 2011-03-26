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

/* Test bench - 2D Game of Life, Periodic version */
#include <cstdio>
#include <cstddef>
#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <cmath>

#include <pochoir.hpp>

using namespace std;
#define TIMES 1
/* N_RANK includes both time and space dimensions */
#define N_RANK 2
// #define N_SIZE 555
// #define T_SIZE 555

void check_result(int t, int j, int i, bool a, bool b)
{
	if (a == b) {
//		printf("a(%d, %d, %d) == b(%d, %d, %d) == %s : passed!\n", t, j, i, t, j, i, a ? "True" : "False");
	} else {
		printf("a(%d, %d, %d) = %s, b(%d, %d, %d) = %s : FAILED!\n", t, j, i, a ? "True" : "False", t, j, i, b ? "True" : "False");
	}

}

    Pochoir_Boundary_2D(life_bv_2D, arr, t, i, j)
        /* this is non-periodic boundary value */
        /* we already shrinked by using range I, J, K,
         * so the following code to set boundary index and
         * boundary rvalue is not necessary!!! 
         */
        int new_i = i, new_j = j;
#if 0
        if (new_i < 0)
            new_i += arr.size(1);
        else if (new_i >= arr.size(1))
            new_i -= arr.size(1);

        if (new_j < 0)
            new_j += arr.size(0);
        else if (new_j >= arr.size(0))
            new_j -= arr.size(0);
#else
     	const int arr_size_1 = arr.size(1);
		const int arr_size_0 = arr.size(0);
		new_i = (i < 0 ? new_i + arr_size_1 : (i >= arr_size_1 ? new_i - arr_size_1 : new_i));
		new_j = (j < 0 ? new_j + arr_size_0 : (j >= arr_size_0 ? new_j - arr_size_0 : new_j));
#endif
		return arr.get(t, new_i, new_j);
    Pochoir_Boundary_end

int main(int argc, char * argv[])
{
	int t;
	struct timeval start, end;
    int N_SIZE = 0, T_SIZE = 0;

    if (argc < 3) {
        printf("argc < 3, quit! \n");
        exit(1);
    }
    N_SIZE = StrToInt(argv[1]);
    T_SIZE = StrToInt(argv[2]);
    printf("N_SIZE = %d, T_SIZE = %d\n", N_SIZE, T_SIZE);
	/* data structure of Pochoir - row major */
	Pochoir_Array<bool, N_RANK> a(N_SIZE, N_SIZE);
//	Pochoir_Array<bool, N_RANK> b(N_SIZE, N_SIZE);
//	Pochoir_Array<bool, N_RANK> c(N_SIZE, N_SIZE);
    Pochoir <bool, N_RANK> life_2D, bt_life_2D;
//	Pochoir_Domain I(0, N_SIZE), J(0, N_SIZE);
#if 0
    Pochoir_Shape<2> life_shape_2D[9] = {{1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, 1, 1}, {0, -1, -1}, {0, 1, -1}, {0, -1, 1}};
#else
    Pochoir_Shape<2> life_shape_2D[9] = {{0, 0, 0}, {-1, 1, 0}, {-1, -1, 0}, {-1, 0, 1}, {-1, 0, -1}, {-1, 1, 1}, {-1, -1, -1}, {-1, 1, -1}, {-1, -1, 1}};
#endif

	for (int i = 0; i < N_SIZE; ++i) {
	for (int j = 0; j < N_SIZE; ++j) {
#if DEBUG 
		a(0, i, j) = ((i * N_SIZE + j) & 0x1) ? true : false;
		a(1, i, j) = 0;
#else
		a(0, i, j) = (rand() & 0x1) ? true : false;
		a(1, i, j) = 0; 
#endif
//        b(0, i, j) = a(0, i, j);
//        b(1, i, j) = 0;
//        c(0, i, j) = a(0, i, j);
//        c(1, i, j) = 0;
	} }

    printf("Game of Life : %d x %d, %d time steps\n", N_SIZE, N_SIZE, T_SIZE);

#if 1
#if 1
    life_2D.registerBoundaryFn(a, life_bv_2D);

    Pochoir_kernel_2D(life_2D_fn, t, i, j)
    int neighbors = a(t-1, i-1, j-1) + a(t-1, i-1, j) + a(t-1, i-1, j+1) +
                    a(t-1, i, j-1)                  + a(t-1, i, j+1) +
                    a(t-1, i+1, j-1) + a(t-1, i+1, j) + a(t-1, i+1, j+1);
    if (a(t-1, i, j) == true && neighbors < 2)
        a(t, i, j) = false;
    else if (a(t-1, i, j) == true && neighbors > 3) { 
        a(t, i, j) = false;
    } else if (a(t-1, i, j) == true && (neighbors == 2 || neighbors == 3)) {
        a(t, i, j) = a(t-1, i, j);
    } else if (a(t-1, i, j) == false && neighbors == 3) 
        a(t, i, j) = true;
    else
        a(t, i, j) = a(t-1, i, j);
    Pochoir_kernel_end

//    life_2D.registerArray(a);
    life_2D.registerShape(life_shape_2D);
//    life_2D.registerDomain(I, J);

	gettimeofday(&start, 0);
    for (int times = 0; times < TIMES; ++times) {
        life_2D.run(T_SIZE, life_2D_fn);
    }
	gettimeofday(&end, 0);
	std::cout << "Pochoir ET: consumed time :" << 1.0e3 * tdiff(&end, &start)/TIMES << "ms" << std::endl;
#endif

#if 0
    bt_life_2D.registerBoundaryFn(c, life_bv_2D);
    Pochoir_kernel_2D(bt_life_2D_fn, t, i, j)
    int neighbors = c(t-1, i-1, j-1) + c(t-1, i-1, j) + c(t-1, i-1, j+1) +
                    c(t-1, i, j-1)                  + c(t-1, i, j+1) +
                    c(t-1, i+1, j-1) + c(t-1, i+1, j) + c(t-1, i+1, j+1);
#if 0
    c(t, i, j) = ((c(t-1, i, j) && neighbors < 2) 
                || (c(t-1, i, j) && (neighbors == 2 || neighbors == 3))
                || (!c(t-1, i, j) && neighbors == 3))
                && !(c(t-1, i, j) && neighbors > 3);
#endif
#if 1
     int x = ((neighbors-2)>>31) | ~((neighbors-3)>>31); // x==0 iff neighbors==2 (otherwise -1)
     int y = ((neighbors-3)>>31) | ~((neighbors-4)>>31); // y==0 iff neighbors==3 (otherwise -1)
     //c(t,i,j) = 1&((~x&c(t-1,i,j))|~y);
     c(t,i,j) = 1&((~x&c(t-1,i,j)) | ~y);
#else
    bool set0 = (c(t-1, i, j) == false && neighbors == 3); /* set true */
    bool set1 = (c(t-1, i, j) == true && neighbors < 2) 
             || (c(t-1, i, j) == true && neighbors > 3); /* set false */
    c(t, i, j) = set0 ? true : (set1 ? false : c(t-1, i, j));
//    c(t, i, j) = pCond(set0, true, pCond(set1, false, c(t-1, i, j)));
#endif
    Pochoir_kernel_end

//    life_2D.registerArray(a);
    bt_life_2D.registerShape(life_shape_2D);
//    life_2D.registerDomain(I, J);

	gettimeofday(&start, 0);
    for (int times = 0; times < TIMES; ++times) {
        bt_life_2D.run(T_SIZE, bt_life_2D_fn);
    }
	gettimeofday(&end, 0);
	std::cout << "Pochoir ET (Bit Trick): consumed time :" << 1.0e3 * tdiff(&end, &start)/TIMES << "ms" << std::endl;
#endif
#endif

#if 0
	gettimeofday(&start, 0);
    // we can handle the boundary condition either by register a boundary function
    // or using following explicit modulo operation
    // b.registerBV(life_bv_2D);
    for (int times = 0; times < TIMES; ++times) {
	for (int t = 0; t < T_SIZE; ++t) {
	cilk_for (int i = 0; i < N_SIZE; ++i) {
	for (int j = 0; j < N_SIZE; ++j) {
        /* Periodic version */
    /* for idx5, i+N_SIZE+1 can be larger than 2 * N_SIZE, so we can NOT use pmod */
	size_t idx0 = (i + N_SIZE - 1) % ( N_SIZE);
	size_t idx1 = (j + N_SIZE - 1) % ( N_SIZE);
	size_t idx2 = (j + N_SIZE) % ( N_SIZE);
	size_t idx3 = (j + N_SIZE + 1) % ( N_SIZE);
	size_t idx4 = (i + N_SIZE) % ( N_SIZE);
	size_t idx5 = (i + N_SIZE + 1) % ( N_SIZE);
	int neighbors = b.interior(t, idx0, idx1) + b.interior(t, idx0, idx2) + b.interior(t, idx0, idx3) + b.interior(t, idx4, idx1) + b.interior(t, idx4, idx3) + b.interior(t, idx5, idx1) + b.interior(t, idx5, idx2) + b.interior(t, idx5, idx3);
	if (b.interior(t, idx4, idx2) == true && neighbors < 2)
	b.interior(t + 1, idx4, idx2) = false;
	else if (b.interior(t, idx4, idx2) == true && neighbors > 3)
	b.interior(t + 1, idx4, idx2) = false;
	else if (b.interior(t, idx4, idx2) == true && (neighbors == 2 || neighbors == 3))
	b.interior(t + 1, idx4, idx2) = b.interior(t, idx4, idx2);
	else if (b.interior(t, idx4, idx2) == false && neighbors == 3)
	b.interior(t + 1, idx4, idx2) = true;
    else
    b.interior(t+1, idx4, idx2) = b.interior(t, idx4, idx2);
	} } }
    }
	gettimeofday(&end, 0);
	std::cout << "Naive Loop: consumed time :" << 1.0e3 * tdiff(&end, &start) / TIMES << "ms" << std::endl;

#endif
#if 0
	t = T_SIZE;
    printf("compare a with b : ");
	for (int i = 0; i < N_SIZE; ++i) {
	for (int j = 0; j < N_SIZE; ++j) {
		check_result(t, i, j, a(t, i, j), b(t, i, j));
	} } 
    printf("passed!\n");

    printf("compare c with b : ");
	for (int i = 0; i < N_SIZE; ++i) {
	for (int j = 0; j < N_SIZE; ++j) {
		check_result(t, i, j, c(t, i, j), b(t, i, j));
	} } 
    printf("passed!\n");
#endif
	return 0;
}
