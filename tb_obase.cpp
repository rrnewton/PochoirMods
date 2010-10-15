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
#define OBASE 1
/* N_RANK includes both time and space dimensions */
#define N_RANK 2
// #define N_SIZE 1555
// #define T_SIZE 1555
#define TOLERANCE (1e-6)

#if 0
    Pochoir_BV_1D(heat_bv_1D, arr, t, i)
        /* but this version has a severe problem is that :
         * if user write :
         * - if (i == 0)
         *      return arr(t, 0);
         * which will invoke endless recursive function call of arr(t, 0)
         */
        if (i == 0 || i == N_SIZE-1)
            return 0;
        else
            return arr(t, i);
    Pochoir_fn_end
#endif
    /* for BValue, we have to return something:
     * We use set()/get() pair to avoid self-recursion!
     * - for NP: 
     *      if (i <= 0) {
     *          arr.set(t, 0) = 0;
     *          return arr.get(t, 0);
     *      } 
     * - for P:
     *      if (i < 0) {
     *          int new_i = i+N_SIZE;
     *          return arr.get(t, new_i);
     *      }
     * A big problem in get/set pair: by setting boundary for only rvalue,
     * we still compute the boundary value of lvalue, so it will be different
     * from that just setting boundary value (lvalue) to some specific bvalue!!!
     */

Pochoir_BV_Declare_1D(heat_bv_1D, arr, t, i);
Pochoir_BV_Declare_2D(heat_bv_2D, arr, t, i, j);

int N_SIZE=0, T_SIZE=0;
int main(int argc, char * argv[])
{
	const int BASE = 1024;
	int t;
	struct timeval start, end;
    if (argc < 3) {
        printf("argc < 3, quit!\n");
        exit(1);
    }
    N_SIZE = StrToInt(argv[1]);
    T_SIZE = StrToInt(argv[2]);
    printf("N_SIZE = %d, T_SIZE = %d\n", N_SIZE, T_SIZE);

	/* data structure of Pochoir - row major */
	SArray<double, N_RANK> a(N_SIZE, N_SIZE);
	SArray<double, N_RANK-1> c(N_SIZE);
    Stencil<N_RANK> heat_2D;
    Stencil<N_RANK-1> heat_1D;
    uRange I(1, N_SIZE-2), J(0, N_SIZE-1), K(1, N_SIZE-2);
    shape_info<1> heat_shape_1D[3] = {{0}, {-1}, {1}};
    shape_info<2> heat_shape_2D[5] = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    a.registerBV(heat_bv_2D);
    c.registerBV(heat_bv_1D);

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
	} 

#if SIMPLE
	cout << endl << "\na(T+1, J, I) = 0.01 + a(T, J, I)" << endl;

    Pochoir_obase_fn_2D(heat_2D_safe_fn, t0, t1, grid)
        grid_info<2> l_grid = grid;
        /* this iterator is also a proxy for SArray */
        SIter<double, 2> iter0(a);
        SIter<double, 2> iter1(a);
        int gap1;
        const int l_stride0 = a.stride(0), l_stride1 = a.stride(1);
        for (int t = t0; t < t1; ++t) {
            iter0.set(t+1, l_grid.x0[1], l_grid.x0[0]);
            iter1.set(t, l_grid.x0[1]-1, l_grid.x0[0]+1);
            //iter1.set(t, l_grid.x0[1], l_grid.x0[0]);
            gap1 = l_stride1 + (l_grid.x0[0] - l_grid.x1[0]) * l_stride0;
            for (int i = l_grid.x0[1]; i < l_grid.x1[1]; ++i, iter0.inc(gap1), iter1.inc(gap1)) {
        for (int j = l_grid.x0[0]; j < l_grid.x1[0]; ++j, ++iter0, ++iter1) {
//            a(t+1, i, j) = 0.01 + a(t, i-1, j+1);
              iter0 = 0.01 + iter1;
        }
            }
            for (int i = 0; i < 2; ++i) {
                l_grid.x0[i] +=l_grid.dx0[i]; l_grid.x1[i] += l_grid.dx1[i];
            }
        }
    Pochoir_fn_end

    Pochoir_fn_2D(heat_2D_unsafe_fn, t, i, j)
        a(t+1, i, j) = 0.01 + a(t, i-1, j+1);
        //a(t+1, i, j) = 0.01 + a(t, i, j);
    Pochoir_fn_end

    Pochoir_obase_fn_1D(heat_1D_safe_fn, t0, t1, grid)
        grid_info<1> l_grid = grid;
        SIter<double, 1> iter0(c);
        SIter<double, 1> iter1(c);
        for (int t = t0; t < t1; ++t) {
            iter0.set(t+1, l_grid.x0[0]);
            iter1.set(t, l_grid.x0[0]);
            for (int i = l_grid.x0[0]; i < l_grid.x1[0]; ++i, ++iter0, ++iter1) {
//            c(t+1, i) = 0.01 + c(t, i);
                iter0 = 0.01 + iter1;
            }
            for (int i = 0; i < 1; ++i) {
                l_grid.x0[i] +=l_grid.dx0[i]; l_grid.x1[i] += l_grid.dx1[i];
            }
        }
    Pochoir_fn_end

    Pochoir_fn_1D(heat_1D_unsafe_fn, t, i)
        c(t+1, i) = 0.01 + c(t, i);
    Pochoir_fn_end

    /* we have to bind arrayInUse and Shape together 
     * => One arrayInUse, one shape[] => One slope[]
     * because each arrayInUse needs to know the slope to determine
     * the boundary region and when to call the user supplied boundary
     * value function
     */
    heat_2D.registerArrayInUse(a, 5, heat_shape_2D, I, K);
    heat_1D.registerArrayInUse(c, 2, heat_shape_1D, I);

	gettimeofday(&start, 0);
