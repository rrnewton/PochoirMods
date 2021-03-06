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
 ********************************************************************************/

#ifndef POCHOIR_WALK_H
#define POCHOIR_WALK_H

#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <iostream>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include "pochoir_common.hpp"

using namespace std;

template <int N_RANK, typename BF>
struct meta_grid_boundary {
	static inline void single_step(int t, grid_info<N_RANK> const & grid, grid_info<N_RANK> const & initial_grid, BF const & bf); 
};

template <typename BF>
struct meta_grid_boundary <3, BF>{
	static inline void single_step(int t, grid_info<3> const & grid, grid_info<3> const & initial_grid, BF const & bf) {
        /* add cilk_for here will only lower the performance */
		for (int i = grid.x0[2]; i < grid.x1[2]; ++i) {
            int new_i = pmod_lu(i, initial_grid.x0[2], initial_grid.x1[2]);
			for (int j = grid.x0[1]; j < grid.x1[1]; ++j) {
                int new_j = pmod_lu(j, initial_grid.x0[1], initial_grid.x1[1]);
        for (int k = grid.x0[0]; k < grid.x1[0]; ++k) {
            int new_k = pmod_lu(k, initial_grid.x0[0], initial_grid.x1[0]);
                bf(t, new_i, new_j, new_k);
        }
			}
		}
	} 
};

template <typename BF>
struct meta_grid_boundary <2, BF>{
	static inline void single_step(int t, grid_info<2> const & grid, grid_info<2> const & initial_grid, BF const & bf) {
		for (int i = grid.x0[1]; i < grid.x1[1]; ++i) {
            int new_i = pmod_lu(i, initial_grid.x0[1], initial_grid.x1[1]);
			for (int j = grid.x0[0]; j < grid.x1[0]; ++j) {
                int new_j = pmod_lu(j, initial_grid.x0[0], initial_grid.x1[0]);
                bf(t, new_i, new_j);
			}
		}
	} 
};

template <typename BF>
struct meta_grid_boundary <1, BF>{
	static inline void single_step(int t, grid_info<1> const & grid, grid_info<1> const & initial_grid, BF const & bf) {
		for (int i = grid.x0[0]; i < grid.x1[0]; ++i) {
            int new_i = pmod_lu(i, initial_grid.x0[0], initial_grid.x1[0]);
		    bf(t, new_i);
        }
	} 
};

template <int N_RANK, typename F>
struct meta_grid_interior {
	static inline void single_step(int t, grid_info<N_RANK> const & grid, grid_info<N_RANK> const & initial_grid, F const & f); 
};

template <typename F>
struct meta_grid_interior <3, F>{
	static inline void single_step(int t, grid_info<3> const & grid, grid_info<3> const & initial_grid, F const & f) {
        /* add cilk_for here will only lower the performance */
		for (int i = grid.x0[2]; i < grid.x1[2]; ++i) {
			for (int j = grid.x0[1]; j < grid.x1[1]; ++j) {
        for (int k = grid.x0[0]; k < grid.x1[0]; ++k) {
                f(t, i, j, k);
        }
			}
		}
	} 
};

template <typename F>
struct meta_grid_interior <2, F>{
	static inline void single_step(int t, grid_info<2> const & grid, grid_info<2> const & initial_grid, F const & f) {
		for (int i = grid.x0[1]; i < grid.x1[1]; ++i) {
			for (int j = grid.x0[0]; j < grid.x1[0]; ++j) {
                f(t, i, j);
			}
		}
	} 
};

template <typename F>
struct meta_grid_interior <1, F>{
	static inline void single_step(int t, grid_info<1> const & grid, grid_info<1> const & initial_grid, F const & f) {
		for (int i = grid.x0[0]; i < grid.x1[0]; ++i) {
		    f(t, i);
        }
	} 
};

static inline void set_worker_count(const char * nstr) 
{
#if 1
    if (0 != __cilkrts_set_param("nworkers", nstr)) {
        printf("Failed to set worker count\n");
    } else {
        printf("Successfully set worker count to %s\n", nstr);
    }
#endif
}

