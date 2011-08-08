/*
 **********************************************************************************
 *  Copyright (C) 2010-2011  Massachusetts Institute of Technology
 *  Copyright (C) 2010-2011  Yuan Tang <yuantang@csail.mit.edu>
 * 		                     Charles E. Leiserson <cel@mit.edu>
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
#include "pochoir_types.hpp"
#include "pochoir_kernel.hpp"
#include "pochoir_walk.hpp"
#include "pochoir_array.hpp"
/* assuming there won't be more than 10 Pochoir_Array in one Pochoir object! */
#define ARRAY_SIZE 10

const char * base_data_file_name = "pochoir_base.dat";
const char * sync_data_file_name = "pochoir_sync.dat";

template <int N_RANK>
class Pochoir {
    private:
        int slope_[N_RANK];
        Grid_Info<N_RANK> logic_grid_;
        Grid_Info<N_RANK> phys_grid_;
        int time_shift_;
        int toggle_;
        int timestep_;
        bool regArrayFlag, regLogicDomainFlag, regPhysDomainFlag, regShapeFlag;
        void checkFlag(bool flag, char const * str);
        void checkFlags(void);
        template <typename T_Array>
        void getPhysDomainFromArray(T_Array & arr);
        template <typename T_Array>
        void cmpPhysDomainFromArray(T_Array & arr);
        void Register_Shape(Pochoir_Shape<N_RANK> * shape, int N_SIZE);
        Pochoir_Shape<N_RANK> * shape_;
        int shape_size_;
        int num_arr_;
        int arr_type_size_;
        int sz_pgk_;
        int lcm_unroll_;
        /* assuming that the number of distinct sub-regions is less than 10 */
        Pochoir_Guard_Kernel<N_RANK> * pgk_;
        Pochoir_Obase_Guard_Kernel<N_RANK> * opgk_;

        /* Private Register Kernel Function */
        template <typename K>
        void reg_kernel(int pt, K k);
        template <typename K, typename ... KS>
        void reg_kernel(int pt, K k, KS ... ks);

        Spawn_Tree<N_RANK> * tree_;
        Node_Info<N_RANK> * root_;
        Vector_Info< Region_Info<N_RANK> > * base_data_;
        Vector_Info<int> * sync_data_;
        int sz_base_data_, sz_sync_data_;
    public:
    template <typename ... KS>
    void Register_Kernel(typename Pochoir_Types<N_RANK>::T_Guard g, KS ... ks);
    void Register_Obase_Kernel(typename Pochoir_Types<N_RANK>::T_Guard g, int unroll, Pochoir_Obase_Kernel<N_RANK> & k, Pochoir_Obase_Kernel<N_RANK> & cond_k, Pochoir_Obase_Kernel<N_RANK> & bk, Pochoir_Obase_Kernel<N_RANK> & cond_bk);
    // get slope(s)
    int slope(int const _idx) { return slope_[_idx]; }
    Pochoir() {
        for (int i = 0; i < N_RANK; ++i) {
            slope_[i] = 0;
            logic_grid_.x0[i] = logic_grid_.x1[i] = logic_grid_.dx0[i] = logic_grid_.dx1[i] = 0;
            phys_grid_.x0[i] = phys_grid_.x1[i] = phys_grid_.dx0[i] = phys_grid_.dx1[i] = 0;
        }
        shape_ = NULL; shape_size_ = 0; time_shift_ = 0; toggle_ = 0;
        timestep_ = 0;
        regArrayFlag = regLogicDomainFlag = regPhysDomainFlag = regShapeFlag = false;
        num_arr_ = 0;
        arr_type_size_ = 0;
        sz_pgk_ = 0;
        pgk_ = NULL; opgk_ = NULL; 
        lcm_unroll_ = 1;
    }

    /* currently, we just compute the slope[] out of the shape[] */
    /* We get the Grid_Info out of arrayInUse */
    template <typename T>
    void Register_Array(Pochoir_Array<T, N_RANK> & a);
    template <typename T, typename ... TS>
    void Register_Array(Pochoir_Array<T, N_RANK> & a, Pochoir_Array<TS, N_RANK> ... as);

    /* We should still keep the Register_Domain for zero-padding!!! */
    template <typename D>
    void Register_Domain(D const & d);
    template <typename D, typename ... DS>
    void Register_Domain(D const & d, DS ... ds);
    /* register boundary value function with corresponding Pochoir_Array object directly */
    template <typename T_Array, typename RET>
    void registerBoundaryFn(T_Array & arr, RET (*_bv)(T_Array &, int, int, int)) {
        arr.Register_Boundary(_bv);
        Register_Array(arr);
    } 
    Grid_Info<N_RANK> get_phys_grid(void);

