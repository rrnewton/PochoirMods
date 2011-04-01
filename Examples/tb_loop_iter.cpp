/*
 **********************************************************************************
 *  Copyright (C) 2010  Massachusetts Institute of Technology
 *  Copyright (C) 2010  Yuan Tang <yuantang@csail.mit.edu>
 *                      Charles E. Leiserson <cel@mit.edu>
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <iostream>

#define TIMES 1
#define TOLERANCE (1e-6)

using namespace std;

#define ref(_arr, _t, _i, _j) \
    _arr[((_t)&0x1) * total_size + ((_i) * N_SIZE + (_j))]

#define addr(_arr, _t, _i, _j) \
    (_arr + (((_t)&0x1) * total_size + ((_i) * N_SIZE + (_j))))

#define idx(_t, _i, _j) \
    (((_t)&0x1) * total_size + ((_i) * N_SIZE + (_j)))

#define INF 100000000

#define Adjust(_begin_i, _end_i, _begin_j, _end_j) \
    do { \
        _begin_i += 1; _end_i -= 1; \
        _begin_j += 1; _end_j -= 1; \
    } while(0)

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

static inline double tdiff (struct timeval *a, struct timeval *b)
{
	    return a->tv_sec - b->tv_sec + 1e-6 * (a->tv_usec - b->tv_usec);
}

int StrToInt(const std::string& s)
{
  return atoi(s.c_str());
}

void check_result(int t, int i, int j, double a, double b)
{
    if (abs(a - b) < TOLERANCE) {
//      printf("a(%d, %d, %d) == b(%d, %d, %d) == %f : passed!\n", t, i, j, t, i, j, a);
    } else {
        printf("a(%d, %d, %d) = %f, b(%d, %d, %d) = %f : FAILED!\n", t, i, j, a, t, i, j, b);
    }

}

#define ARRAY_LENGTH(x) (int)(sizeof(x)/sizeof(x[0]))

int main(int argc, char * argv[])
{
    const int BASE = 1024;
    struct timeval start, end;
    double min_tdiff;
    int N_SIZE = 0, T_SIZE = 0;

    if (argc < 3) {
        printf("argc < 3, quit! \n");
        exit(1);
    }
    N_SIZE = StrToInt(argv[1]);
    T_SIZE = StrToInt(argv[2]);
    printf("N_SIZE = %d, T_SIZE = %d\n", N_SIZE, T_SIZE);

    //double a[2*N_SIZE*N_SIZE], b[2*N_SIZE*N_SIZE];
    double * a = new double[2*N_SIZE*N_SIZE];
    // double a[2*N_SIZE*N_SIZE];
    double * b = new double[2*N_SIZE*N_SIZE];
//    double la[2*100*100];

    const int total_size = N_SIZE * N_SIZE;

    /* initialization */
    for (int i = 0; i < N_SIZE; ++i) {
        for (int j = 0; j < N_SIZE; ++j) {
            if (i == 0 || i == N_SIZE-1
                || j == 0 || j == N_SIZE-1) {
//                printf("addr(%d, %d, %d) = %d, addr(%d, %d, %d) = %d\n", 0, i, j, addr(0, i, j), 1, i, j, addr(1, i, j));
                ref(a, 0, i, j) = ref(a, 1, i, j) = 0;
            } else {
#if DEBUG
                ref(a, 0, i, j) = i * N_SIZE + j;
                ref(a, 1, i, j) = 0;
#else
                ref(a, 0, i, j) = 1.0 * (rand() % BASE);
                ref(a, 1, i, j) = 0;
#endif
            }
            ref(b, 0, i, j) = ref(a, 0, i, j);
            ref(b, 1, i, j) = 0;
        }
    }

    const int odd = (T_SIZE&0x1);
    const int even = (T_SIZE&~0x1);
    int begin_i = 1, end_i = N_SIZE-1;
    int begin_j = 1, end_j = N_SIZE-1;
    int t, i, j;
    min_tdiff = INF;
    double *iter0, *iter1, *iter2, *iter3, *iter4, *iter5;
    int gap;
    const int stride_1 = N_SIZE, stride_0 = 1;
    int ia;

    for (int times = 0; times < TIMES; ++times) {
        gettimeofday(&start, 0);
    for (t = 0; t < even; ++t) {
        ia = idx(t, begin_i, begin_j);
        iter0 = addr(a, t+1, begin_i, begin_j);
        iter1 = addr(a, t, begin_i+1, begin_j);
        iter2 = addr(a, t, begin_i, begin_j);
        iter3 = addr(a, t, begin_i-1, begin_j);
        iter4 = addr(a, t, begin_i, begin_j+1);
        iter5 = addr(a, t, begin_i, begin_j-1);
        gap = stride_1 + (begin_j - end_j) * stride_0;
    for (i = begin_i; i < end_i; ++i,
            ia += gap,
            iter0 += gap,
            iter1 += gap,
            iter2 += gap,
            iter3 += gap,
            iter4 += gap,
            iter5 += gap) {
        for (j = begin_j; j < end_j; ++j,
            ++ia,
            ++iter0,
            ++iter1,
            ++iter2,
            ++iter3,
            ++iter4,
            ++iter5) {
            //ref(a, t+1, i, j) = 0.125 * (ref(a, t, i+1, j) - 2.0 * ref(a, t, i, j) + ref(a, t, i-1, j)) + 0.125 * (ref(a, t, i, j+1) - 2.0 * ref(a, t, i, j) + ref(a, t, i, j-1)) + ref(a, t, i, j);
            (*iter0) = 0.125 * ((*iter1) - 2.0 * (*iter2) + (*iter3)) + 0.125 * ((*iter4) - 2.0 * (*iter2) + (*iter5)) + (*iter2);
#if DEBUG
        if (ia != idx(t, i, j))
            printf("0: iter(%d, %d, %d) = %d, idx(%d, %d, %d) = %d\n", t, i, j, ia, t, i, j, idx(t, i, j)); 
#endif
        } /* end for j */
    } /* end for i */
    ++t; 
    Adjust(begin_i, end_i, begin_j, end_j);
    ia = idx(t, end_i-1, end_j-1);
    iter0 = addr(a, t+1, end_i-1, end_j-1);
    iter1 = addr(a, t, end_i-1+1, end_j-1);
    iter2 = addr(a, t, end_i-1, end_j-1);
    iter3 = addr(a, t, end_i-1-1, end_j-1);
    iter4 = addr(a, t, end_i-1, end_j-1+1);
    iter5 = addr(a, t, end_i-1, end_j-1-1);
    gap = -stride_1 + (end_j - begin_j) * stride_0;
    for (i = end_i-1; i >= begin_i; --i,
            ia += gap,
            iter0 += gap,
            iter1 += gap,
            iter2 += gap,
            iter3 += gap,
            iter4 += gap,
            iter5 += gap) {
        for (j = end_j-1; j >= begin_j; --j,
            --ia, --iter0, --iter1, --iter2, --iter3, --iter4, --iter5) {
            // ref(a, t+1, i, j) = ref(a, t, i, j) + 0.1;
            (*iter0) = 0.125 * ((*iter1) - 2.0 * (*iter2) + (*iter3)) + 0.125 * ((*iter4) - 2.0 * (*iter2) + (*iter5)) + (*iter2);
            // ref(a, t+1, i, j) = 0.125 * (ref(a, t, i+1, j) - 2.0 * ref(a, t, i, j) + ref(a, t, i-1, j)) + 0.125 * (ref(a, t, i, j+1) - 2.0 * ref(a, t, i, j) + ref(a, t, i, j-1)) + ref(a, t, i, j);
#if DEBUG
            if (ia != idx(t, i, j))
                printf("1: iter(%d, %d, %d) = %d, idx(%d, %d, %d) = %d\n", t, i, j, ia, t, i, j, idx(t, i, j)); 
#endif
        } /* end for j */
    } /* end for i */
    Adjust(begin_i, end_i, begin_j, end_j);
    } /* end for t */
    if (odd) {
        ia = idx(t, begin_i, begin_j);
        iter0 = addr(a, t+1, begin_i, begin_j);
        iter1 = addr(a, t, begin_i+1, begin_j);
        iter2 = addr(a, t, begin_i, begin_j);
        iter3 = addr(a, t, begin_i-1, begin_j);
        iter4 = addr(a, t, begin_i, begin_j+1);
        iter5 = addr(a, t, begin_i, begin_j-1);
        gap = stride_1 + (begin_j - end_j) * stride_0;
        for (i = begin_i; i < end_i; ++i,
                ia += gap,
                iter0 += gap, 
                iter1 += gap, 
                iter2 += gap, 
                iter3 += gap, 
                iter4 += gap, 
                iter5+= gap) {
            for (j = begin_j; j < end_j; ++j,
                ++ia, ++iter0, ++iter1, ++iter2, ++iter3, ++iter4, ++iter5) {
               (*iter0) = 0.125 * ((*iter1) - 2.0 * (*iter2) + (*iter3)) + 0.125 * ((*iter4) - 2.0 * (*iter2) + (*iter5)) + (*iter2);
            // ref(a, t+1, i, j) = 0.125 * (ref(a, t, i+1, j) - 2.0 * ref(a, t, i, j) + ref(a, t, i-1, j)) + 0.125 * (ref(a, t, i, j+1) - 2.0 * ref(a, t, i, j) + ref(a, t, i, j-1)) + ref(a, t, i, j);
        // ref(a, t+1, i, j) = ref(a, t, i, j) + 0.1;
#if DEBUG
            if (ia != idx(t, i, j))
                printf("2: iter(%d, %d, %d) = %d, idx(%d, %d, %d) = %d\n", t, i, j, ia, t, i, j, idx(t, i, j)); 
#endif
            } }
    }
        gettimeofday(&end, 0);
        min_tdiff = min(min_tdiff, (1.0e3 * tdiff(&end, &start)));        
    }
    std::cout << "space filling curve consumed time: " << min_tdiff << " ms" << std::endl;

    begin_i = 1; end_i = N_SIZE-1;
    begin_j = 1; end_j = N_SIZE-1;
    min_tdiff = INF;
    for (int times = 0; times < TIMES; ++times) {
        gettimeofday(&start, 0);
    for (t = 0; t < T_SIZE; ++t) {
        for (i = begin_i; i < end_i; ++i) {
    for (j = begin_j; j < end_j; ++j) {
        ref(b, t+1, i, j) = 0.125 * (ref(b, t, i+1, j) - 2.0 * ref(b, t, i, j) + ref(b, t, i-1, j)) + 0.125 * (ref(b, t, i, j+1) - 2.0 * ref(b, t, i, j) + ref(b, t, i, j-1)) + ref(b, t, i, j);
        // ref(b, t+1, i, j) = ref(b, t, i, j) + 0.1;
        //printf("ref(b, %d, %d, %d) = %d\t", t, i, j, addr(t, i, j)); 
    } } /* end for i */ 
        Adjust(begin_i, end_i, begin_j, end_j);
    } /* end for t */
        gettimeofday(&end, 0);
        min_tdiff = min(min_tdiff, (1.0e3 * tdiff(&end, &start)));        
    }
    std::cout << "naive loop consumed time: " << min_tdiff << " ms" << std::endl;

    t = T_SIZE;
    for (i = 1; i < N_SIZE-1; ++i) {
        for (j = 1; j < N_SIZE-1; ++j) {
            check_result(t, i, j, ref(a, t, i, j), ref(b, t, i, j));
        }
    }
    return 0;
}
