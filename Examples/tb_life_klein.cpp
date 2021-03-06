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

/* Test bench - 2D Game of Life, on Klein Bottle */
#include <cstdio>
#include <cstddef>
#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <cmath>

#include <pochoir.hpp>

using namespace std;
/* N_RANK includes both time and space dimensions */
#define N_RANK 2

void check_result(int t, int j, int i, bool a, bool b)
{
	if (a == b) {
//		printf("a(%d, %d, %d) == b(%d, %d, %d) == %s : passed!\n", t, j, i, t, j, i, a ? "True" : "False");
	} else {
		printf("a(%d, %d, %d) = %s, b(%d, %d, %d) = %s : FAILED!\n", t, j, i, a ? "True" : "False", t, j, i, b ? "True" : "False");
	}

}

    Pochoir_Boundary_2D(klein_bottle_2D, arr, t, i, j)
        /* this is non-periodic boundary value */
        /* we already shrinked by using range I, J, K,
         * so the following code to set boundary index and
         * boundary rvalue is not necessary!!! 
         */
        int new_i = i, new_j = j;
        const int l_arr_size_1 = arr.size(1), l_arr_size_0 = arr.size(0);
        if (new_i < 0) 
            new_i += l_arr_size_1;
        else if (new_i >= l_arr_size_1)
            new_i -= l_arr_size_1;
        if (new_j < 0) {
            new_j += l_arr_size_0;
            new_i = l_arr_size_1 - 1 - new_i;
        } else if (new_j >= l_arr_size_0) {
            new_j -= l_arr_size_0;
            new_i = l_arr_size_1 - 1 - new_i;
        }
//        if (new_i != i || new_j != j)
//            printf("a(%d, %d) -> a(%d, %d)\n", i, j, new_i, new_j);
        return arr.get(t, new_i, new_j);
    Pochoir_Boundary_end

    template <typename T>
    T & klein_bottle(Pochoir_Array<T, 2> & arr, int t, int i, int j) {
        /* this is non-periodic boundary value */
        /* we already shrinked by using range I, J, K,
         * so the following code to set boundary index and
         * boundary rvalue is not necessary!!! 
         */
        int new_i = i, new_j = j;
        const int l_arr_size_1 = arr.size(1), l_arr_size_0 = arr.size(0);
        if (new_i < 0) 
            new_i += l_arr_size_1;
        else if (new_i >= l_arr_size_1)
            new_i -= l_arr_size_1;
        if (new_j < 0) {
            new_j += l_arr_size_0;
            new_i = l_arr_size_1 - 1 - new_i;
        } else if (new_j >= l_arr_size_0) {
            new_j -= l_arr_size_0;
            new_i = l_arr_size_1 - 1 - new_i;
        }
//        if (new_i != i || new_j != j)
//            printf("b(%d, %d) -> b(%d, %d)\n", i, j, new_i, new_j);
        return arr.get(t, new_i, new_j);
    }


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
	Pochoir_Array<bool, N_RANK> a(N_SIZE, N_SIZE), b(N_SIZE, N_SIZE);
    Pochoir <bool, N_RANK> life_2D;
	// Pochoir_Domain I(0, N_SIZE), J(0, N_SIZE);
    Pochoir_Shape<2> life_shape_2D[9] = {{1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, 1, 1}, {0, -1, -1}, {0, 1, -1}, {0, -1, 1}};

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
    int neighbors = a(t, i-1, j-1) + a(t, i-1, j) + a(t, i-1, j+1) +
                    a(t, i, j-1)                  + a(t, i, j+1) +
                    a(t, i+1, j-1) + a(t, i+1, j) + a(t, i+1, j+1);
    if (a(t, i, j) == true && neighbors < 2)
        a(t+1, i, j) = false;
    else if (a(t, i, j) == true && neighbors > 3) { 
        a(t+1, i, j) = false;
    } else if (a(t, i, j) == true && (neighbors == 2 || neighbors == 3)) {
        a(t+1, i, j) = a(t, i, j);
    } else if (a(t, i, j) == false && neighbors == 3) 
        a(t+1, i, j) = true;
