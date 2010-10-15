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

#include "expr_array.hpp"

using namespace std;
#define SIMPLE 1
/* N_RANK includes both time and space dimensions */
#define N_RANK 3
#define N_SIZE 555
#define T_SIZE 505
#define TOLERANCE (1e-6)

void check_result(int t, int j, int i, bool a, bool b)
{
	if (a == b) {
//		printf("a(%d, %d, %d) == b(%d, %d, %d) == %s : passed!\n", t, j, i, t, j, i, a ? "True" : "False");
	} else {
		printf("a(%d, %d, %d) = %s, b(%d, %d, %d) = %s : FAILED!\n", t, j, i, a ? "True" : "False", t, j, i, b ? "True" : "False");
	}

}

int main(void)
{
	int t;
	struct timeval start, end;
	/* data structure of Pochoir - row major */
	SArray<bool, N_RANK> a(T_SIZE, N_SIZE, N_SIZE), b(T_SIZE, N_SIZE, N_SIZE), c(T_SIZE, N_SIZE, N_SIZE), d(T_SIZE, N_SIZE, N_SIZE);
	uRange I(1, N_SIZE-2), J(1, N_SIZE-2), T(0, T_SIZE), K(0, N_SIZE-1);
    /* User has to supply the slope[] of each space dimension manually,
     * or rely on our preprocessor to deduce it
     */
    size_t slope[3] = {1, 1, 1};

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
        c(0, i, j) = a(0, i, j);
        c(1, i, j) = 0;
	} }

	cout << endl << "\na(T+1, J, I) = 0.01 + a(T, J, I)" << endl;

	gettimeofday(&start, 0);
#if 0
    pochoir(T, I, J, slope, [&a](int t, int i, int j){a(t+1, i, j) = 0.01 + a(t, i-1, j+1);});
	pochoir(T, I, slope, [&c](int t, int i){ 
//                            printf("C_internal: c(%d, %d) = 0.01 + c(%d, %d)\n", t+1, i, t, i-1);
                            c(t+1, i) = 0.01 + c(t, i-1);  }, 
                         [&c](int t, int i){ 
                            int new_i = (i-1+N_SIZE-2) % (N_SIZE-2) + 1;
                            int old_i = (i-2+N_SIZE-2) % (N_SIZE-2) + 1;
  //                          printf("C_boundary: c(%d, %d) = 0.01 + c(%d, %d)\n", t+1, i, t, l_i);
                            c(t+1, new_i) = 0.01 + c(t, old_i); });
    pochoir(T, I, J, slope, 
            [&a](size_t t, size_t i, size_t j){
            int neighbors = a(t, i-1, j-1) + a(t, i-1, j) + a(t, i-1, j+1) +
                            a(t, i, j-1)                  + a(t, i, j+1) +
                            a(t, i+1, j-1) + a(t, i+1, j) + a(t, i+1, j+1);
            if (a(t, i, j) == true && neighbors < 2)
                a(t+1, i, j) = true;
            else if (a(t, i, j) == true && neighbors > 3)
                a(t+1, i, j) = false;
            else if (a(t, i, j) == true && (neighbors == 2 || neighbors == 3))
                a(t+1, i, j) = a(t, i, j);
            else if (a(t, i, j) == 0 && neighbors == 3)
                a(t+1, i, j) = true;
            });
#else
    Pochoir_begin(Non-periodic);
    int neighbors = c(T, I-1, J-1) + c(T, I-1, J) + c(T, I-1, J+1) +
                    c(T, I, J-1)                  + c(T, I, J+1) +
                    c(T, I+1, J-1) + c(T, I+1, J) + c(T, I+1, J+1);
    if (c(T, I, J) == true && neighbors < 2)
        c(T+1, I, J) = true;
    else if (c(T, I, J) == true && neighbors > 3) {
        c(T+1, I, J) = false;
    } else if (c(T, I, J) == true && (neighbors == 2 || neighbors == 3)) {
        c(T+1, I, J) = c(T, I, J);
    } else if (c(T, I, J) == false && neighbors == 3) 
        c(T+1, I, J) = true;
    Pochoir_end();

    Pochoir_begin(Periodic);
    int neighbors = d(T, I-1, J-1) + d(T, I-1, J) + d(T, I-1, J+1) +
                    d(T, I, J-1)                  + d(T, I, J+1) +
                    d(T, I+1, J-1) + d(T, I+1, J) + d(T, I+1, J+1);
    switch (neighbors) {
        case 2:
            d(T+1, I, J) = true;
            break;
        case 3:
            d(T+1, I, J) = false;
            break;
        case 4: 
            d(T+1, I, J) = true;
            break;
        default:
            d(T+1, I, J) = false;
            break;
    }
    Pochoir_end();
    
    Pochoir_begin(Periodic);
    int neighbors = d(T, I-1, J-1) + d(T, I-1, J) + d(T, I-1, J+1) +
                    d(T, I, J-1)                  + d(T, I, J+1) +
                    d(T, I+1, J-1) + d(T, I+1, J) + d(T, I+1, J+1);
    for (int k = 0, int p = 1; k < neighbors, p > neighbors + k; k++, --p){
        d(T+1, I, J) = true;
        d(T+1, I, J) = true;
        d(T+1, I, J) = false;
    } 
    Pochoir_end();