template <int N_RANK>
struct Algorithm {
	private:
        /* different stencils will have different slopes */
        /* We cut coarser in internal region, finer at boundary
         * to maximize the performance and reduce the region that
         * needs special treatment
        */
        int dx_recursive_[N_RANK];
        int dx_recursive_boundary_[N_RANK];
        const int dt_recursive_;
        const int dt_recursive_boundary_;
        int N_CORES;
	public:
    typedef enum {TILE_NCORES, TILE_BOUNDARY, TILE_MP} algor_type;
    typedef int index_info[N_RANK];

    grid_info<N_RANK> initial_grid_;
    int initial_length_[N_RANK];
    int logic_size_[N_RANK];
	int slope_[N_RANK];
    int stride_[N_RANK];
    int ulb_boundary[N_RANK], uub_boundary[N_RANK], lub_boundary[N_RANK];
    bool boundarySet, initialGridSet, slopeSet;
    
#pragma isat tuning name(tune_coarsening_factor) scope(M1_begin, M1_end) measure(M2_begin, M2_end) variable(tune_dt, range(1, 2, 1)) variable(tune_dt_boundary, range(1, 2, 1)) variable(tune_dx_boundary, range(1, 2, 1)) variable(tune_dx_i, range(1, 100, 50)) variable(tune_dx_0, range(1, 100, 50)) search(dependent)
    /* constructor */
#if 1
    static const int tune_dt = 5, tune_dt_boundary = 1;
    static const int tune_dx_boundary = 1;
//    static const int tune_dx_i = 3;
//    static const int tune_dx_0 = 1000;
//  suppose that we support arbitrary number of ranks up to 8
#define SUPPORT_RANK 8
    int tune_dx[SUPPORT_RANK][SUPPORT_RANK];
    int tune_dt[SUPPORT_RANK];
#endif

    /* constructor */
#pragma isat marker M1_begin
    Algorithm (int const _slope[]) : dt_recursive_(tune_dt[N_RANK]), dt_recursive_boundary_(tune_dt_boundary) {
        for (int i = 0; i < N_RANK; ++i) {
            slope_[i] = _slope[i];
//            dx_recursive_boundary_[i] = _slope[i];
            dx_recursive_boundary_[i] = tune_dx_boundary;
            ulb_boundary[i] = uub_boundary[i] = lub_boundary[i] = 0;
            // dx_recursive_boundary_[i] = 10;
        }
        for (int i = N_RANK-1; i > 0; --i)
            dx_recursive_[i] = tune_dx[N_RANK][i];
        dx_recursive_[0] = tune_dx[N_RANK][0];
        boundarySet = false;
        initialGridSet = false;
        slopeSet = true;
#if DEBUG
        N_CORES = 2;
#else
        N_CORES = __cilkrts_get_nworkers();
#endif
//        cout << " N_CORES = " << N_CORES << endl;

    }
#pragma isat marker M1_end

    /* README!!!: set_initial_grid()/set_stride() must be called before call to 
     * - walk_adaptive 
     * - walk_ncores_hybrid
     * - walk_ncores_boundary
     */
    void set_initial_grid(grid_info<N_RANK> const & grid);
    void set_stride(int const stride[]);
    void set_logic_size(int const phys_size[]);
    void set_slope(int const slope[]);
    inline bool touch_boundary(int i, int lt, grid_info<N_RANK> & grid);
    template <typename F> 
	inline void base_case_kernel_interior(int t0, int t1, grid_info<N_RANK> const grid, F const & f);
    template <typename BF> 
	inline void base_case_kernel_boundary(int t0, int t1, grid_info<N_RANK> const grid, BF const & bf);
    template <typename F> 
	inline void walk_serial(int t0, int t1, grid_info<N_RANK> const grid, F const & f);

