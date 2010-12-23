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

#include "expr_stencil.hpp"

using namespace std;
#define SIMPLE 1
#define N_RANK 2
#define N_SIZE 555
#define T_SIZE 555
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
        if (new_i < 0)
            new_i += arr.size(1);
        else if (new_i >= arr.size(1))
            new_i -= arr.size(1);

        if (new_j < 0)
            new_j += arr.size(0);
        else if (new_j >= arr.size(0))
            new_j -= arr.size(0);
        return arr.get(t, new_i, new_j);
    Pochoir_Boundary_end

int main(void)
{
	int t;
	struct timeval start, end;
	/* data structure of Pochoir - row major */
	
	Pochoir_Array <bool, 2, 2> a(555, 555), b(555, 555);

	Pochoir_Stencil <bool, 2> life_2D;

	Pochoir_uRange I(0, 555 - 1), J(0, 555 - 1);

	Pochoir_Shape_info <2> life_shape_2D [9] = {{1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, 1, 1}, {0, -1, -1}, {0, 1, -1}, {0, -1, 1}};
life_2D.registerBoundaryFn(a, life_bv_2D);
	for (int i = 0; i < N_SIZE; ++i) {
	for (int j = 0; j < N_SIZE; ++j) {
#if DEBUG 
		a(0, i, j) = ((i * N_SIZE + j) & 0x1) ? true : false;
		a(1, i, j) = 0;
#else
		a(0, i, j) = (rand() & 0x1) ? true : false;
		a(1, i, j) = 0; 
#endif
        b(0, i, j) = a(0, i, j);
        b(1, i, j) = 0;
	} }

    printf("Game of Life : %d x %d, %d time steps\n", N_SIZE, N_SIZE, T_SIZE);

    Pochoir_kernel_2D(life_2D_fn, t, i, j)
	int neighbors = a(t, i - 1, j - 1) + a(t, i - 1, j) + a(t, i - 1, j + 1) + a(t, i, j - 1) + a(t, i, j + 1) + a(t, i + 1, j - 1) + a(t, i + 1, j) + a(t, i + 1, j + 1);
	if (a(t, i, j) == true && neighbors < 2)
	a(t + 1, i, j) = true;
	else if (a(t, i, j) == true && neighbors > 3)
	{
	a(t + 1, i, j) = false;
	}
	else if (a(t, i, j) == true && (neighbors == 2 || neighbors == 3))
	{
	a(t + 1, i, j) = a(t, i, j);
	}
	else if (a(t, i, j) == false && neighbors == 3)
	a(t + 1, i, j) = true;
	Pochoir_kernel_end
	life_2D.registerArrayInUse (a);
	life_2D.registerShape(life_shape_2D);
    life_2D.registerDomain(I, J);

	gettimeofday(&start, 0);
    
	Pochoir_obase_fn_2D(pointer_life_2D_fn, t0, t1, grid)
	grid_info<2> l_grid = grid;
	bool * pt_a_1;
	bool * pt_a_0;
	
	bool * a_base = a.data();
	const int l_a_total_size = a.total_size();
	int gap_a_1, gap_a_0;
	const int l_stride_a_1 = a.stride(1), l_stride_a_0 = a.stride(0);

	for (int t = t0; t < t1; ++t) { 
	pt_a_0 = a_base + ((t) & 1) * l_a_total_size + l_grid.x0[1] * l_stride_a_1 + l_grid.x0[0] * l_stride_a_0;
	pt_a_1 = a_base + ((t + 1) & 1) * l_a_total_size + l_grid.x0[1] * l_stride_a_1 + l_grid.x0[0] * l_stride_a_0;
	
	gap_a_1 = l_stride_a_1 + (l_grid.x0[0] - l_grid.x1[0]) * l_stride_a_0;
	for (int i = l_grid.x0[1]; i < l_grid.x1[1]; ++i,
	pt_a_0 += gap_a_1, 
	pt_a_1 += gap_a_1) {
	#pragma ivdep
	for (int j = l_grid.x0[0]; j < l_grid.x1[0]; ++j,
	++pt_a_0, 
	++pt_a_1) {
	
	int neighbors = pt_a_0[l_stride_a_1 * (-1) + l_stride_a_0 * (-1)] + pt_a_0[l_stride_a_1 * (-1)] + pt_a_0[l_stride_a_1 * (-1) + l_stride_a_0 * (1)] + pt_a_0[l_stride_a_0 * (-1)] + pt_a_0[l_stride_a_0 * (1)] + pt_a_0[l_stride_a_1 * (1) + l_stride_a_0 * (-1)] + pt_a_0[l_stride_a_1 * (1)] + pt_a_0[l_stride_a_1 * (1) + l_stride_a_0 * (1)];
	if (pt_a_0[0] == true && neighbors < 2)
	pt_a_1[0] = true;
	else if (pt_a_0[0] == true && neighbors > 3)
	{
	pt_a_1[0] = false;
	}
	else if (pt_a_0[0] == true && (neighbors == 2 || neighbors == 3))
	{
	pt_a_1[0] = pt_a_0[0];
	}
	else if (pt_a_0[0] == false && neighbors == 3)
	pt_a_1[0] = true;
	} } /* end for (sub-trapezoid) */ 
	/* Adjust sub-trapezoid! */
	for (int i = 0; i < 2; ++i) {
		l_grid.x0[i] += l_grid.dx0[i]; l_grid.x1[i] += l_grid.dx1[i];
	}
	} /* end for t */
	Pochoir_kernel_end

	life_2D.run_obase(555, pointer_life_2D_fn, life_2D_fn);
	gettimeofday(&end, 0);
	std::cout << "Pochoir ET: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

	gettimeofday(&start, 0);
    // we can handle the boundary condition either by register a boundary function
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
	b.interior(t + 1, idx4, idx2) = true;
	else if (b.interior(t, idx4, idx2) == true && neighbors > 3)
	b.interior(t + 1, idx4, idx2) = false;
	else if (b.interior(t, idx4, idx2) == true && (neighbors == 2 || neighbors == 3))
	b.interior(t + 1, idx4, idx2) = b.interior(t, idx4, idx2);
	else if (b.interior(t, idx4, idx2) == false && neighbors == 3)
	b.interior(t + 1, idx4, idx2) = true;
	} } }
	gettimeofday(&end, 0);
	std::cout << "Naive Loop: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

	t = T_SIZE;
	for (int i = 0; i < N_SIZE; ++i) {
	for (int j = 0; j < N_SIZE; ++j) {
		check_result(t, i, j, a(t, i, j), b(t, i, j));
	} } 

	return 0;
}