    /* Executable Spec */
    void Run(int timestep);
    void Run_Obase(int timestep);
    Pochoir_Plan<N_RANK> & Gen_Plan(int timepste);
    Pochoir_Plan<N_RANK> & Load_Plan(void);
    void Run_Plan(Pochoir_Plan<N_RANK> & _plan);
#if 0
    /* obsolete methods -- to remove */
    /* obase for zero-padded region */
    template <typename F>
    void Run_Obase(int timestep, F const & f);
    /* obase for interior and ExecSpec for boundary */
    template <typename F, typename BF>
    void Run_Obase(int timestep, F const & f, BF const & bf);
#endif
};

template <int N_RANK> template <typename K>
void Pochoir<N_RANK>::reg_kernel(int pt, K k) {
    pgk_[sz_pgk_].kernel_[pt] = k; 
    Register_Shape(k.Get_Shape(), k.Get_Shape_Size());
}

template <int N_RANK> template <typename K, typename ... KS>
void Pochoir<N_RANK>::reg_kernel(int pt, K k, KS ... ks) {
    pgk_[sz_pgk_].kernel_[pt] = k;
    Register_Shape(k.Get_Shape(), k.Get_Shape_Size());
    reg_kernel(pt+1, ks ...);
}

template <int N_RANK> template <typename ... KS>
void Pochoir<N_RANK>::Register_Kernel(typename Pochoir_Types<N_RANK>::T_Guard g, KS ... ks) {
    int l_size = sizeof...(KS);
    typedef Pochoir_Kernel<N_RANK> T_Kernel;
    if (pgk_ == NULL) {
        pgk_ = new Pochoir_Guard_Kernel<N_RANK>[ARRAY_SIZE];
        sz_pgk_ = 0;
        assert(lcm_unroll_ == 1);
    }
    assert(sz_pgk_ < ARRAY_SIZE);
    if (sz_pgk_ >= ARRAY_SIZE) {
        printf("Pochoir Error: Register_Kernel > %d\n", sz_pgk_);
        exit(1);
    }
    pgk_[sz_pgk_].unroll_ = l_size;
    pgk_[sz_pgk_].pointer_ = 0;
    pgk_[sz_pgk_].guard_ = g;
    pgk_[sz_pgk_].kernel_ = (T_Kernel *) calloc(l_size, sizeof(T_Kernel));
    reg_kernel(0, ks ...);
    lcm_unroll_ = lcm(lcm_unroll_, l_size);
    ++sz_pgk_;
}

template <int N_RANK> 
void Pochoir<N_RANK>::Register_Obase_Kernel(typename Pochoir_Types<N_RANK>::T_Guard g, int unroll, Pochoir_Obase_Kernel<N_RANK> & k, Pochoir_Obase_Kernel<N_RANK> & cond_k, Pochoir_Obase_Kernel<N_RANK> & bk, Pochoir_Obase_Kernel<N_RANK> & cond_bk) {
    typedef Pochoir_Obase_Kernel<N_RANK> T_Kernel;
    if (opgk_ == NULL) {
        opgk_ = new Pochoir_Obase_Guard_Kernel<N_RANK>[ARRAY_SIZE];
        sz_pgk_ = 0;
        assert(lcm_unroll_ == 1);
    }
    assert(sz_pgk_ < ARRAY_SIZE);
    if (sz_pgk_ >= ARRAY_SIZE) {
        printf("Pochoir Error: Register_Kernel > %d\n", sz_pgk_);
        exit(1);
    }
    opgk_[sz_pgk_].guard_ = g;
    opgk_[sz_pgk_].unroll_ = unroll;
    opgk_[sz_pgk_].kernel_ = (T_Kernel *) calloc(1, sizeof(T_Kernel));
    opgk_[sz_pgk_].kernel_[0] = k;
    Register_Shape(k.Get_Shape(), k.Get_Shape_Size());
    opgk_[sz_pgk_].cond_kernel_ = (T_Kernel *) calloc(1, sizeof(T_Kernel));
    opgk_[sz_pgk_].cond_kernel_[0] = cond_k;
    Register_Shape(cond_k.Get_Shape(), cond_k.Get_Shape_Size());
    opgk_[sz_pgk_].bkernel_ = (T_Kernel *) calloc(1, sizeof(T_Kernel));
    opgk_[sz_pgk_].bkernel_[0] = bk;
    Register_Shape(bk.Get_Shape(), bk.Get_Shape_Size());
    opgk_[sz_pgk_].cond_bkernel_ = (T_Kernel *) calloc(1, sizeof(T_Kernel));
    opgk_[sz_pgk_].cond_bkernel_[0] = cond_bk;
    Register_Shape(cond_bk.Get_Shape(), cond_bk.Get_Shape_Size());
    lcm_unroll_ = lcm(lcm_unroll_, unroll);
    ++sz_pgk_;
}