    /* all recursion-based algorithm */
    template <typename F> 
    inline void walk_adaptive(int t0, int t1, grid_info<N_RANK> const grid, F const & f);
    template <typename F> 
    inline void walk_bicut(int t0, int t1, grid_info<N_RANK> const grid, F const & f);
    /* recursive algorithm for obase */
    template <typename F> 
    inline void obase_adaptive(int t0, int t1, grid_info<N_RANK> const grid, F const & f);
    template <typename F> 
    inline void obase_bicut(int t0, int t1, grid_info<N_RANK> const grid, F const & f);
    template <typename F, typename BF> 
    inline void walk_ncores_boundary_p(int t0, int t1, grid_info<N_RANK> const grid, F const & f, BF const & bf);
    template <typename F, typename BF> 
    inline void walk_bicut_boundary_p(int t0, int t1, grid_info<N_RANK> const grid, F const & f, BF const & bf);
    template <typename BF> 
    inline void obase_boundary_p(int t0, int t1, grid_info<N_RANK> const grid, BF const & bf);
    template <typename BF> 
    inline void obase_bicut_boundary_p(int t0, int t1, grid_info<N_RANK> const grid, BF const & bf);
    template <typename F, typename BF> 
    inline void obase_boundary_p(int t0, int t1, grid_info<N_RANK> const grid, F const & f, BF const & bf);
    template <typename F, typename BF> 
    inline void obase_bicut_boundary_p(int t0, int t1, grid_info<N_RANK> const grid, F const & f, BF const & bf);

    /* all loop-based algorithm */
    template <typename F> 
    inline void cut_time(algor_type algor, int t0, int t1, grid_info<N_RANK> const grid, F const & f);
    template <typename F> 
    inline void naive_cut_space_mp(int dim, int t0, int t1, grid_info<N_RANK> const grid, F const & f);
    template <typename F> 
    inline void naive_cut_space_ncores(int dim, int t0, int t1, grid_info<N_RANK> const grid, F const & f);
    template <typename F> 
    inline void cut_space_ncores_boundary(int dim, int t0, int t1, grid_info<N_RANK> const grid, F const & f);
#if DEBUG
	void print_grid(FILE * fp, int t0, int t1, grid_info<N_RANK> const & grid);
	void print_sync(FILE * fp);
	void print_index(int t, int const idx[]);
	void print_region(int t, int const head[], int const tail[]);
#endif
};

template <int N_RANK>
void Algorithm<N_RANK>::set_initial_grid(grid_info<N_RANK> const & grid)
{
    initial_grid_ = grid;
    for (int i = 0; i < N_RANK; ++i)
        initial_length_[i] = grid.x1[i] - grid.x0[i];
    initialGridSet = true;
    if (slopeSet) {
        /* set up the lb/ub_boundary */
        for (int i = 0; i < N_RANK; ++i) {
            ulb_boundary[i] = initial_grid_.x1[i] - slope_[i];
            uub_boundary[i] = initial_grid_.x1[i] + slope_[i];
            lub_boundary[i] = initial_grid_.x0[i] + slope_[i];
        }
    }
}

template <int N_RANK>
void Algorithm<N_RANK>::set_stride(int const stride[])
{
    for (int i = 0; i < N_RANK; ++i)
        stride_[i] = stride[i];
}

template <int N_RANK>
void Algorithm<N_RANK>::set_logic_size(int const logic_size[])
{
    for (int i = 0; i < N_RANK; ++i)
        logic_size_[i] = logic_size[i];
}

template <int N_RANK>
void Algorithm<N_RANK>::set_slope(int const slope[])
{
    for (int i = 0; i < N_RANK; ++i)
        slope_[i] = slope[i];
    slopeSet = true;
    if (initialGridSet) {
        /* set up the lb/ub_boundary */
        for (int i = 0; i < N_RANK; ++i) {
            ulb_boundary[i] = initial_grid_.x1[i] - slope_[i];
            uub_boundary[i] = initial_grid_.x1[i] + slope_[i];
            lub_boundary[i] = initial_grid_.x0[i] + slope_[i];
        }
    }
}

template <int N_RANK> template <typename F>
inline void Algorithm<N_RANK>::base_case_kernel_interior(int t0, int t1, grid_info<N_RANK> const grid, F const & f) {
	grid_info<N_RANK> l_grid = grid;
	for (int t = t0; t < t1; ++t) {
		/* execute one single time step */
		meta_grid_interior<N_RANK, F>::single_step(t, l_grid, initial_grid_, f);

		/* because the shape is trapezoid! */
		for (int i = 0; i < N_RANK; ++i) {
			l_grid.x0[i] += l_grid.dx0[i]; l_grid.x1[i] += l_grid.dx1[i];
		}
	}
}

