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

#include "pochoir_common.hpp"
#include "pochoir_array.hpp"
#include "pochoir_iter.hpp"
/* assuming there won't be more than 10 Pochoir_Array in one Pochoir object! */
#define ARRAY_SIZE 10
template <typename T, int N_RANK, int TOGGLE=2>
class Pochoir {
    private:
        int slope_[N_RANK];
        int stride_[N_RANK];
        grid_info<N_RANK> logic_grid_;
        grid_info<N_RANK> phys_grid_;
        int time_shift_;
        int timestep_;
        Pochoir_Array<T, N_RANK, TOGGLE> ** arr_list_;
        typedef T (*BValue_1D)(Pochoir_Array<T, 1, TOGGLE> &, int, int);
        typedef T (*BValue_2D)(Pochoir_Array<T, 2, TOGGLE> &, int, int, int);
        typedef T (*BValue_3D)(Pochoir_Array<T, 3, TOGGLE> &, int, int, int, int);
        int arr_len_;
        int arr_idx_;
        bool regArrayFlag, regLogicDomainFlag, regPhysDomainFlag, regShapeFlag;
        void checkFlag(bool flag, char const * str);
        void checkFlags(void);
        void getPhysDomainFromArray(void);

    public:
    Pochoir() {
        for (int i = 0; i < N_RANK; ++i) {
            slope_[i] = 0;
            logic_grid_.x0[i] = logic_grid_.x1[i] = logic_grid_.dx0[i] = logic_grid_.dx1[i] = 0;
            phys_grid_.x0[i] = phys_grid_.x1[i] = phys_grid_.dx0[i] = phys_grid_.dx1[i] = 0;
        }
        timestep_ = 0;
        arr_list_ = (Pochoir_Array<T, N_RANK, TOGGLE>**)calloc(ARRAY_SIZE, sizeof(Pochoir_Array<T, N_RANK, TOGGLE>*));
        arr_len_ = 0;
        arr_idx_ = 0;
        regArrayFlag = regLogicDomainFlag = regPhysDomainFlag = regShapeFlag = false;
    }
    /* currently, we just compute the slope[] out of the shape[] */
    /* We get the grid_info out of arrayInUse */
    void registerArray(Pochoir_Array<T, N_RANK, TOGGLE> & arr);
    template <size_t N_SIZE>
    void registerShape(Pochoir_Shape<N_RANK> (& shape)[N_SIZE]);

    /* We should still keep the registerDomain for zero-padding!!! */
    template <typename Domain>
    void registerDomain(Domain const & i);
    template <typename Domain>
    void registerDomain(Domain const & i, Domain const & j);
    template <typename Domain>
    void registerDomain(Domain const & i, Domain const & j, Domain const & k);

