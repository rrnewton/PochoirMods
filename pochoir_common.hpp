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

#ifndef POCHOIR_COMMON_H
#define POCHOIR_COMMON_H

#include <sys/time.h>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <string>

#if 0
#define cilk_spawn 
#define cilk_sync
#define cilk_for for
#endif

static inline double tdiff (struct timeval *a, struct timeval *b)
{
	    return a->tv_sec - b->tv_sec + 1e-6 * (a->tv_usec - b->tv_usec);
}

int StrToInt(const std::string& s)
{
  return std::atoi(s.c_str());
}

#define ARRAY_LENGTH(x) (int)(sizeof(x)/sizeof(x[0]))

/* due to the fact that bit trick is much slower than conditional instruction,
 * let's disable it for now!!!
 */
#define BIT_TRICK 0
#define INF 100000000
#define SUPPORT_RANK 9

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
/* a bit tricky version of modulo operation, assuming a < 2 * b */
#define pmod(a, b) ((a) - ((b) & -((a)>=(b))))
#define pmod_lu(a, lb, ub) ((a) - (((ub)-(lb)) & -((a)>=(ub))))

#define pCond(b, x, y) (x&(-b)) | (y&-(!b))

inline bool select(bool b, bool x, bool y) {
    return (x&(-b)) | (y&-(!b));
}
inline int select(bool b, int x, int y) {
    return (x&(-b)) | (y&-(!b));
}
inline float select(bool b, float x, float y) {
    int __ir__ = ((*(int*)&x) & (-b)) | ((*(int*)&y) & -(!b)); 
    return *(float*)&__ir__; 
}   
inline double select(bool b, double x, double y) {
    long __ir__ = ((*(long*)&x) & (-b)) | ((*(long*)&y) & -(!b));
    return *(double*)&__ir__;
}

typedef int T_dim;
typedef int T_index;

template <int N_RANK>
struct grid_info {
    int x0[N_RANK], x1[N_RANK];
    int dx0[N_RANK], dx1[N_RANK];
};

template <int N_RANK>
struct Pochoir_Shape {
    /* N_RANK + 1 because we probably have to include the time dimension
     * to correctly calculate the slope[]
     */
    int shift[N_RANK+1];
};
 
template <int N_RANK, size_t N>
size_t ArraySize (Pochoir_Shape<N_RANK> (& arr)[N]) { return N; }

#define KLEIN 0
#define USE_CILK_FOR 0
#define BICUT 1
#define STAT 0

inline void klein(int & new_i, int & new_j, grid_info<2> const & grid) {
    int l_arr_size_1 = grid.x1[1] - grid.x0[1];
    int l_arr_size_0 = grid.x1[0] - grid.x0[0];

    if (new_i < grid.x0[1])
        new_i += l_arr_size_1;
    else if (new_i >= grid.x1[1])
        new_i -= l_arr_size_1;
    if (new_j < grid.x0[0]) {
        new_j += l_arr_size_0;
        new_i  = grid.x0[1] + (grid.x1[1] - 1 - new_i);
    } else if (new_j >= grid.x1[0]) {
        new_j -= l_arr_size_0;
        new_i  = grid.x0[1] + (grid.x1[1] - 1 - new_i);
    }
    return;
}

inline void klein_region(grid_info<2> & grid, grid_info<2> const & initial_grid) {
    grid_info<2> orig_grid;
    const int l_arr_size_1 = initial_grid.x1[1] - initial_grid.x0[1];
    const int l_arr_size_0 = initial_grid.x1[0] - initial_grid.x0[0];

    if (grid.x0[1] >= initial_grid.x1[1]) {
        grid.x0[1] -= l_arr_size_1;
        grid.x1[1] -= l_arr_size_1;
    } else if (grid.x1[1] < initial_grid.x0[1]) {
        grid.x0[1] += l_arr_size_1;
        grid.x1[1] += l_arr_size_1;
    } 
    orig_grid = grid;
    if (grid.x0[0] >= initial_grid.x1[0]) {
        grid.x0[0] -= l_arr_size_0;
        grid.x1[0] -= l_arr_size_0;
        grid.x0[1] = initial_grid.x0[1] + (initial_grid.x1[1] - orig_grid.x1[1]);
        grid.x1[1] = initial_grid.x0[1] + (initial_grid.x1[1] - orig_grid.x0[1]);
        grid.dx0[1] = -orig_grid.dx1[1];
        grid.dx1[1] = -orig_grid.dx0[1];
    } else if (grid.x1[0] < initial_grid.x0[0]) {
        grid.x0[0] += l_arr_size_0;
        grid.x1[0] += l_arr_size_0;
        grid.x0[1] = initial_grid.x0[1] + (initial_grid.x1[1] - orig_grid.x1[1]);
        grid.x1[1] = initial_grid.x0[1] + (initial_grid.x1[1] - orig_grid.x0[1]);
        grid.dx0[1] = -orig_grid.dx1[1];
        grid.dx1[1] = -orig_grid.dx0[1];
    }
    return;
}

