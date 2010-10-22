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

#ifndef EXPR_STENCIL_HPP
#define EXPR_STENCIL_HPP

#include "expr_common.hpp"
#include "expr_array.hpp"
#include "expr_iter.hpp"

/* assuming there won't be more than 10 Pochoir_Array in one Pochoir_Stencil object! */
#define ARRAY_SIZE 10
template <typename T, int N_RANK, int TOGGLE=2>
class Pochoir_Stencil {
    private:
        int slope_[N_RANK];
        grid_info<N_RANK> grid_;
        int stride_[N_RANK];
        int logic_size_[N_RANK];
        int timestep_;
        Pochoir_Array<T, N_RANK, TOGGLE> ** arr_list_;
        typedef T (*BValue_1D)(Pochoir_Array<T, 1> &, int, int);
        typedef T (*BValue_2D)(Pochoir_Array<T, 2> &, int, int, int);
        typedef T (*BValue_3D)(Pochoir_Array<T, 3> &, int, int, int, int);
        int arr_len_;
        int arr_idx_;
    public:
    Pochoir_Stencil() {
        for (int i = 0; i < N_RANK; ++i) {
            slope_[i] = 0;
            grid_.x0[i] = grid_.x1[i] = grid_.dx0[i] = grid_.dx1[i] = 0;
        }
        timestep_ = 0;
        arr_list_ = (Pochoir_Array<T, N_RANK, TOGGLE>**)calloc(ARRAY_SIZE, sizeof(Pochoir_Array<T, N_RANK, TOGGLE>*));
        arr_len_ = 0;
        arr_idx_ = 0;
    }
    /* currently, we just compute the slope[] out of the shape[] */
    /* We get the grid_info out of arrayInUse */
    void registerArrayInUse(Pochoir_Array<T, N_RANK, TOGGLE> & arr);
    template <size_t N_SIZE>
    void registerShape(Pochoir_Shape_info<N_RANK> (& shape)[N_SIZE]);
    /* register boundary value function with corresponding Pochoir_Array object directly */
    void registerBoundaryFn(Pochoir_Array<T, 1, TOGGLE> & arr, BValue_1D _bv1) {
        arr.registerBV(_bv1);
    } 
    void registerBoundaryFn(Pochoir_Array<T, 2, TOGGLE> & arr, BValue_2D _bv2) {
        arr.registerBV(_bv2);
    } 
    void registerBoundaryFn(Pochoir_Array<T, 3, TOGGLE> & arr, BValue_3D _bv3) {
        arr.registerBV(_bv3);
    } 
    template <typename Range>
    void registerDomain(Range const & i);
    template <typename Range>
    void registerDomain(Range const & i, Range const & j);
    template <typename Range>
    void registerDomain(Range const & i, Range const & j, Range const & k);

    /* Executable Spec */
    template <typename BF>
    void run(int timestep, BF const & bf);
    /* safe/unsafe Executable Spec */
    template <typename F, typename BF>
    void run(int timestep, F const & f, BF const & bf);
    /* obase for zero-padded region */
    template <typename F>
    void run_obase(int timestep, F const & f);
    /* obase for interior and ExecSpec for boundary */
    template <typename F, typename BF>
    void run_obase(int timestep, F const & f, BF const & bf);
};

template <typename T, int N_RANK, int TOGGLE>
void Pochoir_Stencil<T, N_RANK, TOGGLE>::registerArrayInUse(Pochoir_Array<T, N_RANK, TOGGLE> & arr) {
    arr_list_[arr_idx_] = &(arr);
    ++arr_idx_; ++arr_len_;
}

template <typename T, int N_RANK, int TOGGLE> template <size_t N_SIZE>
void Pochoir_Stencil<T, N_RANK, TOGGLE>::registerShape(Pochoir_Shape_info<N_RANK> (& shape)[N_SIZE]) {
    /* currently we just get the slope_[] out of the shape[] */
    int l_min_time_shift=0, l_max_time_shift=0, time_slope=0;
    for (int i = 0; i < N_SIZE; ++i) {
        if (shape[i].shift[0] < l_min_time_shift)
            l_min_time_shift = shape[i].shift[0];
        if (shape[i].shift[0] > l_max_time_shift)
            l_max_time_shift = shape[i].shift[0];
        for (int r = 1; r < N_RANK+1; ++r) {
            slope_[N_RANK-r] = max(slope_[N_RANK-r], abs(shape[i].shift[r]));
        }
    }
    time_slope = l_max_time_shift - l_min_time_shift;
    for (int i = 0; i < N_RANK; ++i) {
        slope_[i] = (int)ceil((float)slope_[i]/time_slope);
    }
}