#endif
	gettimeofday(&end, 0);
	std::cout << "Pochoir ET: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

	gettimeofday(&start, 0);
	for (int t = 0; t <= T_SIZE; ++t) {
	for (int i = 1; i <= N_SIZE-2; ++i) {
	for (int j = 1; j <= N_SIZE-2; ++j) {
#if 0
        /* Non-periodic version */
        int neighbors = b(t, i-1, j-1) + b(t, i-1, j) + b(t, i-1, j+1) +
                        b(t, i, j-1)                  + b(t, i, j+1) +
                        b(t, i+1, j-1) + b(t, i+1, j) + b(t, i+1, j+1);
        if (b(t, i, j) == true && neighbors < 2)
            b(t+1, i, j) = true;
        else if (b(t, i, j) == true && neighbors > 3)
            b(t+1, i, j) = false;
        else if (b(t, i, j) == true && (neighbors == 2 || neighbors == 3))
            b(t+1, i, j) = b(t, i, j);
        else if (b(t, i, j) == false && neighbors == 3)
            b(t+1, i, j) = true;
#else
        /* Periodic version */
	size_t idx0 = (i - 1 + 553 - 1) % 553 + 1;
	size_t idx1 = (j - 1 + 553 - 1) % 553 + 1;
	size_t idx2 = (j - 1 + 553) % 553 + 1;
	size_t idx3 = (j - 1 + 553 + 1) % 553 + 1;
	size_t idx4 = (i - 1 + 553) % 553 + 1;
	size_t idx5 = (i - 1 + 553 + 1) % 553 + 1;
	int neighbors = b(t, idx0, idx1) + b(t, idx0, idx2) + b(t, idx0, idx3) + b(t, idx4, idx1) + b(t, idx4, idx3) + b(t, idx5, idx1) + b(t, idx5, idx2) + b(t, idx5, idx3);
	if (b(t, idx4, idx2) == true && neighbors < 2)
	b(t + 1, idx4, idx2) = true;
	else if (b(t, idx4, idx2) == true && neighbors > 3)
	b(t + 1, idx4, idx2) = false;
	else if (b(t, idx4, idx2) == true && (neighbors == 2 || neighbors == 3))
	b(t + 1, idx4, idx2) = b(t, idx4, idx2);
	else if (b(t, idx4, idx2) == false && neighbors == 3)
	b(t + 1, idx4, idx2) = true;
#endif
	} } }
	gettimeofday(&end, 0);
	std::cout << "Naive Loop: consumed time :" << 1.0e3 * tdiff(&end, &start) << "ms" << std::endl;

	t = T_SIZE+1;
	for (int i = 1; i <= N_SIZE-2; ++i) {
	for (int j = 1; j <= N_SIZE-2; ++j) {
//		check_result(t, i, j, a(t, i, j), b(t, i, j));
		check_result(t, i, j, b(t, i, j), c(t, i, j));
	} } 

#if 0 
//	cout << "I = " << I << endl;
//	cout << "I+1 = " << I+1 << endl;
//	cout << "I-1 = " << I-1 << endl;
	cout << "a = " << a << endl;
	cout << "b = " << b << endl;
//	cout << "c = " << c << endl;
//	cout << "P1 = " << P1 << endl;
//	cout << "P2 = " << P2 << endl;
//	cout << "d = " << d << endl;
#endif
	return 0;
}