#if 0
    heat_1D.run(T_SIZE, heat_1D_fn);
    heat_2D.run(T_SIZE, heat_2D_fn);
#else
    heat_1D.run_op(T_SIZE, heat_1D_safe_fn, heat_1D_unsafe_fn);
    heat_2D.run_op(T_SIZE, heat_2D_safe_fn, heat_2D_unsafe_fn);
#endif
	gettimeofday(&end, 0);
	std::cout << "Pochoir : consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

#else
	cout << "a(T+1, J, I) = 0.125 * (a(T, J+1, I) - 2.0 * a(T, J, I) + a(T, J-1, I)) + 0.125 * (a(T, J, I+1) - 2.0 * a(T, J, I) + a(T, J, I-1)) + a(T, J, I)" << endl;
    Pochoir_obase_fn_2D(heat_2D_safe_fn, t0, t1, grid)
        grid_info<2> l_grid = grid;
        /* this iterator is also a proxy for SArray */
        SIter<double, 2> iter0(a), iter1(a), iter2(a), iter3(a), iter4(a), iter5(a);
        int gap1;
        const int l_stride0 = a.stride(0), l_stride1 = a.stride(1);
        for (int t = t0; t < t1; ++t) {
            iter0.set(t+1, l_grid.x0[1], l_grid.x0[0]);
            iter1.set(t, l_grid.x0[1]+1, l_grid.x0[0]);
            iter2.set(t, l_grid.x0[1], l_grid.x0[0]);
            iter3.set(t, l_grid.x0[1]-1, l_grid.x0[0]);
            iter4.set(t, l_grid.x0[1], l_grid.x0[0]+1);
            iter5.set(t, l_grid.x0[1], l_grid.x0[0]-1);
            gap1 = l_stride1 + (l_grid.x0[0] - l_grid.x1[0]) * l_stride0;
            for (int i = l_grid.x0[1]; 
                    i < l_grid.x1[1]; 
                    ++i, 
                    iter0.inc(gap1), iter1.inc(gap1), 
                    iter2.inc(gap1), iter3.inc(gap1),
                    iter4.inc(gap1), iter5.inc(gap1)) {
        for (int j = l_grid.x0[0]; 
                j < l_grid.x1[0]; 
                ++j, 
                ++iter0, ++iter1, ++iter2, ++iter3, ++iter4, ++iter5) {
//	        a(t+1, i, k) = 0.125 * (a(t, i+1, k) - 2.0 * a(t, i, k) + a(t, i-1, k)) + 0.125 * (a(t, i, k+1) - 2.0 * a(t, i, k) + a(t, i, k-1)) + a(t, i, k);
	        iter0 = 0.125 * (iter1 - 2.0 * iter2 + iter3) + 0.125 * (iter4 - 2.0 * iter2 + iter5) + iter2;
        }
            }
            for (int i = 0; i < 2; ++i) {
                l_grid.x0[i] +=l_grid.dx0[i]; l_grid.x1[i] += l_grid.dx1[i];
            }
        }
    Pochoir_fn_end

    Pochoir_fn_2D(heat_2D_unsafe_fn, t, i, k)
	    a(t+1, i, k) = 0.125 * (a(t, i+1, k) - 2.0 * a(t, i, k) + a(t, i-1, k)) + 0.125 * (a(t, i, k+1) - 2.0 * a(t, i, k) + a(t, i, k-1)) + a(t, i, k);
    Pochoir_fn_end

    Pochoir_obase_fn_1D(heat_1D_safe_fn, t0, t1, grid)
        grid_info<1> l_grid = grid;
        SIter<double, 1> iter0(c), iter1(c), iter2(c), iter3(c);
        for (int t = t0; t < t1; ++t) {
            iter0.set(t+1, l_grid.x0[0]);
            iter1.set(t, l_grid.x0[0]-1);
            iter2.set(t, l_grid.x0[0]);
            iter3.set(t, l_grid.x0[0]+1);
            for (int i = l_grid.x0[0]; 
                    i < l_grid.x1[0]; 
                    ++i, 
                    ++iter0, ++iter1, ++iter2, ++iter3) {
                // c(t+1, i) = 0.125 * (c(t, i-1) - 2.0 * c(t, i) + c(t, i+1)) + c(t, i);
                iter0 = 0.125 * (iter1 - 2.0 * iter2 + iter3) + iter2;
            }
            for (int i = 0; i < 1; ++i) {
                l_grid.x0[i] +=l_grid.dx0[i]; l_grid.x1[i] += l_grid.dx1[i];
            }
        }
    Pochoir_fn_end

    Pochoir_fn_1D(heat_1D_unsafe_fn, t, i)
        c(t+1, i) = 0.125 * (c(t, i-1) - 2.0 * c(t, i) + c(t, i+1)) + c(t, i);
    Pochoir_fn_end

    /* we have to bind arrayInUse and Shape together 
     * => One arrayInUse, one shape[] => One slope[]
     * because each arrayInUse needs to know the slope to determine
     * the boundary region and when to call the user supplied boundary
     * value function
     */
    heat_2D.registerArrayInUse(a, 5, heat_shape_2D, I, K);
    heat_1D.registerArrayInUse(c, 3, heat_shape_1D, J);

	gettimeofday(&start, 0);
    heat_1D.run_op(T_SIZE, heat_1D_safe_fn, heat_1D_unsafe_fn);
    heat_2D.run_op(T_SIZE, heat_2D_safe_fn, heat_2D_unsafe_fn);
	gettimeofday(&end, 0);
	std::cout << "Pochoir ET: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

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