    /* register boundary value function with corresponding Pochoir_Array object directly */
    void registerBoundaryFn(Pochoir_Array<T, 1, TOGGLE> & arr, BValue_1D _bv1) {
        arr.registerBV(_bv1);
        registerArray(arr);
    } 
    void registerBoundaryFn(Pochoir_Array<T, 2, TOGGLE> & arr, BValue_2D _bv2) {
        arr.registerBV(_bv2);
        registerArray(arr);
    } 
    void registerBoundaryFn(Pochoir_Array<T, 3, TOGGLE> & arr, BValue_3D _bv3) {
        arr.registerBV(_bv3);
        registerArray(arr);
    } 
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
void Pochoir<T, N_RANK, TOGGLE>::checkFlag(bool flag, char const * str) {
    if (!flag) {
        printf("\n<%s:%s:%d> :\nYou forgot register %s!\n", __FILE__, __FUNCTION__, __LINE__, str);
        exit(1);
    }
}

template <typename T, int N_RANK, int TOGGLE>
void Pochoir<T, N_RANK, TOGGLE>::checkFlags(void) {
    checkFlag(regArrayFlag, "Array");
    checkFlag(regLogicDomainFlag, "Logic Domain");
    checkFlag(regPhysDomainFlag, "Physical Domain");
    checkFlag(regShapeFlag, "Shape");
    return;
}

template <typename T, int N_RANK, int TOGGLE> 
void Pochoir<T, N_RANK, TOGGLE>::getPhysDomainFromArray(void) {
    if (arr_len_ == 0) {
        printf("No Pochoir_Array registered! Quit!\n");
        exit(1);
    }
    /* get the physical grid */
    for (int i = 0; i < N_RANK; ++i) {
        phys_grid_.x0[i] = 0; phys_grid_.x1[i] = arr_list_[0]->size(i);
        /* if logic domain is not set, let's set it the same as physical grid */
        if (!regLogicDomainFlag) {
            logic_grid_.x0[i] = 0; logic_grid_.x1[i] = arr_list_[0]->size(i);
        }
        stride_[i] = 1;
    }

    /* check the consistency of all engaged Pochoir_Array */
    for (int i = 1; i < arr_len_; ++i) {
        for (int j = 0; j < N_RANK; ++j) {
            if (arr_list_[i]->size(j) != phys_grid_.x1[j]) {
                printf("Not all engaged Pochoir_Arrays are of the same size!! Quit!\n");
                exit(1);
            }
        }
    }
    regPhysDomainFlag = true;
    regLogicDomainFlag = true;
}

template <typename T, int N_RANK, int TOGGLE>
void Pochoir<T, N_RANK, TOGGLE>::registerArray(Pochoir_Array<T, N_RANK, TOGGLE> & arr) {
    arr_list_[arr_idx_] = &(arr);
    ++arr_idx_; ++arr_len_;
    regArrayFlag = true;
}

template <typename T, int N_RANK, int TOGGLE> template <size_t N_SIZE>
void Pochoir<T, N_RANK, TOGGLE>::registerShape(Pochoir_Shape<N_RANK> (& shape)[N_SIZE]) {
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
    time_shift_ = 0 - l_min_time_shift;
//    cout << "time_shift_ = " << time_shift_ << endl;
    for (int i = 0; i < N_RANK; ++i) {
        slope_[i] = (int)ceil((float)slope_[i]/time_slope);
    }
    regShapeFlag = true;
}

template <typename T, int N_RANK, int TOGGLE> template <typename Domain>
void Pochoir<T, N_RANK, TOGGLE>::registerDomain(Domain const & r_i, Domain const & r_j, Domain const & r_k) {
    logic_grid_.x0[2] = r_i.first();
    logic_grid_.x1[2] = r_i.first() + r_i.size();
    logic_grid_.x0[1] = r_j.first();
    logic_grid_.x1[1] = r_j.first() + r_j.size();
    logic_grid_.x0[0] = r_k.first();
    logic_grid_.x1[0] = r_k.first() + r_k.size();
    stride_[2] = r_i.stride();
    stride_[1] = r_j.stride();
    stride_[0] = r_k.stride();
    regLogicDomainFlag = true;
}

template <typename T, int N_RANK, int TOGGLE> template <typename Domain>
void Pochoir<T, N_RANK, TOGGLE>::registerDomain(Domain const & r_i, Domain const & r_j) {
    logic_grid_.x0[1] = r_i.first();
    logic_grid_.x1[1] = r_i.first() + r_i.size();
    logic_grid_.x0[0] = r_j.first();
    logic_grid_.x1[0] = r_j.first() + r_j.size();
    stride_[1] = r_i.stride();
    stride_[0] = r_j.stride();
    regLogicDomainFlag = true;
}

template <typename T, int N_RANK, int TOGGLE> template <typename Domain>
void Pochoir<T, N_RANK, TOGGLE>::registerDomain(Domain const & r_i) {
    logic_grid_.x0[0] = r_i.first();
    logic_grid_.x1[0] = r_i.first() + r_i.size();
    stride_[0] = r_i.stride();
    regLogicDomainFlag = true;
}

/* Executable Spec */
template <typename T, int N_RANK, int TOGGLE> template <typename BF>
void Pochoir<T, N_RANK, TOGGLE>::run(int timestep, BF const & bf) {
    /* this version uses 'f' to compute interior region, 
     * and 'bf' to compute boundary region
     */
    Algorithm<N_RANK> algor(slope_);
    getPhysDomainFromArray();
    algor.set_phys_grid(phys_grid_);
    algor.set_stride(stride_);
    timestep_ = timestep;
    /* base_case_kernel() will mimic exact the behavior of serial nested loop!
    */
    checkFlags();
    algor.base_case_kernel_boundary(0 + time_shift_, timestep + time_shift_, logic_grid_, bf);
    // algor.sim_bicut_zero(0 + time_shift_, timestep + time_shift_, logic_grid_, bf);
    /* obase_boundary_p() is a parallel divide-and-conquer algorithm, which checks
     * boundary for every point
     */
    // algor.obase_boundary_p(0, timestep, logic_grid_, bf);
}

/* safe/non-safe ExecSpec */
template <typename T, int N_RANK, int TOGGLE> template <typename F, typename BF>
void Pochoir<T, N_RANK, TOGGLE>::run(int timestep, F const & f, BF const & bf) {
    Algorithm<N_RANK> algor(slope_);
    getPhysDomainFromArray();
    algor.set_phys_grid(phys_grid_);
    algor.set_stride(stride_);
    /* this version uses 'f' to compute interior region, 
     * and 'bf' to compute boundary region
     */
    timestep_ = timestep;
    checkFlags();
#if BICUT
#if 1
    algor.walk_bicut_boundary_p(0+time_shift_, timestep+time_shift_, logic_grid_, f, bf);
#else
    algor.sim_bicut_p(0+time_shift_, timestep+time_shift_, logic_grid_, f, bf);
#endif
#else
    algor.walk_ncores_boundary_p(0+time_shift_, timestep+time_shift_, logic_grid_, f, bf);
#endif
}

/* obase for zero-padded area! */
template <typename T, int N_RANK, int TOGGLE> template <typename F>
void Pochoir<T, N_RANK, TOGGLE>::run_obase(int timestep, F const & f) {
    Algorithm<N_RANK> algor(slope_);
    getPhysDomainFromArray();
    algor.set_phys_grid(phys_grid_);
    algor.set_stride(stride_);
    timestep_ = timestep;
    checkFlags();
//  It seems that whether it's bicut or adaptive cut only matters in small scale!
#if BICUT
#if 0
    fprintf(stderr, "Call obase_bicut\n");
    algor.obase_bicut(0+time_shift_, timestep+time_shift_, logic_grid_, f);
#else
    fprintf(stderr, "Call sim_obase_bicut\n");
    algor.sim_obase_bicut(0+time_shift_, timestep+time_shift_, logic_grid_, f);
#if STAT
    for (int i = 1; i < SUPPORT_RANK; ++i) {
        fprintf(stderr, "sim_count_cut[%d] = %ld\n", i, sim_count_cut[i]);
    }
#endif
#endif
#else
    algor.obase_m(0+time_shift_, timestep+time_shift_, logic_grid_, f);
#endif
}

/* obase for interior and ExecSpec for boundary */
template <typename T, int N_RANK, int TOGGLE> template <typename F, typename BF>
void Pochoir<T, N_RANK, TOGGLE>::run_obase(int timestep, F const & f, BF const & bf) {
    Algorithm<N_RANK> algor(slope_);
    getPhysDomainFromArray();
    algor.set_phys_grid(phys_grid_);
    algor.set_stride(stride_);
    /* this version uses 'f' to compute interior region, 
     * and 'bf' to compute boundary region
     */
    timestep_ = timestep;
    checkFlags();
#if BICUT
#if 0
    fprintf(stderr, "Call obase_bicut_boundary_P\n");
    algor.obase_bicut_boundary_p(0+time_shift_, timestep+time_shift_, logic_grid_, f, bf);
#else
    fprintf(stderr, "Call sim_obase_bicut_P\n");
    algor.sim_obase_bicut_p(0+time_shift_, timestep+time_shift_, logic_grid_, f, bf);
#if STAT
    for (int i = 1; i < SUPPORT_RANK; ++i) {
        fprintf(stderr, "sim_count_cut[%d] = %ld\n", i, sim_count_cut[i]);
    }
#endif
#endif
#else
    algor.obase_boundary_p(0+time_shift_, timestep+time_shift_, logic_grid_, f, bf);
#endif
}

#endif
