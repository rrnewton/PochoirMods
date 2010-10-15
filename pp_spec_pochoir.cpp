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

void check_result(int t, int i, double a, double b)
{
	if (abs(a-b)< TOLERANCE) {
//		printf("a(%d, %d) == b(%d, %d) == %f : passed!\n", t, i, t, i, a);
} else {
		printf("a(%d, %d) = %f, b(%d, %d) = %f : FAILED!\n", t, i, a, t, i, b);
	}

}

#if SIMPLE
    /* this is non-periodic boundary value */
    Pochoir_Boundary_1D(heat_bv_1D, arr, t, i)
        if (i <= 0 || i >= arr.size(0)-1)
            return 0;
        else
            return arr.get(t, i);
    Pochoir_Boundary_end
#else
    /* this is periodic boundary value */
    Pochoir_Boundary_1D(heat_bv_1D, arr, t, i)
        int new_i = i;
        new_i = (new_i < 0) ? (new_i+arr.size(0)) : new_i;
        new_i = (new_i >= arr.size(0)) ? (new_i-arr.size(0)) : new_i;
        return arr.get(t, new_i);
    Pochoir_Boundary_end
#endif

    Pochoir_Boundary_2D(heat_bv_2D, arr, t, i, j)
        /* this is non-periodic boundary value */
        /* we already shrinked by using range I, J, K,
         * so the following code to set boundary index and
         * boundary rvalue is not necessary!!! 
         */
        if (i <= 0 || i >= N_SIZE-1 || j <= 0 || j >= N_SIZE-1)
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
	
	Pochoir_SArray <double, 2, 2> a(555, 555);
	Pochoir_SArray <double, 2, 2> b(555, 555);
	Pochoir_SArray <double, 1, 2> c(555);
	Pochoir_SArray <double, 1, 2> d(555);
	Pochoir_Stencil <double, 2> heat_2D;
	Pochoir_Stencil <double, 1> heat_1D;
	Pochoir_uRange I(0, 554), J(0, 554), K(0, 554);
	Pochoir_Shape_info <1> heat_shape_1D [3] = {{1, 0}, {0, 1}, {0, -1}};
	Pochoir_Shape_info <2> heat_shape_2D [5] = {{1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, -1}, {0, 0, 1}};heat_2D.registerBoundaryFn(a, heat_bv_2D);
//    b.registerBV(heat_bv_2D);
heat_1D.registerBoundaryFn(c, heat_bv_1D);
    d.registerBV(heat_bv_1D);

    printf("array_length(1D) = %lu\n", ArraySize(heat_shape_1D));
    printf("array_length(2D) = %lu\n", ArraySize(heat_shape_2D));
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

	for (int i = 0; i < N_SIZE; ++i) {
        if (i == 0 || i == N_SIZE-1) {
            c(0, i) = c(1, i) = 0;
        } else {
#if DEBUG 
            c(0, i) = i;
            c(1, i) = 0;
#else
            c(0, i) = 1.0 * (rand() % BASE); 
            c(1, i) = 0; 
#endif
        }
        d(0, i) = c(0, i);
        d(1, i) = 0;
	} 

