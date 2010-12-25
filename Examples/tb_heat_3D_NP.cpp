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

/* Test bench - 2D heat equation, Non-periodic version */
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
#define N_RANK 3
// #define N_SIZE 555
// #define T_SIZE 555
#define TOLERANCE (1e-6)

void check_result(int t, int i, int j, int k, double a, double b)
{
	if (abs(a - b) < TOLERANCE) {
//		printf("a(%d, %d, %d, %d) == b(%d, %d, %d, %d) == %f : passed!\n", t, i, j, k, t, i, j, k, a);
	} else {
		printf("a(%d, %d, %d, %d) = %f, b(%d, %d, %d, %d) = %f : FAILED!\n", t, i, j, k, a, t, i, j, k, b);
	}

}

    Pochoir_Boundary_3D(heat_bv_3D, arr, t, i, j, k)
        /* this is non-periodic boundary value */
        /* we already shrinked by using range I, J, K,
         * so the following code to set boundary index and
         * boundary rvalue is not necessary!!! 
         */
        if (i <= 0 || i >= arr.size(2)-1 
                || j <= 0 || j >= arr.size(1)-1
                || k <= 0 || k >= arr.size(0)-1)
            return 0;
        else
            return arr.get(t, i, j, k);
    Pochoir_Boundary_end

    template <typename Array>
    void print_array(Array const & a) {
        for (int i = 0; i < a.size(2); ++i) {
            for (int j = 0; j < a.size(1); ++j) {
        for (int k = 0; k < a.size(0); ++k) {
            printf("%g(%g) ", a.interior(1, i, j, k), a.interior(0, i, j, k));
        }
            }
            printf("\n");
        }
    }

int main(int argc, char * argv[])
{
	const int BASE = 1024;
	int t;
	struct timeval start, end;
    double min_tdiff = INF;
    int N_SIZE = 0, T_SIZE = 0;

    if (argc < 3) {
        printf("argc < 3, quit! \n");
        exit(1);
    }
    N_SIZE = StrToInt(argv[1]);
    T_SIZE = StrToInt(argv[2]);
    printf("N_SIZE = %d, T_SIZE = %d\n", N_SIZE, T_SIZE);
	/* data structure of Pochoir - row major */
	Pochoir_Array<double, N_RANK> a(N_SIZE, N_SIZE, N_SIZE), b(N_SIZE, N_SIZE, N_SIZE);
    Pochoir<double, N_RANK> heat_3D;
    Pochoir_Domain I(1, N_SIZE-1), J(1, N_SIZE-1), K(1, N_SIZE-1);
    Pochoir_Shape<3> heat_shape_3D[] = {{0, 0, 0, 0}, {-1, 1, 0, 0}, {-1, -1, 0, 0}, {-1, 0, 0, 0}, {-1, 0, 0, -1}, {-1, 0, 0, 1}, {-1, 0, 1, 0}, {-1, 0, -1, 0}};

	for (int i = 0; i < N_SIZE; ++i) {
	for (int j = 0; j < N_SIZE; ++j) {
    for (int k = 0; k < N_SIZE; ++k) {
        if (i == 0 || i == N_SIZE-1
            || j == 0 || j == N_SIZE-1
            || k == 0 || k == N_SIZE-1) {
            a(0, i, j, k) = a(1, i, j, k) = 0;
        } else {
#if DEBUG 
		    a(0, i, j, k) = i * N_SIZE * N_SIZE + j * N_SIZE + k;
		    a(1, i, j, k) = 0;
#else
            a(0, i, j, k) = 1.0 * (rand() % BASE); 
            a(1, i, j, k) = 0; 
#endif
        }
        b(0, i, j, k) = a(0, i, j, k);
        b(1, i, j, k) = 0;
	} } }

    Pochoir_kernel_3D(heat_3D_fn, t, i, j, k)
#if DEBUG
       a(t, i, j, k) = a(t-1, i-1, j-1, k-1) + 0.01; 
#else
	   a(t, i, j, k) = 
           0.125 * (a(t-1, i+1, j, k) - 2.0 * a(t-1, i, j, k) + a(t-1, i-1, j, k)) 
         + 0.125 * (a(t-1, i, j+1, k) - 2.0 * a(t-1, i, j, k) + a(t-1, i, j-1, k)) 
         + 0.125 * (a(t-1, i, j, k+1) - 2.0 * a(t-1, i, j, k) + a(t-1, i, j, k-1))
         + a(t-1, i, j, k);
#endif
    Pochoir_kernel_end

    /* we have to bind arrayInUse and Shape together 
     * => One arrayInUse, one shape[] => One slope[]
     * because each arrayInUse needs to know the slope to determine
     * the boundary region and when to call the user supplied boundary
     * value function
     */
//    heat_3D.registerBoundaryFn(a, heat_bv_3D);
    heat_3D.registerArray(a);
    heat_3D.registerShape(heat_shape_3D);
    heat_3D.registerDomain(I, J, K);

#if 1
    for (int times = 0; times < TIMES; ++times) {
	    gettimeofday(&start, 0);
        heat_3D.run(T_SIZE, heat_3D_fn);
	    gettimeofday(&end, 0);
        min_tdiff = min(min_tdiff, (1.0e3 * tdiff(&end, &start)));
    }
	std::cout << "Pochoir ET: consumed time :" << min_tdiff << "ms" << std::endl;

//    b.registerShape(heat_shape_2D);
//    b.registerBV(heat_bv_2D);
#endif
#if 1
    min_tdiff = INF;
    /* cilk_for + zero-padding */
    for (int times = 0; times < TIMES; ++times) {
	gettimeofday(&start, 0);
	for (int t = 1; t < T_SIZE+1; ++t) {
    cilk_for (int i = 1; i < N_SIZE-1; ++i) {
	for (int j = 1; j < N_SIZE-1; ++j) {
    for (int k = 1; k < N_SIZE-1; ++k) {
#if DEBUG
       b(t, i, j) = b(t-1, i-1, j-1, k-1) + 0.01; 
#else
	   b.interior(t, i, j, k) = 
           0.125 * (b.interior(t-1, i+1, j, k) - 2.0 * b.interior(t-1, i, j, k) + b.interior(t-1, i-1, j, k)) 
         + 0.125 * (b.interior(t-1, i, j+1, k) - 2.0 * b.interior(t-1, i, j, k) + b.interior(t-1, i, j-1, k)) 
         + 0.125 * (b.interior(t-1, i, j, k+1) - 2.0 * b.interior(t-1, i, j, k) + b.interior(t-1, i, j, k-1))
         + b.interior(t-1, i, j, k);
#endif
    } } } }
	gettimeofday(&end, 0);
    min_tdiff = min(min_tdiff, (1.0e3 * tdiff(&end, &start)));
    }
	std::cout << "Naive Loop: consumed time :" << min_tdiff << "ms" << std::endl;

	t = T_SIZE;
	for (int i = 1; i < N_SIZE-1; ++i) {
	for (int j = 1; j < N_SIZE-1; ++j) {
    for (int k = 1; k < N_SIZE-1; ++k) {
		check_result(t, i, j, k, a.interior(t, i, j, k), b.interior(t, i, j, k));
	} } }
#endif

#if 0
    printf("a = \n");
    print_array(a);
    printf("b = \n");
    print_array(b);
#endif
	return 0;
}
