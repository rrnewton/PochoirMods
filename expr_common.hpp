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

#ifndef EXPR_COMMON_H
#define EXPR_COMMON_H

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

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
/* a bit tricky version of modulo operation, assuming a < 2 * b */
#define pmod(a, b) ((a) - ((b) & -((a)>=(b))))
#define pmod_lu(a, lb, ub) ((a) - (((ub)-(lb)) & -((a)>=(ub))))

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
struct Pochoir_Shape_info {
    /* N_RANK + 1 because we probably have to include the time dimension
     * to correctly calculate the slope[]
     */
    int shift[N_RANK+1];
};
 
template <int N_RANK, size_t N>
size_t ArraySize (Pochoir_Shape_info<N_RANK> (& arr)[N]) { return N; }

/* these lambda functions are for computing internal/boundary region,
 * the original 'f'/'bf'
 */
#define Pochoir_kernel_1D(name, t, i) \
    auto name = [&](int t, int i) { 

#define Pochoir_kernel_2D(name, t, i, j) \
    auto name = [&](int t, int i, int j) {

#define Pochoir_kernel_3D(name, t, i, j, k) \
    auto name = [&](int t, int i, int j, int k) {

#define Pochoir_kernel_end }; 

#define Pochoir_obase_fn_1D(name, t0, t1, grid) \
    auto name = [&](int t0, int t1, grid_info<1> const & grid) {

#define Pochoir_obase_fn_2D(name, t0, t1, grid) \
    auto name = [&](int t0, int t1, grid_info<2> const & grid) {

#define Pochoir_obase_fn_3D(name, t0, t1, grid) \
    auto name = [&](int t0, int t1, grid_info<3> const & grid) {

/* - these function templates are for computing boundary values, currently
 *   icc doesn't support capturing the lambda function by function objects,
 *   so, we have to utilize the function pointers!
 * - because these functions will be called inside T & operator() functions,
 *   so we have to return a value of T&
 */
#if 1
#define Pochoir_Boundary_Declare_1D(name, arr, t, i) \
    template <typename T> \
    T name (Pochoir_Array<T, 1> & arr, int t, int i) 

#define Pochoir_Boundary_Declare_2D(name, arr, t, i, j) \
    template <typename T> \
    T name (Pochoir_Array<T, 2> & arr, int t, int i, int j)

#define Pochoir_Boundary_Declare_3D(name, arr, t, i, j, k) \
    template <typename T> \
    T name (Pochoir_Array<T, 3> & arr, int t, int i, int j, int k)
#endif

#define Pochoir_Boundary_1D(name, arr, t, i) \
    template <typename T> \
    T name (Pochoir_Array<T, 1> & arr, int t, int i) { 

#define Pochoir_Boundary_2D(name, arr, t, i, j) \
    template <typename T> \
    T name (Pochoir_Array<T, 2> & arr, int t, int i, int j) { 

#define Pochoir_Boundary_3D(name, arr, t, i, j, k) \
    template <typename T> \
    T name (Pochoir_Array<T, 3> & arr, int t, int i, int j, int k) { 

#define Pochoir_Boundary_end }

#endif /* EXPR_COMMON_H */