#if SIMPLE
	cout << endl << "\n /* a(T+1, J, I) = 0.01 + a(T, J, I) */" << endl;
    Pochoir_kernel_2D(heat_2D_fn, t, i, j)
	a(t + 1, i, j) = 1.0e-2 + a(t, i - 1, j + 1);
	Pochoir_kernel_end
	Pochoir_kernel_1D(heat_1D_fn, t, i)
	c(t + 1, i) = 1.0e-2 + c(t, i - 1);
	#if DEBUG	/* Unrecognized! */
	printf("c(%d, %d) = 0.01 + c(%d, %d)\t", t+1, i, t, i-1);	/* Unrecognized! */
	printf("%f = 0.01 + %f\n", double(c(t+1, i)), double(c(t, i-1)));	/* Unrecognized! */
	#endif	/* Unrecognized! */
	Pochoir_kernel_end
	heat_1D.registerArrayInUse (c);
	heat_1D.registerShape(heat_shape_1D);
    heat_1D.registerDomain(I);
    heat_2D.registerArrayInUse (a);
	heat_2D.registerShape(heat_shape_2D);
    heat_2D.registerDomain(I, K);

	gettimeofday(&start, 0);
    
	Pochoir_obase_fn_1D(obase_heat_1D_fn, t0, t1, grid)
	grid_info<1> l_grid = grid;
	Pochoir_Iterator<double, 1> iter0(c);
	Pochoir_Iterator<double, 1> iter1(c);
	int gap_c_0;
	const int l_stride_c_0 = c.stride(0);

	for (int t = t0; t < t1; ++t) { 
	iter0.set(t + 1, l_grid.x0[0]);
	iter1.set(t, l_grid.x0[0] - 1);
	
	for (int i = l_grid.x0[0]; i < l_grid.x1[0]; ++i,
	++iter0, 
	++iter1) {
	
	iter0 = 1.0e-2 + iter1;
	#if DEBUG	/* Unrecognized! */
	printf("c(%d, %d) = 0.01 + c(%d, %d)\t", t+1, i, t, i-1);	/* Unrecognized! */
	printf("%f = 0.01 + %f\n", double(c(t+1, i)), double(c(t, i-1)));	/* Unrecognized! */
	#endif	/* Unrecognized! */
	} /* end for (sub-trapezoid) */ 
	/* Adjust sub-trapezoid! */
	for (int i = 0; i < 1; ++i) {
		l_grid.x0[i] += l_grid.dx0[i]; l_grid.x1[i] += l_grid.dx1[i];
	}
	} /* end for t */
	Pochoir_kernel_end

	heat_1D.run_obase(555, obase_heat_1D_fn, heat_1D_fn);
	
	Pochoir_obase_fn_2D(obase_heat_2D_fn, t0, t1, grid)
	grid_info<2> l_grid = grid;
	Pochoir_Iterator<double, 2> iter0(a);
	Pochoir_Iterator<double, 2> iter1(a);
	int gap_a_1, gap_a_0;
	const int l_stride_a_1 = a.stride(1), l_stride_a_0 = a.stride(0);

	for (int t = t0; t < t1; ++t) { 
	iter0.set(t + 1, l_grid.x0[1], l_grid.x0[0]);
	iter1.set(t, l_grid.x0[1] - 1, l_grid.x0[0] + 1);
	
	gap_a_1 = l_stride_a_1 + (l_grid.x0[0] - l_grid.x1[0]) * l_stride_a_0;
	for (int i = l_grid.x0[1]; i < l_grid.x1[1]; ++i,
	iter0.inc(gap_a_1), 
	iter1.inc(gap_a_1)) {
	for (int j = l_grid.x0[0]; j < l_grid.x1[0]; ++j,
	++iter0, 
	++iter1) {
	
	iter0 = 1.0e-2 + iter1;
	} } /* end for (sub-trapezoid) */ 
	/* Adjust sub-trapezoid! */
	for (int i = 0; i < 2; ++i) {
		l_grid.x0[i] += l_grid.dx0[i]; l_grid.x1[i] += l_grid.dx1[i];
	}
	} /* end for t */
	Pochoir_kernel_end

	heat_2D.run_obase(555, obase_heat_2D_fn, heat_2D_fn);
	gettimeofday(&end, 0);
	std::cout << "Pochoir : consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

    b.registerShape(heat_shape_2D);
    d.registerShape(heat_shape_1D);

	gettimeofday(&start, 0);
    for (int t = 0; t < T_SIZE; ++t) {
	cilk_for (int i = 1; i <= N_SIZE-2; ++i) {
	for (int j = 1; j <= N_SIZE-2; ++j) {
        b.interior(t+1, i, j) = 0.01 + b.interior(t, i-1, j+1);
	} } }
	for (int t = 0; t < T_SIZE; ++t) {
	cilk_for (int i = 0; i <= N_SIZE-1; ++i) {
        d(t+1, i) = 0.01 + d(t, i-1);
#if DEBUG
        printf("d(%d, %d) = 0.01 + d(%d, %d)\t", t+1, i, t, i-1);
        printf("%f = 0.01 + %f\n", double(d(t+1, i)), double(d(t, i-1)));
#endif
	} } 
	gettimeofday(&end, 0);
	std::cout << "Naive Loop: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