template <int N_RANK> template <typename BF>
inline void Algorithm<N_RANK>::base_case_kernel_boundary(int t0, int t1, grid_info<N_RANK> const grid, BF const & bf) {
	grid_info<N_RANK> l_grid = grid;
	for (int t = t0; t < t1; ++t) {
		/* execute one single time step */
		meta_grid_boundary<N_RANK, BF>::single_step(t, l_grid, initial_grid_, bf);

		/* because the shape is trapezoid! */
		for (int i = 0; i < N_RANK; ++i) {
			l_grid.x0[i] += l_grid.dx0[i]; l_grid.x1[i] += l_grid.dx1[i];
		}
	}
}

#if DEBUG
template <int N_RANK>
void Algorithm<N_RANK>::print_grid(FILE *fp, int t0, int t1, grid_info<N_RANK> const & grid)
{
    int i;
    fprintf(fp, "{ BASE, ");
    fprintf(fp, "t = {%d, %d}, {", t0, t1);

    fprintf(fp, "x0 = {");
    for (i = 0; i < N_RANK-1; ++i) {
        /* print x0[3] */
        fprintf(fp, "%lu, ", grid.x0[i]);
    }
    fprintf(fp, "%lu}, ", grid.x0[i]);

    fprintf(fp, "x1 = {");
    for (i = 0; i < N_RANK-1; ++i) {
        /* print x1[3] */
        fprintf(fp, "%lu, ", grid.x1[i]);
    }
    fprintf(fp, "%lu}, ", grid.x1[i]);

    fprintf(fp, "dx0 = {");
    for (i = 0; i < N_RANK-1; ++i) {
        /* print dx0[3] */
        fprintf(fp, "%d, ", grid.dx0[i]);
    }
    fprintf(fp, "%d}, ", grid.dx0[i]);

    fprintf(fp, "dx1 = {");
    for (i = 0; i < N_RANK-1; ++i) {
        /* print dx1[3] */
        fprintf(fp, "%d, ", grid.dx1[i]);
    }
    fprintf(fp, "%d}}}, \n", grid.dx1[i]);
    fflush(fp);
    return;
}

template <int N_RANK>
void Algorithm<N_RANK>::print_sync(FILE * fp)
{
    int i;
    fprintf(fp, "{ SYNC, ");
    fprintf(fp, "t = {0, 0}, {");

    fprintf(fp, "x0 = {");
    for (i = 0; i < N_RANK-1; ++i) {
        /* print x0[3] */
        fprintf(fp, "0, ");
    }
    fprintf(fp, "0}, ");

    fprintf(fp, "x1 = {");
    for (i = 0; i < N_RANK-1; ++i) {
        /* print x1[3] */
        fprintf(fp, "0, ");
    }
    fprintf(fp, "0}, ");

    fprintf(fp, "dx0 = {");
    for (i = 0; i < N_RANK-1; ++i) {
        /* print dx0[3] */
        fprintf(fp, "0, ");
    }
    fprintf(fp, "0}, ");

    fprintf(fp, "dx1 = {");
    for (i = 0; i < N_RANK-1; ++i) {
        /* print dx1[3] */
        fprintf(fp, "0, ");
    }
    fprintf(fp, "0}}}, \n");
    fflush(fp);
    return;
}

template <int N_RANK>
void Algorithm<N_RANK>::print_index(int t, int const idx[])
{
    printf("U(t=%lu, {", t);
    for (int i = 0; i < N_RANK; ++i) {
        printf("%lu ", idx[i]);
    }
    printf("}) ");
    fflush(stdout);
}

template <int N_RANK>
void Algorithm<N_RANK>::print_region(int t, int const head[], int const tail[])
{
    printf("%s:%lu t=%lu, {", __FUNCTION__, __LINE__, t);
    for (int i = 0; i < N_RANK; ++i) {
        printf("{%lu, %lu} ", head[i], tail[i]);
    }
    printf("}\n");
    fflush(stdout);

}

#endif /* end if DEBUG */
#endif /* POCHOIR_WALK_H */