/* these lambda functions are for computing internal/boundary region,
 * the original 'f'/'bf'
 */
#define Pochoir_kernel_1D(name, t, i) \
    auto name = [&](int t, int i) { 

#define Pochoir_kernel_2D(name, t, i, j) \
    auto name = [&](int t, int i, int j) {

#define Pochoir_kernel_3D(name, t, i, j, k) \
    auto name = [&](int t, int i, int j, int k) {

#define Pochoir_kernel_4D(name, t, i, j, k, l) \
    auto name = [&](int t, int i, int j, int k, int l) {

#define Pochoir_kernel_5D(name, t, i, j, k, l, m) \
    auto name = [&](int t, int i, int j, int k, int l, int m) {

#define Pochoir_kernel_6D(name, t, i, j, k, l, m, n) \
    auto name = [&](int t, int i, int j, int k, int l, int m, int n) {

#define Pochoir_kernel_7D(name, t, i, j, k, l, m, n, o) \
    auto name = [&](int t, int i, int j, int k, int l, int m, int n, int o) {

#define Pochoir_kernel_8D(name, t, i, j, k, l, m, n, o, p) \
    auto name = [&](int t, int i, int j, int k, int l, int m, int n, int o, int p) {

#define Pochoir_kernel_end }; 

#define Pochoir_obase_fn_1D(name, t0, t1, grid) \
    auto name = [&](int t0, int t1, grid_info<1> const & grid) {

#define Pochoir_obase_fn_2D(name, t0, t1, grid) \
    auto name = [&](int t0, int t1, grid_info<2> const & grid) {

#define Pochoir_obase_fn_3D(name, t0, t1, grid) \
    auto name = [&](int t0, int t1, grid_info<3> const & grid) {

#define Pochoir_obase_fn_4D(name, t0, t1, grid) \
    auto name = [&](int t0, int t1, grid_info<4> const & grid) {

#define Pochoir_obase_fn_5D(name, t0, t1, grid) \
    auto name = [&](int t0, int t1, grid_info<5> const & grid) {

#define Pochoir_obase_fn_6D(name, t0, t1, grid) \
    auto name = [&](int t0, int t1, grid_info<6> const & grid) {

#define Pochoir_obase_fn_7D(name, t0, t1, grid) \
    auto name = [&](int t0, int t1, grid_info<7> const & grid) {

#define Pochoir_obase_fn_8D(name, t0, t1, grid) \
    auto name = [&](int t0, int t1, grid_info<8> const & grid) {

/* - these function templates are for computing boundary values, currently
 *   icc doesn't support capturing the lambda function by function objects,
 *   so, we have to utilize the function pointers!
 * - because these functions will be called inside T & operator() functions,
 *   so we have to return a value of T&
 */
#define Pochoir_Boundary_1D(name, arr, t, i) \
    template <typename T, int TOGGLE> \
    T name (Pochoir_Array<T, 1, TOGGLE> & arr, int t, int i) { 

#define Pochoir_Boundary_2D(name, arr, t, i, j) \
    template <typename T, int TOGGLE> \
    T name (Pochoir_Array<T, 2, TOGGLE> & arr, int t, int i, int j) { 

#define Pochoir_Boundary_3D(name, arr, t, i, j, k) \
    template <typename T, int TOGGLE> \
    T name (Pochoir_Array<T, 3, TOGGLE> & arr, int t, int i, int j, int k) { 

#define Pochoir_Boundary_4D(name, arr, t, i, j, k, l) \
    template <typename T, int TOGGLE> \
    T name (Pochoir_Array<T, 4, TOGGLE> & arr, int t, int i, int j, int k, int l) { 

#define Pochoir_Boundary_5D(name, arr, t, i, j, k, l, m) \
    template <typename T, int TOGGLE> \
    T name (Pochoir_Array<T, 5, TOGGLE> & arr, int t, int i, int j, int k, int l, int m) { 

#define Pochoir_Boundary_6D(name, arr, t, i, j, k, l, m, n) \
    template <typename T, int TOGGLE> \
    T name (Pochoir_Array<T, 6, TOGGLE> & arr, int t, int i, int j, int k, int l, int m, int n) { 

#define Pochoir_Boundary_7D(name, arr, t, i, j, k, l, m, n, o) \
    template <typename T, int TOGGLE> \
    T name (Pochoir_Array<T, 7, TOGGLE> & arr, int t, int i, int j, int k, int l, int m, int n, int o) { 

#define Pochoir_Boundary_8D(name, arr, t, i, j, k, l, m, n, o, p) \
    template <typename T, int TOGGLE> \
    T name (Pochoir_Array<T, 8, TOGGLE> & arr, int t, int i, int j, int k, int l, int m, int n, int o, int p) { 

#define Pochoir_Boundary_end }

#endif /* POCHOIR_COMMON_H */