template <int N_RANK>
void Pochoir<N_RANK>::checkFlag(bool flag, char const * str) {
    if (!flag) {
        printf("\nPochoir registration error:\n");
        printf("You forgot to register %s.\n", str);
        exit(1);
    }
}

template <int N_RANK>
void Pochoir<N_RANK>::checkFlags(void) {
    checkFlag(regArrayFlag, "Pochoir array");
    checkFlag(regLogicDomainFlag, "Logic Domain");
    checkFlag(regPhysDomainFlag, "Physical Domain");
    checkFlag(regShapeFlag, "Shape");
    return;
}

template <int N_RANK> template <typename T_Array> 
void Pochoir<N_RANK>::getPhysDomainFromArray(T_Array & arr) {
    /* get the physical grid */
    for (int i = 0; i < N_RANK; ++i) {
        phys_grid_.x0[i] = 0; phys_grid_.x1[i] = arr.size(i);
        /* if logic domain is not set, let's set it the same as physical grid */
        if (!regLogicDomainFlag) {
            logic_grid_.x0[i] = 0; logic_grid_.x1[i] = arr.size(i);
        }
    }

    regPhysDomainFlag = true;
    regLogicDomainFlag = true;
}

template <int N_RANK> template <typename T_Array> 
void Pochoir<N_RANK>::cmpPhysDomainFromArray(T_Array & arr) {
    /* check the consistency of all engaged Pochoir_Array */
    for (int j = 0; j < N_RANK; ++j) {
        if (arr.size(j) != phys_grid_.x1[j]) {
            printf("Pochoir array size mismatch error:\n");
            printf("Registered Pochoir arrays have different sizes!\n");
            exit(1);
        }
    }
}

template <int N_RANK> template <typename T>
void Pochoir<N_RANK>::Register_Array(Pochoir_Array<T, N_RANK> & a) {
    if (!regShapeFlag) {
        printf("Please register Shape before register Array!\n");
        exit(1);
    }

    if (num_arr_ == 0) {
        arr_type_size_ = sizeof(T);
        // arr_type_size_ = sizeof(double);
#if DEBUG
        printf("<%s:%d> arr_type_size = %d\n", __FILE__, __LINE__, arr_type_size_);
#endif
        ++num_arr_;
    } 
    if (!regPhysDomainFlag) {
        getPhysDomainFromArray(a);
    } else {
        cmpPhysDomainFromArray(a);
    }
    a.Register_Shape(shape_, shape_size_);
#if 0
    arr.set_slope(slope_);
    arr.set_toggle(toggle_);
    arr.alloc_mem();
#endif
    regArrayFlag = true;
}

template <int N_RANK> template <typename T, typename ... TS>
void Pochoir<N_RANK>::Register_Array(Pochoir_Array<T, N_RANK> & a, Pochoir_Array<TS, N_RANK> ... as) {
    if (!regShapeFlag) {
        printf("Please register Shape before register Array!\n");
        exit(1);
    }

    if (num_arr_ == 0) {
        arr_type_size_ = sizeof(T);
        // arr_type_size_ = sizeof(double);
#if DEBUG
        printf("<%s:%d> arr_type_size = %d\n", __FILE__, __LINE__, arr_type_size_);
#endif
        ++num_arr_;
    } 
    if (!regPhysDomainFlag) {
        getPhysDomainFromArray(a);
    } else {
        cmpPhysDomainFromArray(a);
    }
    a.Register_Shape(shape_, shape_size_);
#if 0
    arr.set_slope(slope_);
    arr.set_toggle(toggle_);
    arr.alloc_mem();
#endif
    Register_Array(as ...);
    regArrayFlag = true;
}