template <typename T, int N_RANK, int TOGGLE> template <typename Range>
void Pochoir_Stencil<T, N_RANK, TOGGLE>::registerDomain(Range const & r_i, Range const & r_j, Range const & r_k) {
    grid_.x0[2] = r_i.first();
    grid_.x1[2] = r_i.first() + r_i.size();
    grid_.x0[1] = r_j.first();
    grid_.x1[1] = r_j.first() + r_j.size();
    grid_.x0[0] = r_k.first();
    grid_.x1[0] = r_k.first() + r_k.size();
    logic_size_[2] = r_i.size(); logic_size_[1] = r_j.size(); logic_size_[0] = r_k.size();
    stride_[2] = r_i.stride();
    stride_[1] = r_j.stride();
    stride_[0] = r_k.stride();
}

template <typename T, int N_RANK, int TOGGLE> template <typename Range>
void Pochoir_Stencil<T, N_RANK, TOGGLE>::registerDomain(Range const & r_i, Range const & r_j) {
    grid_.x0[1] = r_i.first();
    grid_.x1[1] = r_i.first() + r_i.size();
    grid_.x0[0] = r_j.first();
    grid_.x1[0] = r_j.first() + r_j.size();
    logic_size_[1] = r_i.size(); logic_size_[0] = r_j.size();
    stride_[1] = r_i.stride();
    stride_[0] = r_j.stride();
}

template <typename T, int N_RANK, int TOGGLE> template <typename Range>
void Pochoir_Stencil<T, N_RANK, TOGGLE>::registerDomain(Range const & r_i) {
    grid_.x0[0] = r_i.first();
    grid_.x1[0] = r_i.first() + r_i.size();
    logic_size_[0] = r_i.size();
    stride_[0] = r_i.stride();
}

/* Executable Spec */
template <typename T, int N_RANK, int TOGGLE> template <typename BF>
void Pochoir_Stencil<T, N_RANK, TOGGLE>::run(int timestep, BF const & bf) {
    Algorithm<N_RANK, grid_info<N_RANK> > algor(slope_);
    algor.set_initial_grid(grid_);
    algor.set_stride(stride_);
    algor.set_logic_size(logic_size_);
    /* this version uses 'f' to compute interior region, 
     * and 'bf' to compute boundary region
     */
    timestep_ = timestep;
    for (int i = 0; i < arr_len_; ++i) {
        arr_list_[i]->registerSlope(slope_);
        arr_list_[i]->set_logic_size(logic_size_);
    }
    /* base_case_kernel() will mimic exact the behavior of serial nested loop!
    */
    algor.base_case_kernel(0, timestep, grid_, bf);
    /* obase_boundary_p() is a parallel divide-and-conquer algorithm, which checks
     * boundary for every point
     */
    // algor.obase_boundary_p(0, timestep, grid_, bf);
}

/* safe/non-safe ExecSpec */
template <typename T, int N_RANK, int TOGGLE> template <typename F, typename BF>
void Pochoir_Stencil<T, N_RANK, TOGGLE>::run(int timestep, F const & f, BF const & bf) {
    Algorithm<N_RANK, grid_info<N_RANK> > algor(slope_);
    algor.set_initial_grid(grid_);
    algor.set_stride(stride_);
    algor.set_logic_size(logic_size_);
    /* this version uses 'f' to compute interior region, 
     * and 'bf' to compute boundary region
     */
    timestep_ = timestep;
    for (int i = 0; i < arr_len_; ++i) {
        arr_list_[i]->registerSlope(slope_);
        arr_list_[i]->set_logic_size(logic_size_);
    }
    algor.walk_bicut_boundary_p(0, timestep, grid_, f, bf);
}

/* obase for zero-padded area! */
template <typename T, int N_RANK, int TOGGLE> template <typename F>
void Pochoir_Stencil<T, N_RANK, TOGGLE>::run_obase(int timestep, F const & f) {
    Algorithm<N_RANK, grid_info<N_RANK> > algor(slope_);
    algor.set_initial_grid(grid_);
    algor.set_stride(stride_);
    algor.set_logic_size(logic_size_);
    timestep_ = timestep;
    for (int i = 0; i < arr_len_; ++i) {
        arr_list_[i]->registerSlope(slope_);
        arr_list_[i]->set_logic_size(logic_size_);
    }
//  It seems that whether it's bicut or adaptive cut only matters in small scale!
    algor.obase_bicut(0, timestep, grid_, f);
//    algor.obase_adaptive(0, timestep, grid_, f);
}

/* obase for interior and ExecSpec for boundary */
template <typename T, int N_RANK, int TOGGLE> template <typename F, typename BF>
void Pochoir_Stencil<T, N_RANK, TOGGLE>::run_obase(int timestep, F const & f, BF const & bf) {
    Algorithm<N_RANK, grid_info<N_RANK> > algor(slope_);
    algor.set_initial_grid(grid_);
    algor.set_stride(stride_);
    algor.set_logic_size(logic_size_);
    /* this version uses 'f' to compute interior region, 
     * and 'bf' to compute boundary region
     */
    timestep_ = timestep;
    for (int i = 0; i < arr_len_; ++i) {
        arr_list_[i]->registerSlope(slope_);
        arr_list_[i]->set_logic_size(logic_size_);
    }
    algor.obase_bicut_boundary_p(0, timestep, grid_, f, bf);
}

#endif
