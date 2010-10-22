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

#include "expr_stencil.hpp"

using namespace std;
#define SIMPLE 0
#define N_RANK 2
#define N_SIZE 555
#define T_SIZE 555
#define TOLERANCE (1e-6)
void check_result(int t, int j, int i, double a, double b)
{
	if (abs(a - b) < TOLERANCE) {
//		printf("a(%d, %d, %d) == b(%d, %d, %d) == %f : passed!\n", t, j, i, t, j, i, a);
} else {
		printf("a(%d, %d, %d) = %f, b(%d, %d, %d) = %f : FAILED!\n", t, j, i, a, t, j, i, b);
	}

}

    Pochoir_Boundary_2D(heat_bv_2D, arr, t, i, j)
        /* this is non-periodic boundary value */
        /* we already shrinked by using range I, J, K,
         * so the following code to set boundary index and
         * boundary rvalue is not necessary!!! 
         */
        if (i <= 0 || i >= arr.size(1)-1 || j <= 0 || j >= arr.size(0)-1)
            return 0;
        else
            return arr.get(t, i, j);
    Pochoir_Boundary_end

int main(void)
{
	const int BASE = 1024;
	int t;
	struct timeval start, end;
	/* data structure of Pochoir - row major */
	
	Pochoir_Array <double, 2, 2> a(555, 555), b(555, 555);

	Pochoir_Stencil <double, 2> heat_2D;

	Pochoir_uRange I(0, 555 - 1), J(0, 555 - 1);

	Pochoir_Shape_info <2> heat_shape_2D [5] = {{1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, -1}, {0, 0, 1}};
heat_2D.registerBoundaryFn(a, heat_bv_2D);
	for (int i = 0; i < N_SIZE; ++i) {
	for (int j = 0; j < N_SIZE; ++j) {
        if (i == 0 || i == N_SIZE-1
            || j == 0 || j == N_SIZE-1) {
            a(0, i, j) = a(1, i, j) = 0;
        } else {
#if DEBUG 
		    a(0, i, j) = i * N_SIZE + j;
		    a(1, i, j) = 0;
#else
            a(0, i, j) = 1.0 * (rand() % BASE); 
            a(1, i, j) = 0; 
#endif
        }
        b(0, i, j) = a(0, i, j);
        b(1, i, j) = 0;
	} }

	cout << "a(T+1, J, I) = 0.125 * (a(T, J+1, I) - 2.0 * a(T, J, I) + a(T, J-1, I)) + 0.125 * (a(T, J, I+1) - 2.0 * a(T, J, I) + a(T, J, I-1)) + a(T, J, I)" << endl;
    Pochoir_kernel_2D(heat_2D_fn, t, i, j)
	a(t + 1, i, j) = 0.125 * (a(t, i + 1, j) - 2.0 * a(t, i, j) + a(t, i - 1, j)) + 0.125 * (a(t, i, j + 1) - 2.0 * a(t, i, j) + a(t, i, j - 1)) + a(t, i, j);
	Pochoir_kernel_end
	heat_2D.registerArrayInUse (a);
	heat_2D.registerShape(heat_shape_2D);
    heat_2D.registerDomain(I, J);

	gettimeofday(&start, 0);
    
	Pochoir_obase_fn_2D(pointer_heat_2D_fn, t0, t1, grid)
	grid_info<2> l_grid = grid;
	double * pt_a_1;
	double * pt_a_0;
	
	double * a_base = a.data();
	const int l_a_total_size = a.total_size();
	int gap_a_1, gap_a_0;
	const int l_stride_a_1 = a.stride(1), l_stride_a_0 = a.stride(0);

	for (int t = t0; t < t1; ++t) { 
	pt_a_0 = a_base + ((t + 1) & 1) * l_a_total_size + l_grid.x0[1] * l_stride_a_1 + l_grid.x0[0] * l_stride_a_0;
	pt_a_1 = a_base + ((t) & 1) * l_a_total_size + l_grid.x0[1] * l_stride_a_1 + l_grid.x0[0] * l_stride_a_0;
	
	gap_a_1 = l_stride_a_1 + (l_grid.x0[0] - l_grid.x1[0]) * l_stride_a_0;
	for (int i = l_grid.x0[1]; i < l_grid.x1[1]; ++i,
	pt_a_0 += gap_a_1, 
	pt_a_1 += gap_a_1) {
	#pragma ivdep
	for (int j = l_grid.x0[0]; j < l_grid.x1[0]; ++j,
	++pt_a_0, 
	++pt_a_1) {
	
	pt_a_0[0] = 0.125 * (pt_a_1[l_stride_a_1 * (1)] - 2.0 * pt_a_1[0] + pt_a_1[l_stride_a_1 * (-1)]) + 0.125 * (pt_a_1[l_stride_a_0 * (1)] - 2.0 * pt_a_1[0] + pt_a_1[l_stride_a_0 * (-1)]) + pt_a_1[0];
	} } /* end for (sub-trapezoid) */ 
	/* Adjust sub-trapezoid! */
	for (int i = 0; i < 2; ++i) {
		l_grid.x0[i] += l_grid.dx0[i]; l_grid.x1[i] += l_grid.dx1[i];
	}
	} /* end for t */
	Pochoir_kernel_end

	heat_2D.run_obase(555, pointer_heat_2D_fn, heat_2D_fn);
	gettimeofday(&end, 0);
	std::cout << "Pochoir ET: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

    b.registerShape(heat_shape_2D);

	gettimeofday(&start, 0);
    /* cilk_for + zero-padding */
	for (int t = 0; t < T_SIZE; ++t) {
    cilk_for (int i = 1; i <= N_SIZE-2; ++i) {
	for (int j = 1; j <= N_SIZE-2; ++j) {
        b.interior(t+1, i, j) = 0.125 * (b.interior(t, i+1, j) - 2.0 * b.interior(t, i, j) + b.interior(t, i-1, j)) + 0.125 * (b.interior(t, i, j+1) - 2.0 * b.interior(t, i, j) + b.interior(t, i, j-1)) + b.interior(t, i, j); } } }
	gettimeofday(&end, 0);
	std::cout << "Naive Loop: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

	t = T_SIZE;
	for (int i = 1; i <= N_SIZE-2; ++i) {
	for (int j = 1; j <= N_SIZE-2; ++j) {
		check_result(t, i, j, a.interior(t, i, j), b.interior(t, i, j));
	} } 

	return 0;
}