template <int N_RANK> 
void Pochoir<N_RANK>::Register_Shape(Pochoir_Shape<N_RANK> * shape, int N_SIZE) {
    Pochoir_Shape<N_RANK> * l_shape;
    if (!regShapeFlag) {
        regShapeFlag = true;
        l_shape = new Pochoir_Shape<N_RANK>[N_SIZE];
        shape_ = l_shape;
    } else {
        l_shape = new Pochoir_Shape<N_RANK>[N_SIZE + shape_size_] ;
        /* copy in the old shape value */
        for (int i = 0; i < shape_size_; ++i)  {
            for (int r = 0; r < N_RANK+1; ++r)  {
                l_shape[i].shift[r]  = shape_[i].shift[r];
            }
        }
        delete (shape_);
        shape_ = l_shape;
    }
    /* currently we just get the slope_[]  and toggle_ out of the shape[]  */
    int l_min_time_shift=0, l_max_time_shift=0, depth=0;
    for (int i = 0; i < N_SIZE; ++i)  {
        if (shape[i] .shift[0] < l_min_time_shift)
            l_min_time_shift = shape[i].shift[0];
        if (shape[i].shift[0] > l_max_time_shift)
            l_max_time_shift = shape[i].shift[0];
    }
    depth = l_max_time_shift - l_min_time_shift;
    time_shift_ = max(time_shift_, 0 - l_min_time_shift);
    toggle_ = max(toggle_, depth + 1);
    for (int i = 0; i < N_SIZE; ++i) {
        for (int r = 0; r < N_RANK+1; ++r) {
            slope_[N_RANK-r] = (r > 0) ? max(slope_[N_RANK-r], abs((int)ceil((float)shape_[i].shift[N_RANK-r]/(l_max_time_shift - shape_[i].shift[0])))) : 0;
            shape_[shape_size_ + i].shift[r] = (r > 0) ? shape[i].shift[r] : shape[i].shift[r] + time_shift_;
        }
    }
    shape_size_ += N_SIZE;
#if DEBUG
    printf("time_shift_ = %d, toggle = %d\n", time_shift_, toggle_);
    for (int r = 0; r < N_RANK; ++r) {
        printf("slope[%d] = %d, ", r, slope_[r]);
    }
    printf("\n");
#endif
}

template <int N_RANK> template <typename D>
void Pochoir<N_RANK>::Register_Domain(D const & d) {
    logic_grid_.x0[0] = d.first();
    logic_grid_.x1[0] = d.first() + d.size();
    regLogicDomainFlag = true;
}

template <int N_RANK> template <typename D, typename ... DS>
void Pochoir<N_RANK>::Register_Domain(D const & d, DS ... ds) {
    int l_pointer = sizeof...(DS);
    logic_grid_.x0[l_pointer] = d.first();
    logic_grid_.x1[l_pointer] = d.first() + d.size();
}

template <int N_RANK> 
Grid_Info<N_RANK> Pochoir<N_RANK>::get_phys_grid(void) {
    return phys_grid_;
}

/* Run the kernel functions stored in array of function pointers */
template <int N_RANK>
void Pochoir<N_RANK>::Run(int timestep) {
    Algorithm<N_RANK> algor(slope_);
    algor.set_phys_grid(phys_grid_);
    algor.set_thres(arr_type_size_);
    algor.set_unroll(lcm_unroll_);
    algor.set_pgk(sz_pgk_, pgk_);
    timestep_ = timestep;
    /* base_case_kernel() will mimic exact the behavior of serial nested loop!
    */
    checkFlags();
    inRun = true;
    algor.base_case_kernel_guard(0 + time_shift_, timestep + time_shift_, logic_grid_);
    inRun = false;
}

/* obase for interior and ExecSpec for boundary */
template <int N_RANK> 
void Pochoir<N_RANK>::Run_Obase(int timestep) {
//    int l_total_points = 1;
    Algorithm<N_RANK> algor(slope_);
    algor.set_phys_grid(phys_grid_);
    algor.set_thres(arr_type_size_);
    algor.set_obase_pgk(sz_pgk_, opgk_);
    algor.set_unroll(lcm_unroll_);
    timestep_ = timestep;
    checkFlags();
    // cutting based on shorter bar
    algor.adaptive_bicut_p(0 + time_shift_, timestep + time_shift_, logic_grid_);
}