#if SIMPLE
    /* this is non-periodic boundary value */
    Pochoir_BV_1D(heat_bv_1D, arr, t, i)
        int new_i = i;
        if (i <= 0)
            new_i = 0;
        else if (i >= N_SIZE-1)
            new_i = N_SIZE-1;
        arr.set(t, new_i) = 0;
        return arr.get(t, new_i);
    Pochoir_BV_end
#else
    /* this is periodic boundary value */
    Pochoir_BV_1D(heat_bv_1D, arr, t, i)
        int new_i = i;
        while (new_i < 0)
            new_i += N_SIZE;
        while (new_i >= N_SIZE)
            new_i -= N_SIZE;
        return arr.get(t, new_i);
    Pochoir_BV_end
#endif

    Pochoir_BV_2D(heat_bv_2D, arr, t, i, j)
        int new_i = i, new_j = j;
#if 1
        /* this is non-periodic boundary value */
        /* we already shrinked by using range I, J, K,
         * so the following code to set boundary index and
         * boundary rvalue is not necessary!!! 
         */
        if (i <= 0) 
            new_i = 0;
        else if (i >= N_SIZE-1) 
            new_i = N_SIZE-1;
        if (j <= 0)
            new_j = 0;
        else if (j >= N_SIZE-1)
            new_j = N_SIZE-1;
        arr.set(t, new_i, new_j) = 0;
#endif
        return arr.get(t, new_i, new_j);
    Pochoir_fn_end