#if 0
    a.unregisterBV();
    b.unregisterBV();
    c.unregisterBV();
    d.unregisterBV();
#endif
	t = T_SIZE;
	for (int i = 1; i <= N_SIZE-2; ++i) {
	for (int j = 1; j <= N_SIZE-2; ++j) {
		check_result(t, i, j, a.interior(t, i, j), b.interior(t, i, j));
	} } 
	for (int i = 0; i <= N_SIZE-1; ++i) {
		check_result(t, i, c.interior(t, i), d.interior(t, i));
	}  
#else
	cout << "a(T+1, J, I) = 0.125 * (a(T, J+1, I) - 2.0 * a(T, J, I) + a(T, J-1, I)) + 0.125 * (a(T, J, I+1) - 2.0 * a(T, J, I) + a(T, J, I-1)) + a(T, J, I)" << endl;
    Pochoir_kernel_2D(heat_2D_fn, t, i, k)
	a(t + 1, i, k) = 0.125 * (a(t, i + 1, k) - 2.0 * a(t, i, k) + a(t, i - 1, k)) + 0.125 * (a(t, i, k + 1) - 2.0 * a(t, i, k) + a(t, i, k - 1)) + a(t, i, k);
	Pochoir_kernel_end
	Pochoir_kernel_1D(heat_1D_fn, t, i)
	c(t + 1, i) = 0.125 * (c(t, i - 1) - 2.0 * c(t, i) + c(t, i + 1)) + c(t, i);
	Pochoir_kernel_end
	heat_2D.registerArrayInUse (a);
	heat_2D.registerShape(heat_shape_2D);
    heat_2D.registerDomain(I, K);
    heat_1D.registerArrayInUse (c);
	heat_1D.registerShape(heat_shape_1D);
    heat_1D.registerDomain(J);

	gettimeofday(&start, 0);
    
	Pochoir_obase_fn_1D(obase_heat_1D_fn, t0, t1, grid)
	grid_info<1> l_grid = grid;
	Pochoir_Iterator<double, 1> iter0(c);
	Pochoir_Iterator<double, 1> iter1(c);
	Pochoir_Iterator<double, 1> iter2(c);
	Pochoir_Iterator<double, 1> iter3(c);
	int gap_c_0;
	const int l_stride_c_0 = c.stride(0);

	for (int t = t0; t < t1; ++t) { 
	iter0.set(t + 1, l_grid.x0[0]);
	iter1.set(t, l_grid.x0[0] - 1);
	iter2.set(t, l_grid.x0[0]);
	iter3.set(t, l_grid.x0[0] + 1);
	
	for (int i = l_grid.x0[0]; i < l_grid.x1[0]; ++i,
	++iter0, 
	++iter1, 
	++iter2, 
	++iter3) {
	
	iter0 = 0.125 * (iter1 - 2.0 * iter2 + iter3) + iter2;
	} /* end for (sub-trapezoid) */ 
	/* Adjust sub-trapezoid! */
	for (int i = 0; i < 1; ++i) {
		l_grid.x0[i] += l_grid.dx0[i]; l_grid.x1[i] += l_grid.dx1[i];
	}
	} /* end for t */
	Pochoir_kernel_end

	heat_1D.run_obase(555, obase_heat_1D_fn, heat_1D_fn);
	
	Pochoir_obase_fn_2D(obase_heat_2D_fn, t0, t1, grid)
	grid_info<2> l_grid = grid;
	Pochoir_Iterator<double, 2> iter0(a);
	Pochoir_Iterator<double, 2> iter1(a);
	Pochoir_Iterator<double, 2> iter2(a);
	Pochoir_Iterator<double, 2> iter3(a);
	Pochoir_Iterator<double, 2> iter4(a);
	Pochoir_Iterator<double, 2> iter5(a);
	int gap_a_1, gap_a_0;
	const int l_stride_a_1 = a.stride(1), l_stride_a_0 = a.stride(0);

	for (int t = t0; t < t1; ++t) { 
	iter0.set(t + 1, l_grid.x0[1], l_grid.x0[0]);
	iter1.set(t, l_grid.x0[1] + 1, l_grid.x0[0]);
	iter2.set(t, l_grid.x0[1], l_grid.x0[0]);
	iter3.set(t, l_grid.x0[1] - 1, l_grid.x0[0]);
	iter4.set(t, l_grid.x0[1], l_grid.x0[0] + 1);
	iter5.set(t, l_grid.x0[1], l_grid.x0[0] - 1);
	
	gap_a_1 = l_stride_a_1 + (l_grid.x0[0] - l_grid.x1[0]) * l_stride_a_0;
	for (int i = l_grid.x0[1]; i < l_grid.x1[1]; ++i,
	iter0.inc(gap_a_1), 
	iter1.inc(gap_a_1), 
	iter2.inc(gap_a_1), 
	iter3.inc(gap_a_1), 
	iter4.inc(gap_a_1), 
	iter5.inc(gap_a_1)) {
	for (int k = l_grid.x0[0]; k < l_grid.x1[0]; ++k,
	++iter0, 
	++iter1, 
	++iter2, 
	++iter3, 
	++iter4, 
	++iter5) {
	
	iter0 = 0.125 * (iter1 - 2.0 * iter2 + iter3) + 0.125 * (iter4 - 2.0 * iter2 + iter5) + iter2;
	} } /* end for (sub-trapezoid) */ 
	/* Adjust sub-trapezoid! */
	for (int i = 0; i < 2; ++i) {
		l_grid.x0[i] += l_grid.dx0[i]; l_grid.x1[i] += l_grid.dx1[i];
	}
	} /* end for t */
	Pochoir_kernel_end

	heat_2D.run_obase(555, obase_heat_2D_fn, heat_2D_fn);
	gettimeofday(&end, 0);
	std::cout << "Pochoir ET: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

    b.registerShape(heat_shape_2D);
    d.registerShape(heat_shape_1D);

	gettimeofday(&start, 0);
	for (int t = 0; t < T_SIZE; ++t) {
    cilk_for (int i = 1; i <= N_SIZE-2; ++i) {
	for (int k = 1; k <= N_SIZE-2; ++k) {
        b.interior(t+1, i, k) = 0.125 * (b.interior(t, i+1, k) - 2.0 * b.interior(t, i, k) + b.interior(t, i-1, k)) + 0.125 * (b.interior(t, i, k+1) - 2.0 * b.interior(t, i, k) + b.interior(t, i, k-1)) + b.interior(t, i, k); } } }
	for (int t = 0; t < T_SIZE; ++t) {
    cilk_for (int i = 0; i <= N_SIZE-1; ++i) {
        /* conditional instruction is more expensive than modulo operation!!! */
        d(t+1, i) = 0.125 * (d(t, i+1) - 2.0 * d(t, i) + d(t, i-1)) + d(t, i);
//        d(t+1, idx0) = 0.01 + d(t, idx2);
} } 
	gettimeofday(&end, 0);
	std::cout << "Naive Loop: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

	t = T_SIZE;
	for (int i = 1; i <= N_SIZE-2; ++i) {
	for (int j = 1; j <= N_SIZE-2; ++j) {
		check_result(t, i, j, a.interior(t, i, j), b.interior(t, i, j));
	} } 
	for (int i = 0; i <= N_SIZE-1; ++i) {
		check_result(t, i, c(t, i), d(t, i));
	}  
#endif

#if 0
//	cout << "I = " << I << endl;
cout << "c = " << c << endl;
//	cout << "P1 = " << P1 << endl;
cout << "d = " << d << endl;
#endif
	return 0;
}