template <int N_RANK> 
Pochoir_Plan<N_RANK> & Pochoir<N_RANK>::Gen_Plan(int timestep) {
    Pochoir_Plan<N_RANK> * l_plan = new Pochoir_Plan<N_RANK>();
    int l_sz_base_data, l_sz_sync_data;

    Algorithm<N_RANK> algor(slope_);
    algor.set_phys_grid(phys_grid_);
    algor.set_thres(arr_type_size_);
    /* set individual unroll factor from opgk_ */
    algor.set_pgk(sz_pgk_, pgk_);
    algor.set_unroll(lcm_unroll_);
    timestep_ = timestep;
    checkFlags();
    tree_ = new Spawn_Tree<N_RANK>();
    root_ = tree_->get_root();
    algor.set_tree(tree_);
    algor.gen_plan_bicut_p(root_, 0 + time_shift_, timestep + time_shift_, logic_grid_);
    l_sz_base_data = algor.get_sz_base_data();
    l_sz_sync_data = algor.get_sz_sync_data();
    l_plan->alloc_base_data(l_sz_base_data);
    l_plan->alloc_sync_data(l_sz_sync_data);
    printf("sz_base_data = %d, sz_sync_data = %d\n", l_sz_base_data, l_sz_sync_data);
    int l_tree_size_begin = tree_->size();
    printf("tree size = %d\n", l_tree_size_begin);
    /* after remove all nodes, the only remaining node will be the 'root' */
    while (l_tree_size_begin > 1) {
        tree_->dfs_until_sync(root_->left, (*(l_plan->base_data_)));
        int l_tree_size_end = tree_->size();
        l_plan->sync_data_->add_element(l_tree_size_begin - l_tree_size_end);
        tree_->dfs_rm_sync(root_->left);
        l_tree_size_begin = tree_->size();
        printf("tree size = %d\n", l_tree_size_begin);
    }
    l_plan->sync_data_->scan();
    l_plan->sync_data_->add_element(END_SYNC);
    /* extract data and store plan */
    ofstream os_base_data(base_data_file_name);
    ofstream os_sync_data(sync_data_file_name);
    if (os_base_data.is_open()) {
        printf("os_base_data is open!\n");
        os_base_data << (*(l_plan->base_data_));
    } else {
        printf("os_base_data is NOT open! exit!\n");
        exit(1);
    }
    if (os_sync_data.is_open()) {
        printf("os_sync_data is open!\n");
        os_sync_data << (*(l_plan->sync_data_));
    } else {
        printf("os_sync_data is NOT open! exit!\n");
        exit(1);
    }
    os_base_data.close();
    os_sync_data.close();
    return (*l_plan);
}

template <int N_RANK>
Pochoir_Plan<N_RANK> & Pochoir<N_RANK>::Load_Plan(void) {
    Pochoir_Plan<N_RANK> * l_plan = new Pochoir_Plan(10, 10);
    
    FILE * is_base_data = fopen(base_data_file_name, "r");

    if (is_base_data != NULL) {
        printf("is_base_data is open!\n");
        while (!feof(is_base_data)) {
            Region_Info<N_RANK> l_region;
            l_region.pscanf(is_base_data);
            l_plan->base_data_->add_element(l_region);
        }
    } else {
        printf("is_base_data is NOT open! exit!\n");
        exit(1);
    }
    fclose(is_base_data);
    std::cerr << "base_data : \n" << (*(l_plan->base_data_)) << std::endl;

    FILE * is_sync_data = fopen(sync_data_file_name, "r");
    if (is_sync_data != NULL) {
        printf("is_sync_data is open!\n");
        while (!feof(is_sync_data)) {
            int l_sync;
            fscanf(is_sync_data, "%d\n", &l_sync);
            l_plan->sync_data_->add_element(l_sync);
        }
    } else {
        printf("is_sync_data is NOT open! exit!\n");
        exit(1);
    }
    fclose(is_sync_data);
    std::cerr << "sync_data : \n" << (*(l_plan->sync_data_)) << std::endl;

    return (*l_plan);
}

template <int N_RANK>
void Pochoir<N_RANK>::Run_Plan(Pochoir_Plan<N_RANK> & _plan) {
    int offset = 0;
    for (int j = 0; _plan.sync_data_->region_[j] != END_SYNC; ++j) {
        for (int i = offset; i < _plan.sync_data_->region_[j]; ++i) {
            int l_region_n = _plan.base_data_->region_[i].region_n;
            int l_t0 = _plan.base_data_->region_[i].t0;
            int l_t1 = _plan.base_data_->region_[i].t1;
            Grid_Info<N_RANK> l_grid = _plan.base_data_->region_[i].grid;
            Pochoir_Region_Kernel<N_RANK> l_kernel(pgk_[l_region_n]);
            l_kernel.set_pointer(l_t0 - time_shift_);
            for (int t = l_t0; t < l_t1; ) {
                meta_grid_boundary<N_RANK>::single_step(t, l_grid, phys_grid_, l_kernel);
                for (int i = 0; i < N_RANK; ++i) {
                    l_grid.x0[i] += l_grid.dx0[i]; l_grid.x1[i] += l_grid.dx1[i];
                }
                ++t;
                l_kernel.shift_pointer();
            }
        }
        offset = _plan.sync_data_->region_[j];
    }
    return;
}
#endif