//    printf("a(%d, %d, %d)[%d] -> a(%d+1, %d, %d)[%d] = a(%d, %d, %d)[%d] + a(%d, %d, %d)[%d] + a(%d, %d, %d)[%d] + a(%d, %d, %d)[%d] + a(%d, %d, %d)[%d] + a(%d, %d, %d)[%d] + a(%d, %d, %d)[%d] + a(%d, %d, %d)[%d]\n\n", t, i, j, (bool)a(t, i, j), t, i, j, (bool)a(t+1, i, j), t, i-1, j-1, (bool)a(t, i-1, j-1), t, i-1, j, (bool)a(t, i-1, j), t, i-1, j+1,(bool)a(t, i-1, j+1), t, i, j-1, (bool)a(t, i, j-1), t, i, j+1, (bool)a(t, i, j+1), t, i+1, j-1, (bool)a(t, i+1, j-1), t, i+1, j, (bool)a(t, i+1, j), t, i+1, j+1, (bool)a(t, i+1, j+1));
    Pochoir_kernel_end

    life_2D.registerBoundaryFn(a, klein_bottle_2D);
//    life_2D.registerArray(a);
    life_2D.registerShape(life_shape_2D);
//    life_2D.registerDomain(I, J);

	gettimeofday(&start, 0);
    life_2D.run(T_SIZE, life_2D_fn);
	gettimeofday(&end, 0);
	std::cout << "Pochoir ET: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

	gettimeofday(&start, 0);
    // we can handle the boundary condition either by register a boundary function
    // b.registerBV(klein_bottle_2D);
    // because the boundary function declared for Pochoir is just a comman c++ function,
    // so let's directly call it here to simplify the implementation!
	for (int t = 0; t < T_SIZE; ++t) {
	cilk_for (int i = 0; i < N_SIZE; ++i) {
	for (int j = 0; j < N_SIZE; ++j) {
	int neighbors = klein_bottle(b, t, i-1, j-1) + klein_bottle(b, t, i-1, j) +
                    klein_bottle(b, t, i-1, j+1) + klein_bottle(b, t, i, j-1) +
                    klein_bottle(b, t, i, j+1)   + klein_bottle(b, t, i+1, j-1) +
                    klein_bottle(b, t, i+1, j)   + klein_bottle(b, t, i+1, j+1);
	if (b.interior(t, i, j) == true && neighbors < 2)
	    b.interior(t+1, i, j) = false;
	else if (b.interior(t, i, j) == true && neighbors > 3)
	    b.interior(t+1, i, j) = false;
	else if (b.interior(t, i, j) == true && (neighbors == 2 || neighbors == 3))
	    b.interior(t+1, i, j) = b.interior(t, i, j);
	else if (b.interior(t, i, j) == false && neighbors == 3)
	    b.interior(t+1, i, j) = true;
//    printf("b(%d, %d, %d)[%d] -> b(%d+1, %d, %d)[%d] = b(%d, %d, %d)[%d] + b(%d, %d, %d)[%d] + b(%d, %d, %d)[%d] + b(%d, %d, %d)[%d] + b(%d, %d, %d)[%d] + b(%d, %d, %d)[%d] + b(%d, %d, %d)[%d] + b(%d, %d, %d)[%d]\n\n", 
//            t, i, j, klein_bottle(b, t, i, j), t, i, j, klein_bottle(b, t+1, i, j), t, i-1, j-1, klein_bottle(b, t, i-1, j-1), t, i-1, j, klein_bottle(b, t, i-1, j), t, i-1, j+1,klein_bottle(b, t, i-1, j+1), t, i, j-1, klein_bottle(b, t, i, j-1), t, i, j+1, klein_bottle(b, t, i, j+1), t, i+1, j-1, klein_bottle(b, t, i+1, j-1), t, i+1, j, klein_bottle(b, t, i+1, j), t, i+1, j+1, klein_bottle(b, t, i+1, j+1));
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
