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

#ifndef POCHOIR_WALK_RECURSIVE_HPP
#define POCHOIR_WALK_RECURSIVE_HPP

#include "pochoir_common.hpp"
#include "pochoir_walk.hpp"

#define initial_cut(i) (lb[i] == initial_length_[i])
/* grid.x1[i] >= initial_grid_.x1[i] - stride_[i] - slope_[i] 
 * because we compute the kernel with range [a, b)
 */
template <int N_RANK, typename Grid_info>
inline bool Algorithm<N_RANK, Grid_info>::touch_boundary(int i, int lt, Grid_info & grid) 
{
    bool interior = false;
    if (grid.x0[i] >= uub_boundary[i] 
     && grid.x0[i] + grid.dx0[i] * lt >= uub_boundary[i]) {
        interior = true;
        grid.x0[i] -= initial_length_[i];
        grid.x1[i] -= initial_length_[i];
    } else if (grid.x1[i] < ulb_boundary[i] 
            && grid.x1[i] + grid.dx1[i] * lt < ulb_boundary[i]
            && grid.x0[i] >= lub_boundary[i]
            && grid.x0[i] + grid.dx0[i] * lt >= lub_boundary[i]) {
        interior = true;
    } else {
        interior = false;
    }
    return !interior;
}

template <int N_RANK, typename Grid_info> template <typename F>
inline void Algorithm<N_RANK, Grid_info>::walk_serial(int t0, int t1, Grid_info const grid, F const & f)
{
    int lt = t1 - t0;
    bool base_cube = (lt <= dt_recursive_); /* dt_recursive_ : temporal dimension stop */
    bool cut_yet = false;
    bool can_cut[N_RANK];
    Grid_info l_grid;

    for (int i = 0; i < N_RANK; ++i) {
        can_cut[i] = (2 * (grid.x1[i] - grid.x0[i]) + (grid.dx1[i] - grid.dx0[i]) * lt >= 4 * slope_[i] * lt) && (grid.x1[i] - grid.x0[i] > dx_recursive_[i]);
        /* if all lb[i] < thres[i] && lt <= dt_recursive, 
           we have nothing to cut!
         */
        base_cube = base_cube && (!can_cut[i]);
    }

    if (base_cube) {
#if DEBUG
        print_grid(stdout, t0, t1, grid);
#endif
        base_case_kernel(t0, t1, grid, f);
        return;
    } else  {
		/* N_RANK-1 because we exclude the time dimension here */
        for (int i = N_RANK-1; i >= 0 && !cut_yet; --i) {
            if (can_cut[i]) {
                l_grid = grid;
                int xm = (2 * (grid.x0[i] + grid.x1[i]) + (2 * slope_[i] + grid.dx0[i] + grid.dx1[i]) * lt) / 4;
                l_grid.x0[i] = grid.x0[i]; l_grid.dx0[i] = grid.dx0[i];
                l_grid.x1[i] = xm; l_grid.dx1[i] = -slope_[i];
                walk_serial(t0, t1, l_grid, f);
                l_grid.x0[i] = xm; l_grid.dx0[i] = -slope_[i];
                l_grid.x1[i] = grid.x1[i]; l_grid.dx1[i] = grid.dx1[i];
                walk_serial(t0, t1, l_grid, f);
#if 0
                printf("%s:%d cut into %d dim\n", __FUNCTION__, __LINE__, i);
                fflush(stdout);
#endif
                cut_yet = true;
            }/* end if */
        } /* end for */
        if (!cut_yet && lt > dt_recursive_) {
            int halflt = lt / 2;
            l_grid = grid;
            walk_serial(t0, t0+halflt, l_grid, f);
#if DEBUG
            print_sync(stdout);
#endif

            for (int i = 0; i < N_RANK; ++i) {
                l_grid.x0[i] = grid.x0[i] + grid.dx0[i] * halflt;
                l_grid.dx0[i] = grid.dx0[i];
                l_grid.x1[i] = grid.x1[i] + grid.dx1[i] * halflt;
                l_grid.dx1[i] = grid.dx1[i];
            }
            walk_serial(t0+halflt, t1, l_grid, f);
#if 0
            printf("%s:%d cut into time dim\n", __FUNCTION__, __LINE__);
            fflush(stdout);
#endif
            cut_yet = true;
        }
        assert(cut_yet);
        return;
    }
}

/* walk_adaptive() is just for interior region */
template <int N_RANK, typename Grid_info> template <typename F>
inline void Algorithm<N_RANK, Grid_info>::walk_bicut(int t0, int t1, Grid_info const grid, F const & f)
{
	/* for the initial cut on each dimension, cut into exact N_CORES pieces,
	   for the rest cut into that dimension, cut into as many as we can!
	 */
	int lt = t1 - t0;
	index_info lb, thres;
	Grid_info l_grid;

	for (int i = 0; i < N_RANK; ++i) {
		lb[i] = grid.x1[i] - grid.x0[i];
		thres[i] = 2 * (2 * slope_[i] * lt);
	}	

	for (int i = N_RANK-1; i >= 0; --i) {
		if (lb[i] >= thres[i] && lb[i] > dx_recursive_[i]) { 
			l_grid = grid;
			const int sep = (int)lb[i]/2;
			const int r = 2;
#if DEBUG
//			printf("initial_cut = %s, lb[%d] = %d, sep = %d, r = %d\n", initial_cut(i) ? "True" : "False", i, lb[i], sep, r);
#endif
			l_grid.x0[i] = grid.x0[i];
			l_grid.dx0[i] = slope_[i];
			l_grid.x1[i] = grid.x0[i] + sep;
			l_grid.dx1[i] = -slope_[i];
			cilk_spawn walk_bicut(t0, t1, l_grid, f);

			l_grid.x0[i] = grid.x0[i] + sep;
			l_grid.dx0[i] = slope_[i];
			l_grid.x1[i] = grid.x1[i];
			l_grid.dx1[i] = -slope_[i];
			cilk_spawn walk_bicut(t0, t1, l_grid, f);
#if DEBUG
//			print_sync(stdout);
#endif
			cilk_sync;
			if (grid.dx0[i] != slope_[i]) {
				l_grid.x0[i] = grid.x0[i]; l_grid.dx0[i] = grid.dx0[i];
				l_grid.x1[i] = grid.x0[i]; l_grid.dx1[i] = slope_[i];
				cilk_spawn walk_bicut(t0, t1, l_grid, f);
			}

			l_grid.x0[i] = grid.x0[i] + sep;
			l_grid.dx0[i] = -slope_[i];
			l_grid.x1[i] = grid.x0[i] + sep;
			l_grid.dx1[i] = slope_[i];
			cilk_spawn walk_bicut(t0, t1, l_grid, f);

			if (grid.dx1[i] != -slope_[i]) {
				l_grid.x0[i] = grid.x1[i]; l_grid.dx0[i] = -slope_[i];
				l_grid.x1[i] = grid.x1[i]; l_grid.dx1[i] = grid.dx1[i];
				cilk_spawn walk_bicut(t0, t1, l_grid, f);
			}
#if DEBUG
			printf("%s:%d cut into %d dim\n", __FUNCTION__, __LINE__, i);
			fflush(stdout);
#endif
            return;
		}/* end if */
	} /* end for */
	if (lt > dt_recursive_) {
		int halflt = lt / 2;
		l_grid = grid;
		walk_bicut(t0, t0+halflt, l_grid, f);
#if DEBUG
//		print_sync(stdout);
#endif
		for (int i = 0; i < N_RANK; ++i) {
			l_grid.x0[i] = grid.x0[i] + grid.dx0[i] * halflt;
			l_grid.dx0[i] = grid.dx0[i];
			l_grid.x1[i] = grid.x1[i] + grid.dx1[i] * halflt;
			l_grid.dx1[i] = grid.dx1[i];
		}
		walk_bicut(t0+halflt, t1, l_grid, f);
#if DEBUG
//		printf("%s:%d cut into time dim\n", __FUNCTION__, __LINE__);
		fflush(stdout);
#endif
        return;
	}
    /* base case */
#if DEBUG
//    printf("call Adaptive! ");
//	  print_grid(stdout, t0, t1, grid);
#endif
	base_case_kernel(t0, t1, grid, f);
	return;
}

/* walk_adaptive() is just for interior region */
template <int N_RANK, typename Grid_info> template <typename F>
inline void Algorithm<N_RANK, Grid_info>::walk_adaptive(int t0, int t1, Grid_info const grid, F const & f)
{
	/* for the initial cut on each dimension, cut into exact N_CORES pieces,
	   for the rest cut into that dimension, cut into as many as we can!
	 */
	int lt = t1 - t0;
	bool base_cube = (lt <= dt_recursive_); /* dt_recursive_ : temporal dimension stop */
	bool cut_yet = false;
	//int lb[N_RANK];
	//int thres[N_RANK];
	index_info lb, thres;
	Grid_info l_grid;

	for (int i = 0; i < N_RANK; ++i) {
		lb[i] = grid.x1[i] - grid.x0[i];
		thres[i] = (initial_cut(i)) ? N_CORES * (2 * slope_[i] * lt) : 2 * (2 * slope_[i] * lt);
		base_cube = base_cube && (lb[i] <= dx_recursive_[i] || lb[i] < thres[i]); 
//		base_cube = base_cube && (lb[i] < thres[i]); 
	}	
	if (base_cube) {
#if DEBUG
        printf("call Adaptive! ");
		print_grid(stdout, t0, t1, grid);
#endif
		base_case_kernel(t0, t1, grid, f);
		return;
	} else  {
		for (int i = N_RANK-1; i >= 0 && !cut_yet; --i) {
			if (lb[i] >= thres[i] && lb[i] > dx_recursive_[i]) { 
//			if (lb[i] >= thres[i]) { 
				l_grid = grid;
				int sep = (initial_cut(i)) ? lb[i]/N_CORES : (2 * slope_[i] * lt);
				int r = (initial_cut(i)) ? N_CORES : (lb[i]/sep);
#if DEBUG
				printf("initial_cut = %s, lb[%d] = %d, sep = %d, r = %d\n", initial_cut(i) ? "True" : "False", i, lb[i], sep, r);
#endif
				int j;
				for (j = 0; j < r-1; ++j) {
					l_grid.x0[i] = grid.x0[i] + sep * j;
					l_grid.dx0[i] = slope_[i];
					l_grid.x1[i] = grid.x0[i] + sep * (j+1);
					l_grid.dx1[i] = -slope_[i];
					cilk_spawn walk_adaptive(t0, t1, l_grid, f);
				}
	//			j_loc = r-1;
				l_grid.x0[i] = grid.x0[i] + sep * (r-1);
				l_grid.dx0[i] = slope_[i];
				l_grid.x1[i] = grid.x1[i];
				l_grid.dx1[i] = -slope_[i];
				cilk_spawn walk_adaptive(t0, t1, l_grid, f);
#if DEBUG
//				print_sync(stdout);
#endif
				cilk_sync;
				if (grid.dx0[i] != slope_[i]) {
					l_grid.x0[i] = grid.x0[i]; l_grid.dx0[i] = grid.dx0[i];
					l_grid.x1[i] = grid.x0[i]; l_grid.dx1[i] = slope_[i];
					cilk_spawn walk_adaptive(t0, t1, l_grid, f);
				}
				for (int j = 1; j < r; ++j) {
					l_grid.x0[i] = grid.x0[i] + sep * j;
					l_grid.dx0[i] = -slope_[i];
					l_grid.x1[i] = grid.x0[i] + sep * j;
					l_grid.dx1[i] = slope_[i];
					cilk_spawn walk_adaptive(t0, t1, l_grid, f);
				}
				if (grid.dx1[i] != -slope_[i]) {
					l_grid.x0[i] = grid.x1[i]; l_grid.dx0[i] = -slope_[i];
					l_grid.x1[i] = grid.x1[i]; l_grid.dx1[i] = grid.dx1[i];
					cilk_spawn walk_adaptive(t0, t1, l_grid, f);
				}
#if 0
				printf("%s:%d cut into %d dim\n", __FUNCTION__, __LINE__, i);
				fflush(stdout);
#endif
				cut_yet = true;
			}/* end if */
		} /* end for */
		if (!cut_yet && lt > dt_recursive_) {
			int halflt = lt / 2;
			l_grid = grid;
			walk_adaptive(t0, t0+halflt, l_grid, f);
#if DEBUG
//			print_sync(stdout);
#endif
			for (int i = 0; i < N_RANK; ++i) {
				l_grid.x0[i] = grid.x0[i] + grid.dx0[i] * halflt;
				l_grid.dx0[i] = grid.dx0[i];
				l_grid.x1[i] = grid.x1[i] + grid.dx1[i] * halflt;
				l_grid.dx1[i] = grid.dx1[i];
			}
			walk_adaptive(t0+halflt, t1, l_grid, f);
#if 0
			printf("%s:%d cut into time dim\n", __FUNCTION__, __LINE__);
			fflush(stdout);
#endif
			cut_yet = true;
		}
		assert(cut_yet);
		return;
	}
}

#if DEBUG
static int count_boundary = 0;
static int count_internal = 0;
#endif

/* walk_ncores_boundary_p() will be called for -split-shadow mode */
template <int N_RANK, typename Grid_info> template <typename F, typename BF>
inline void Algorithm<N_RANK, Grid_info>::walk_bicut_boundary_p(int t0, int t1, Grid_info const grid, F const & f, BF const & bf)
{
	/* cut into exact N_CORES pieces */
	/* Indirect memory access is expensive */
	int lt = t1 - t0;
	bool base_cube = (lt <= dt_recursive_); /* dt_recursive_ : temporal dimension stop */
	bool can_cut = false, call_boundary = false;
	index_info lb, thres;
    Grid_info l_father_grid = grid, l_son_grid;
    bool l_touch_boundary[N_RANK];
    int l_dt_stop;

	for (int i = 0; i < N_RANK; ++i) {
        l_touch_boundary[i] = touch_boundary(i, lt, l_father_grid);
		lb[i] = (l_father_grid.x1[i] - l_father_grid.x0[i]);
		thres[i] = 2 * (2 * slope_[i] * lt);
		call_boundary |= l_touch_boundary[i];
	}	

	for (int i = N_RANK-1; i >= 0; --i) {
		can_cut = (l_touch_boundary[i]) ? (lb[i] >= thres[i] && lb[i] > dx_recursive_boundary_[i]) : (lb[i] >= thres[i] && lb[i] > dx_recursive_[i]);
		if (can_cut) { 
			l_son_grid = l_father_grid;
            int sep = (int)lb[i]/2;
            int r = 2;
			int l_start = (l_father_grid.x0[i]);
			int l_end = (l_father_grid.x1[i]);

			l_son_grid.x0[i] = l_start;
			l_son_grid.dx0[i] = slope_[i];
			l_son_grid.x1[i] = l_start + sep;
			l_son_grid.dx1[i] = -slope_[i];
            if (call_boundary) {
                cilk_spawn walk_bicut_boundary_p(t0, t1, l_son_grid, f, bf);
            } else {
                cilk_spawn walk_bicut(t0, t1, l_son_grid, f);
            }

			l_son_grid.x0[i] = l_start + sep;
			l_son_grid.dx0[i] = slope_[i];
			l_son_grid.x1[i] = l_end;
			l_son_grid.dx1[i] = -slope_[i];
            if (call_boundary) {
                walk_bicut_boundary_p(t0, t1, l_son_grid, f, bf);
            } else {
                walk_bicut(t0, t1, l_son_grid, f);
            }
#if DEBUG
			print_sync(stdout);
#endif
			cilk_sync;

			l_son_grid.x0[i] = l_start + sep;
			l_son_grid.dx0[i] = -slope_[i];
			l_son_grid.x1[i] = l_start + sep;
			l_son_grid.dx1[i] = slope_[i];
            if (call_boundary) {
                cilk_spawn walk_bicut_boundary_p(t0, t1, l_son_grid, f, bf);
            } else {
                cilk_spawn walk_bicut(t0, t1, l_son_grid, f);
            }

			if (l_start == initial_grid_.x0[i] && l_end == initial_grid_.x1[i]) {
        //        printf("merge triagles!\n");
				l_son_grid.x0[i] = l_end;
				l_son_grid.dx0[i] = -slope_[i];
				l_son_grid.x1[i] = l_end;
				l_son_grid.dx1[i] = slope_[i];
                if (call_boundary) {
                    cilk_spawn walk_bicut_boundary_p(t0, t1, l_son_grid, f, bf);
                } else {
                    cilk_spawn walk_bicut(t0, t1, l_son_grid, f);
                }
			} else {
				if (l_father_grid.dx0[i] != slope_[i]) {
					l_son_grid.x0[i] = l_start; 
					l_son_grid.dx0[i] = l_father_grid.dx0[i];
					l_son_grid.x1[i] = l_start; 
					l_son_grid.dx1[i] = slope_[i];
                    if (call_boundary) {
                        cilk_spawn walk_bicut_boundary_p(t0, t1, l_son_grid, f, bf);
                    } else {
                        cilk_spawn walk_bicut(t0, t1, l_son_grid, f);
                    }
				}
				if (l_father_grid.dx1[i] != -slope_[i]) {
					l_son_grid.x0[i] = l_end; 
					l_son_grid.dx0[i] = -slope_[i];
					l_son_grid.x1[i] = l_end; 
					l_son_grid.dx1[i] = l_father_grid.dx1[i];
                    if (call_boundary) {
                        cilk_spawn walk_bicut_boundary_p(t0, t1, l_son_grid, f, bf);
                    } else {
                        cilk_spawn walk_bicut(t0, t1, l_son_grid, f);
                    }
				}
			}
	        return;
		}/* end if */
	} /* end for */
    if (call_boundary)
        l_dt_stop = dt_recursive_boundary_;
    else
        l_dt_stop = dt_recursive_;
	if (lt > l_dt_stop) {
		int halflt = lt / 2;
		l_son_grid = l_father_grid;
        if (call_boundary) {
            walk_bicut_boundary_p(t0, t0+halflt, l_son_grid, f, bf);
        } else {
            walk_bicut(t0, t0+halflt, l_son_grid, f);
        }
#if DEBUG
		print_sync(stdout);
#endif
		for (int i = 0; i < N_RANK; ++i) {
			l_son_grid.x0[i] = l_father_grid.x0[i] + l_father_grid.dx0[i] * halflt;
			l_son_grid.dx0[i] = l_father_grid.dx0[i];
			l_son_grid.x1[i] = l_father_grid.x1[i] + l_father_grid.dx1[i] * halflt;
			l_son_grid.dx1[i] = l_father_grid.dx1[i];
		}
        if (call_boundary) { 
            walk_bicut_boundary_p(t0+halflt, t1, l_son_grid, f, bf);
        } else {
            walk_bicut(t0+halflt, t1, l_son_grid, f);
        }
	    return;
	} 
    // base cube
	if (call_boundary) {
        /* for periodic stencils, all elements falling into boundary region
         * requires special treatment of 'BF' (usually requires modulo operation
         * to wrap-up the index)
         */
#if DEBUG
        printf("call Boundary! ");
        print_grid(stdout, t0, t1, l_father_grid);
#endif
		base_case_kernel(t0, t1, l_father_grid, bf);
    } else {
#if DEBUG
        printf("call Interior! ");
	    print_grid(stdout, t0, t1, l_father_grid);
#endif
	    base_case_kernel(t0, t1, l_father_grid, f);
    }
    return;
}


/* walk_ncores_boundary_p() will be called for -split-shadow mode */
template <int N_RANK, typename Grid_info> template <typename F, typename BF>
inline void Algorithm<N_RANK, Grid_info>::walk_ncores_boundary_p(int t0, int t1, Grid_info const grid, F const & f, BF const & bf)
{
	/* cut into exact N_CORES pieces */
	/* Indirect memory access is expensive */
	int lt = t1 - t0;
	bool base_cube = (lt <= dt_recursive_); /* dt_recursive_ : temporal dimension stop */
	bool cut_yet = false, can_cut = false, call_boundary = false;
	index_info lb, thres;
    Grid_info l_father_grid = grid, l_son_grid;
    bool l_touch_boundary[N_RANK];

	for (int i = 0; i < N_RANK; ++i) {
        l_touch_boundary[i] = touch_boundary(i, lt, l_father_grid);
		lb[i] = (l_father_grid.x1[i] - l_father_grid.x0[i]);
		thres[i] = (initial_cut(i)) ?  N_CORES * (2 * slope_[i] * lt) : 2 * (2 * slope_[i] * lt);
		call_boundary |= l_touch_boundary[i];
		if (l_touch_boundary[i])
			base_cube = base_cube && (lb[i] <= dx_recursive_boundary_[i] || lb[i] < thres[i]); 
		else 
			base_cube = base_cube && (lb[i] <= dx_recursive_[i] || lb[i] < thres[i]); 
	}	

	if (base_cube) {
		if (call_boundary) {
            /* for periodic stencils, all elements falling into boundary region
             * requires special treatment of 'BF' (usually requires modulo operation
             * to wrap-up the index)
             */
#if DEBUG
	        printf("call Boundary! ");
            print_grid(stdout, t0, t1, l_father_grid);
#endif
			base_case_kernel(t0, t1, l_father_grid, bf);
        } else {
#if DEBUG
            printf("call Interior! ");
	    	print_grid(stdout, t0, t1, l_father_grid);
#endif
			base_case_kernel(t0, t1, l_father_grid, f);
        }
		return;
	} else  {
		for (int i = N_RANK-1; i >= 0 && !cut_yet; --i) {
			can_cut = (l_touch_boundary[i]) ? (lb[i] >= thres[i] && lb[i] > dx_recursive_boundary_[i]) : (lb[i] >= thres[i] && lb[i] > dx_recursive_[i]);
			if (can_cut) { 
				l_son_grid = l_father_grid;
                int sep = (initial_cut(i)) ? lb[i]/N_CORES : (2 * slope_[i] * lt);
                int r = (initial_cut(i)) ? N_CORES : (lb[i]/sep);
				int l_start = (l_father_grid.x0[i]);
				int l_end = (l_father_grid.x1[i]);
				int j;
				for (j = 0; j < r-1; ++j) {
					l_son_grid.x0[i] = l_start + sep * j;
					l_son_grid.dx0[i] = slope_[i];
					l_son_grid.x1[i] = l_start + sep * (j+1);
					l_son_grid.dx1[i] = -slope_[i];
                    if (call_boundary) {
                        cilk_spawn walk_ncores_boundary_p(t0, t1, l_son_grid, f, bf);
                    } else {
                        cilk_spawn walk_adaptive(t0, t1, l_son_grid, f);
                    }
				}
				l_son_grid.x0[i] = l_start + sep * j;
				l_son_grid.dx0[i] = slope_[i];
				l_son_grid.x1[i] = l_end;
				l_son_grid.dx1[i] = -slope_[i];
                if (call_boundary) {
                    walk_ncores_boundary_p(t0, t1, l_son_grid, f, bf);
                } else {
                    walk_adaptive(t0, t1, l_son_grid, f);
                }
#if DEBUG
//				print_sync(stdout);
#endif
				cilk_sync;
				for (j = 1; j < r; ++j) {
					l_son_grid.x0[i] = l_start + sep * j;
					l_son_grid.dx0[i] = -slope_[i];
					l_son_grid.x1[i] = l_start + sep * j;
					l_son_grid.dx1[i] = slope_[i];
                    if (call_boundary) {
                        cilk_spawn walk_ncores_boundary_p(t0, t1, l_son_grid, f, bf);
                    } else {
                        cilk_spawn walk_adaptive(t0, t1, l_son_grid, f);
                    }
				}
				if (l_start == initial_grid_.x0[i] && l_end == initial_grid_.x1[i]) {
            //        printf("merge triagles!\n");
					l_son_grid.x0[i] = l_end;
					l_son_grid.dx0[i] = -slope_[i];
					l_son_grid.x1[i] = l_end;
					l_son_grid.dx1[i] = slope_[i];
                    if (call_boundary) {
                        cilk_spawn walk_ncores_boundary_p(t0, t1, l_son_grid, f, bf);
                    } else {
                        cilk_spawn walk_adaptive(t0, t1, l_son_grid, f);
                    }
				} else {
					if (l_father_grid.dx0[i] != slope_[i]) {
						l_son_grid.x0[i] = l_start; 
						l_son_grid.dx0[i] = l_father_grid.dx0[i];
						l_son_grid.x1[i] = l_start; 
						l_son_grid.dx1[i] = slope_[i];
                        if (call_boundary) {
                            cilk_spawn walk_ncores_boundary_p(t0, t1, l_son_grid, f, bf);
                        } else {
                            cilk_spawn walk_adaptive(t0, t1, l_son_grid, f);
                        }
					}
					if (l_father_grid.dx1[i] != -slope_[i]) {
						l_son_grid.x0[i] = l_end; 
						l_son_grid.dx0[i] = -slope_[i];
						l_son_grid.x1[i] = l_end; 
						l_son_grid.dx1[i] = l_father_grid.dx1[i];
                        if (call_boundary) {
                            cilk_spawn walk_ncores_boundary_p(t0, t1, l_son_grid, f, bf);
                        } else {
                            cilk_spawn walk_adaptive(t0, t1, l_son_grid, f);
                        }
					}
				}
				cut_yet = true;
			}/* end if */
		} /* end for */
		if (!cut_yet && lt > dt_recursive_) {
			int halflt = lt / 2;
			l_son_grid = l_father_grid;
            if (call_boundary) {
                walk_ncores_boundary_p(t0, t0+halflt, l_son_grid, f, bf);
            } else {
                walk_adaptive(t0, t0+halflt, l_son_grid, f);
            }
#if DEBUG
//			print_sync(stdout);
#endif
			for (int i = 0; i < N_RANK; ++i) {
				l_son_grid.x0[i] = l_father_grid.x0[i] + l_father_grid.dx0[i] * halflt;
				l_son_grid.dx0[i] = l_father_grid.dx0[i];
				l_son_grid.x1[i] = l_father_grid.x1[i] + l_father_grid.dx1[i] * halflt;
				l_son_grid.dx1[i] = l_father_grid.dx1[i];
			}
            if (call_boundary) { 
                walk_ncores_boundary_p(t0+halflt, t1, l_son_grid, f, bf);
            } else {
                walk_adaptive(t0+halflt, t1, l_son_grid, f);
            }
			cut_yet = true;
		}
		assert(cut_yet);
		return;
	}
}

/* this is for interior region */
template <int N_RANK, typename Grid_info> template <typename F>
inline void Algorithm<N_RANK, Grid_info>::obase_bicut(int t0, int t1, Grid_info const grid, F const & f)
{
	/* for the initial cut on each dimension, cut into exact N_CORES pieces,
	   for the rest cut into that dimension, cut into as many as we can!
	 */
	int lt = t1 - t0;
	index_info lb, thres;
	Grid_info l_grid;

	for (int i = 0; i < N_RANK; ++i) {
		lb[i] = grid.x1[i] - grid.x0[i];
		thres[i] = 2 * (2 * slope_[i] * lt);
	}	
	for (int i = N_RANK-1; i >= 0; --i) {
		if (lb[i] >= thres[i] && lb[i] > dx_recursive_[i]) { 
			l_grid = grid;
			int sep = (int)lb[i]/2;
			int r = 2;
#if DEBUG
			printf("initial_cut = %s, lb[%d] = %d, sep = %d, r = %d\n", initial_cut(i) ? "True" : "False", i, lb[i], sep, r);
#endif
			l_grid.x0[i] = grid.x0[i];
			l_grid.dx0[i] = slope_[i];
			l_grid.x1[i] = grid.x0[i] + sep;
			l_grid.dx1[i] = -slope_[i];
			cilk_spawn obase_bicut(t0, t1, l_grid, f);

			l_grid.x0[i] = grid.x0[i] + sep;
			l_grid.dx0[i] = slope_[i];
			l_grid.x1[i] = grid.x1[i];
			l_grid.dx1[i] = -slope_[i];
			cilk_spawn obase_bicut(t0, t1, l_grid, f);
#if DEBUG
//			print_sync(stdout);
#endif
			cilk_sync;
			if (grid.dx0[i] != slope_[i]) {
				l_grid.x0[i] = grid.x0[i]; l_grid.dx0[i] = grid.dx0[i];
				l_grid.x1[i] = grid.x0[i]; l_grid.dx1[i] = slope_[i];
				cilk_spawn obase_bicut(t0, t1, l_grid, f);
			}

			l_grid.x0[i] = grid.x0[i] + sep;
			l_grid.dx0[i] = -slope_[i];
			l_grid.x1[i] = grid.x0[i] + sep;
			l_grid.dx1[i] = slope_[i];
			cilk_spawn obase_bicut(t0, t1, l_grid, f);

			if (grid.dx1[i] != -slope_[i]) {
				l_grid.x0[i] = grid.x1[i]; l_grid.dx0[i] = -slope_[i];
				l_grid.x1[i] = grid.x1[i]; l_grid.dx1[i] = grid.dx1[i];
				cilk_spawn obase_bicut(t0, t1, l_grid, f);
			}
#if DEBUG
			printf("%s:%d cut into %d dim\n", __FUNCTION__, __LINE__, i);
			fflush(stdout);
#endif
            return;
		}/* end if */
	} /* end for */
	if (lt > dt_recursive_) {
		int halflt = lt / 2;
		l_grid = grid;
		obase_bicut(t0, t0+halflt, l_grid, f);
#if DEBUG
//		print_sync(stdout);
#endif
		for (int i = 0; i < N_RANK; ++i) {
			l_grid.x0[i] = grid.x0[i] + grid.dx0[i] * halflt;
			l_grid.dx0[i] = grid.dx0[i];
			l_grid.x1[i] = grid.x1[i] + grid.dx1[i] * halflt;
			l_grid.dx1[i] = grid.dx1[i];
		}
		obase_bicut(t0+halflt, t1, l_grid, f);
#if DEBUG 
		printf("%s:%d cut into time dim\n", __FUNCTION__, __LINE__);
		fflush(stdout);
#endif
        return;
	}
#if DEBUG
    printf("call obase_bicut! ");
    print_grid(stdout, t0, t1, grid);
#endif
	f(t0, t1, grid);
	return;
}


/* this is for interior region */
template <int N_RANK, typename Grid_info> template <typename F>
inline void Algorithm<N_RANK, Grid_info>::obase_adaptive(int t0, int t1, Grid_info const grid, F const & f)
{
	/* for the initial cut on each dimension, cut into exact N_CORES pieces,
	   for the rest cut into that dimension, cut into as many as we can!
	 */
	int lt = t1 - t0;
	bool base_cube = (lt <= dt_recursive_); /* dt_recursive_ : temporal dimension stop */
	bool cut_yet = false;
	//int lb[N_RANK];
	//int thres[N_RANK];
	index_info lb, thres;
	Grid_info l_grid;

	for (int i = 0; i < N_RANK; ++i) {
		lb[i] = grid.x1[i] - grid.x0[i];
		thres[i] = (initial_cut(i)) ? N_CORES * (2 * slope_[i] * lt) : 2 * (2 * slope_[i] * lt);
		base_cube = base_cube && (lb[i] <= dx_recursive_[i] || lb[i] < thres[i]); 
	}	
	if (base_cube) {
#if DEBUG
        printf("call Adaptive! ");
		print_grid(stdout, t0, t1, grid);
#endif
		f(t0, t1, grid);
		return;
	} else  {
		for (int i = N_RANK-1; i >= 0 && !cut_yet; --i) {
			if (lb[i] >= thres[i] && lb[i] > dx_recursive_[i]) { 
				l_grid = grid;
				int sep = (initial_cut(i)) ? lb[i]/N_CORES : (2 * slope_[i] * lt);
				int r = (initial_cut(i)) ? N_CORES : (lb[i]/sep);
#if DEBUG
				printf("initial_cut = %s, lb[%d] = %d, sep = %d, r = %d\n", initial_cut(i) ? "True" : "False", i, lb[i], sep, r);
#endif
				int j;
				for (j = 0; j < r-1; ++j) {
					l_grid.x0[i] = grid.x0[i] + sep * j;
					l_grid.dx0[i] = slope_[i];
					l_grid.x1[i] = grid.x0[i] + sep * (j+1);
					l_grid.dx1[i] = -slope_[i];
					cilk_spawn obase_adaptive(t0, t1, l_grid, f);
				}
	//			j_loc = r-1;
				l_grid.x0[i] = grid.x0[i] + sep * (r-1);
				l_grid.dx0[i] = slope_[i];
				l_grid.x1[i] = grid.x1[i];
				l_grid.dx1[i] = -slope_[i];
				cilk_spawn obase_adaptive(t0, t1, l_grid, f);
#if DEBUG
//				print_sync(stdout);
#endif
				cilk_sync;
				if (grid.dx0[i] != slope_[i]) {
					l_grid.x0[i] = grid.x0[i]; l_grid.dx0[i] = grid.dx0[i];
					l_grid.x1[i] = grid.x0[i]; l_grid.dx1[i] = slope_[i];
					cilk_spawn obase_adaptive(t0, t1, l_grid, f);
				}
				for (int j = 1; j < r; ++j) {
					l_grid.x0[i] = grid.x0[i] + sep * j;
					l_grid.dx0[i] = -slope_[i];
					l_grid.x1[i] = grid.x0[i] + sep * j;
					l_grid.dx1[i] = slope_[i];
					cilk_spawn obase_adaptive(t0, t1, l_grid, f);
				}
				if (grid.dx1[i] != -slope_[i]) {
					l_grid.x0[i] = grid.x1[i]; l_grid.dx0[i] = -slope_[i];
					l_grid.x1[i] = grid.x1[i]; l_grid.dx1[i] = grid.dx1[i];
					cilk_spawn obase_adaptive(t0, t1, l_grid, f);
				}
#if 0
				printf("%s:%d cut into %d dim\n", __FUNCTION__, __LINE__, i);
				fflush(stdout);
#endif
				cut_yet = true;
			}/* end if */
		} /* end for */
		if (!cut_yet && lt > dt_recursive_) {
			int halflt = lt / 2;
			l_grid = grid;
			obase_adaptive(t0, t0+halflt, l_grid, f);
#if DEBUG
//			print_sync(stdout);
#endif
			for (int i = 0; i < N_RANK; ++i) {
				l_grid.x0[i] = grid.x0[i] + grid.dx0[i] * halflt;
				l_grid.dx0[i] = grid.dx0[i];
				l_grid.x1[i] = grid.x1[i] + grid.dx1[i] * halflt;
				l_grid.dx1[i] = grid.dx1[i];
			}
			obase_adaptive(t0+halflt, t1, l_grid, f);
#if 0
			printf("%s:%d cut into time dim\n", __FUNCTION__, __LINE__);
			fflush(stdout);
#endif
			cut_yet = true;
		}
		assert(cut_yet);
		return;
	}
}

/* this is the version for executable spec!!! */
template <int N_RANK, typename Grid_info> template <typename BF>
inline void Algorithm<N_RANK, Grid_info>::obase_bicut_boundary_p(int t0, int t1, Grid_info const grid, BF const & bf)
{
	/* cut into exact N_CORES pieces */
	/* Indirect memory access is expensive */
	int lt = t1 - t0;
	bool can_cut = false, call_boundary = false;
	index_info lb, thres;
    Grid_info l_father_grid = grid, l_son_grid;
    bool l_touch_boundary[N_RANK];

	for (int i = 0; i < N_RANK; ++i) {
        l_touch_boundary[i] = touch_boundary(i, lt, l_father_grid);
		lb[i] = (l_father_grid.x1[i] - l_father_grid.x0[i]);
		thres[i] = 2 * (2 * slope_[i] * lt);
		call_boundary |= l_touch_boundary[i];
	}	

	for (int i = N_RANK-1; i >= 0; --i) {
		can_cut = (l_touch_boundary[i]) ? (lb[i] >= thres[i] && lb[i] > dx_recursive_boundary_[i]) : (lb[i] >= thres[i] && lb[i] > dx_recursive_[i]);
		if (can_cut) { 
			l_son_grid = l_father_grid;
            int sep = (int)lb[i]/2;
            int r = 2;
			int l_start = (l_father_grid.x0[i]);
			int l_end = (l_father_grid.x1[i]);
			int j;

			l_son_grid.x0[i] = l_start;
			l_son_grid.dx0[i] = slope_[i];
			l_son_grid.x1[i] = l_start + sep;
			l_son_grid.dx1[i] = -slope_[i];
            cilk_spawn obase_bicut_boundary_p(t0, t1, l_son_grid, bf);

			l_son_grid.x0[i] = l_start + sep * j;
			l_son_grid.dx0[i] = slope_[i];
			l_son_grid.x1[i] = l_end;
			l_son_grid.dx1[i] = -slope_[i];
            obase_bicut_boundary_p(t0, t1, l_son_grid, bf);
#if DEBUG
//			print_sync(stdout);
#endif
			cilk_sync;
			l_son_grid.x0[i] = l_start + sep;
			l_son_grid.dx0[i] = -slope_[i];
			l_son_grid.x1[i] = l_start + sep;
			l_son_grid.dx1[i] = slope_[i];
            cilk_spawn obase_bicut_boundary_p(t0, t1, l_son_grid, bf);
			if (l_start == initial_grid_.x0[i] && l_end == initial_grid_.x1[i]) {
        //        printf("merge triagles!\n");
				l_son_grid.x0[i] = l_end;
				l_son_grid.dx0[i] = -slope_[i];
				l_son_grid.x1[i] = l_end;
				l_son_grid.dx1[i] = slope_[i];
                cilk_spawn obase_bicut_boundary_p(t0, t1, l_son_grid, bf);
			} else {
				if (l_father_grid.dx0[i] != slope_[i]) {
					l_son_grid.x0[i] = l_start; 
					l_son_grid.dx0[i] = l_father_grid.dx0[i];
					l_son_grid.x1[i] = l_start; 
					l_son_grid.dx1[i] = slope_[i];
                    cilk_spawn obase_bicut_boundary_p(t0, t1, l_son_grid, bf);
				}
				if (l_father_grid.dx1[i] != -slope_[i]) {
					l_son_grid.x0[i] = l_end; 
					l_son_grid.dx0[i] = -slope_[i];
					l_son_grid.x1[i] = l_end; 
					l_son_grid.dx1[i] = l_father_grid.dx1[i];
                    cilk_spawn obase_bicut_boundary_p(t0, t1, l_son_grid, bf);
				}
			}
            return;
		}/* end if */
	} /* end for */
	if (lt > dt_recursive_) {
		int halflt = lt / 2;
		l_son_grid = l_father_grid;
        obase_bicut_boundary_p(t0, t0+halflt, l_son_grid, bf);
#if DEBUG
//		print_sync(stdout);
#endif
		for (int i = 0; i < N_RANK; ++i) {
			l_son_grid.x0[i] = l_father_grid.x0[i] + l_father_grid.dx0[i] * halflt;
			l_son_grid.dx0[i] = l_father_grid.dx0[i];
			l_son_grid.x1[i] = l_father_grid.x1[i] + l_father_grid.dx1[i] * halflt;
			l_son_grid.dx1[i] = l_father_grid.dx1[i];
		}
        obase_bicut_boundary_p(t0+halflt, t1, l_son_grid, bf);
        return;
	}
	base_case_kernel(t0, t1, l_father_grid, bf);
	return;
}


/* this is the version for executable spec!!! */
template <int N_RANK, typename Grid_info> template <typename BF>
inline void Algorithm<N_RANK, Grid_info>::obase_boundary_p(int t0, int t1, Grid_info const grid, BF const & bf)
{
	/* cut into exact N_CORES pieces */
	/* Indirect memory access is expensive */
	int lt = t1 - t0;
	bool base_cube = (lt <= dt_recursive_); /* dt_recursive_ : temporal dimension stop */
	bool cut_yet = false, can_cut = false, call_boundary = false;
	index_info lb, thres;
    Grid_info l_father_grid = grid, l_son_grid;
    bool l_touch_boundary[N_RANK];

	for (int i = 0; i < N_RANK; ++i) {
        l_touch_boundary[i] = touch_boundary(i, lt, l_father_grid);
		lb[i] = (l_father_grid.x1[i] - l_father_grid.x0[i]);
		thres[i] = (initial_cut(i)) ?  N_CORES * (2 * slope_[i] * lt) : 2 * (2 * slope_[i] * lt);
		if (l_touch_boundary[i])
			base_cube = base_cube && (lb[i] <= dx_recursive_boundary_[i] || lb[i] < thres[i]); 
		else 
			base_cube = base_cube && (lb[i] <= dx_recursive_[i] || lb[i] < thres[i]); 
		call_boundary |= l_touch_boundary[i];
	}	

	if (base_cube) {
		base_case_kernel(t0, t1, l_father_grid, bf);
		return;
	} else  {
		for (int i = N_RANK-1; i >= 0 && !cut_yet; --i) {
			can_cut = (l_touch_boundary[i]) ? (lb[i] >= thres[i] && lb[i] > dx_recursive_boundary_[i]) : (lb[i] >= thres[i] && lb[i] > dx_recursive_[i]);
			if (can_cut) { 
				l_son_grid = l_father_grid;
                int sep = (initial_cut(i)) ? lb[i]/N_CORES : (2 * slope_[i] * lt);
                //int r = (initial_cut(i)) ? N_CORES : (lb[i]/sep);
                int r = lb[i]/sep;
				int l_start = (l_father_grid.x0[i]);
				int l_end = (l_father_grid.x1[i]);
				int j;
				for (j = 0; j < r-1; ++j) {
					l_son_grid.x0[i] = l_start + sep * j;
					l_son_grid.dx0[i] = slope_[i];
					l_son_grid.x1[i] = l_start + sep * (j+1);
					l_son_grid.dx1[i] = -slope_[i];
                    cilk_spawn obase_boundary_p(t0, t1, l_son_grid, bf);
				}
				l_son_grid.x0[i] = l_start + sep * j;
				l_son_grid.dx0[i] = slope_[i];
				l_son_grid.x1[i] = l_end;
				l_son_grid.dx1[i] = -slope_[i];
                obase_boundary_p(t0, t1, l_son_grid, bf);
#if DEBUG
//				print_sync(stdout);
#endif
				cilk_sync;
				for (j = 1; j < r; ++j) {
					l_son_grid.x0[i] = l_start + sep * j;
					l_son_grid.dx0[i] = -slope_[i];
					l_son_grid.x1[i] = l_start + sep * j;
					l_son_grid.dx1[i] = slope_[i];
                    cilk_spawn obase_boundary_p(t0, t1, l_son_grid, bf);
				}
				if (l_start == initial_grid_.x0[i] && l_end == initial_grid_.x1[i]) {
            //        printf("merge triagles!\n");
					l_son_grid.x0[i] = l_end;
					l_son_grid.dx0[i] = -slope_[i];
					l_son_grid.x1[i] = l_end;
					l_son_grid.dx1[i] = slope_[i];
                    cilk_spawn obase_boundary_p(t0, t1, l_son_grid, bf);
				} else {
					if (l_father_grid.dx0[i] != slope_[i]) {
						l_son_grid.x0[i] = l_start; 
						l_son_grid.dx0[i] = l_father_grid.dx0[i];
						l_son_grid.x1[i] = l_start; 
						l_son_grid.dx1[i] = slope_[i];
                        cilk_spawn obase_boundary_p(t0, t1, l_son_grid, bf);
					}
					if (l_father_grid.dx1[i] != -slope_[i]) {
						l_son_grid.x0[i] = l_end; 
						l_son_grid.dx0[i] = -slope_[i];
						l_son_grid.x1[i] = l_end; 
						l_son_grid.dx1[i] = l_father_grid.dx1[i];
                        cilk_spawn obase_boundary_p(t0, t1, l_son_grid, bf);
					}
				}
				cut_yet = true;
			}/* end if */
		} /* end for */
		if (!cut_yet && lt > dt_recursive_) {
			int halflt = lt / 2;
			l_son_grid = l_father_grid;
            obase_boundary_p(t0, t0+halflt, l_son_grid, bf);
#if DEBUG
//			print_sync(stdout);
#endif
			for (int i = 0; i < N_RANK; ++i) {
				l_son_grid.x0[i] = l_father_grid.x0[i] + l_father_grid.dx0[i] * halflt;
				l_son_grid.dx0[i] = l_father_grid.dx0[i];
				l_son_grid.x1[i] = l_father_grid.x1[i] + l_father_grid.dx1[i] * halflt;
				l_son_grid.dx1[i] = l_father_grid.dx1[i];
			}
            obase_boundary_p(t0+halflt, t1, l_son_grid, bf);
			cut_yet = true;
		}
		assert(cut_yet);
		return;
	}
}

/* this is for optimizing base case!!! */
template <int N_RANK, typename Grid_info> template <typename F, typename BF>
inline void Algorithm<N_RANK, Grid_info>::obase_bicut_boundary_p(int t0, int t1, Grid_info const grid, F const & f, BF const & bf)
{
	/* cut into exact N_CORES pieces */
	/* Indirect memory access is expensive */
	int lt = t1 - t0;
	bool can_cut = false, call_boundary = false;
	index_info lb, thres;
    Grid_info l_father_grid = grid, l_son_grid;
    bool l_touch_boundary[N_RANK];
    int l_dt_stop;

	for (int i = 0; i < N_RANK; ++i) {
        l_touch_boundary[i] = touch_boundary(i, lt, l_father_grid);
		lb[i] = (l_father_grid.x1[i] - l_father_grid.x0[i]);
		thres[i] = 2 * (2 * slope_[i] * lt);
		call_boundary |= l_touch_boundary[i];
	}	

	for (int i = N_RANK-1; i >= 0; --i) {
		can_cut = (l_touch_boundary[i]) ? (lb[i] >= thres[i] && lb[i] > dx_recursive_boundary_[i]) : (lb[i] >= thres[i] && lb[i] > dx_recursive_[i]);
		if (can_cut) { 
            l_son_grid = l_father_grid;
            int sep = lb[i]/2;
            int r = 2;
			int l_start = (l_father_grid.x0[i]);
			int l_end = (l_father_grid.x1[i]);

			l_son_grid.x0[i] = l_start;
			l_son_grid.dx0[i] = slope_[i];
			l_son_grid.x1[i] = l_start + sep;
			l_son_grid.dx1[i] = -slope_[i];
            if (call_boundary) {
                cilk_spawn obase_bicut_boundary_p(t0, t1, l_son_grid, f, bf);
            } else {
                cilk_spawn obase_bicut(t0, t1, l_son_grid, f);
            }

			l_son_grid.x0[i] = l_start + sep;
			l_son_grid.dx0[i] = slope_[i];
			l_son_grid.x1[i] = l_end;
			l_son_grid.dx1[i] = -slope_[i];
            if (call_boundary) {
                obase_bicut_boundary_p(t0, t1, l_son_grid, f, bf);
            } else {
                obase_bicut(t0, t1, l_son_grid, f);
            }
			cilk_sync;

			l_son_grid.x0[i] = l_start + sep;
			l_son_grid.dx0[i] = -slope_[i];
			l_son_grid.x1[i] = l_start + sep;
			l_son_grid.dx1[i] = slope_[i];
            if (call_boundary) {
                cilk_spawn obase_bicut_boundary_p(t0, t1, l_son_grid, f, bf);
            } else {
                cilk_spawn obase_bicut(t0, t1, l_son_grid, f);
            }

			if (l_start == initial_grid_.x0[i] && l_end == initial_grid_.x1[i]) {
        //        printf("merge triagles!\n");
				l_son_grid.x0[i] = l_end;
				l_son_grid.dx0[i] = -slope_[i];
				l_son_grid.x1[i] = l_end;
				l_son_grid.dx1[i] = slope_[i];
                if (call_boundary) {
                    cilk_spawn obase_bicut_boundary_p(t0, t1, l_son_grid, f, bf);
                } else {
                    cilk_spawn obase_bicut(t0, t1, l_son_grid, f);
                }
			} else {
				if (l_father_grid.dx0[i] != slope_[i]) {
					l_son_grid.x0[i] = l_start; 
					l_son_grid.dx0[i] = l_father_grid.dx0[i];
					l_son_grid.x1[i] = l_start; 
					l_son_grid.dx1[i] = slope_[i];
                    if (call_boundary) {
                        cilk_spawn obase_bicut_boundary_p(t0, t1, l_son_grid, f, bf);
                    } else {
                        cilk_spawn obase_bicut(t0, t1, l_son_grid, f);
                    }
				}
				if (l_father_grid.dx1[i] != -slope_[i]) {
					l_son_grid.x0[i] = l_end; 
					l_son_grid.dx0[i] = -slope_[i];
					l_son_grid.x1[i] = l_end; 
					l_son_grid.dx1[i] = l_father_grid.dx1[i];
                    if (call_boundary) {
                        cilk_spawn obase_bicut_boundary_p(t0, t1, l_son_grid, f, bf);
                    } else {
                        cilk_spawn obase_bicut(t0, t1, l_son_grid, f);
                    }
				}
			}
            return;
		}/* end if */
	} /* end for */    
    if (call_boundary)
        l_dt_stop = dt_recursive_boundary_;
    else
        l_dt_stop = dt_recursive_;

	if (lt > l_dt_stop) {
		int halflt = lt / 2;
		l_son_grid = l_father_grid;
        if (call_boundary) {
            obase_bicut_boundary_p(t0, t0+halflt, l_son_grid, f, bf);
        } else {
            obase_bicut(t0, t0+halflt, l_son_grid, f);
        }
		for (int i = 0; i < N_RANK; ++i) {
			l_son_grid.x0[i] = l_father_grid.x0[i] + l_father_grid.dx0[i] * halflt;
			l_son_grid.dx0[i] = l_father_grid.dx0[i];
			l_son_grid.x1[i] = l_father_grid.x1[i] + l_father_grid.dx1[i] * halflt;
			l_son_grid.dx1[i] = l_father_grid.dx1[i];
		}
        if (call_boundary) { 
            obase_bicut_boundary_p(t0+halflt, t1, l_son_grid, f, bf);
        } else {
            obase_bicut(t0+halflt, t1, l_son_grid, f);
        }
        return;
	}
	if (call_boundary) {
        /* for periodic stencils, all elements falling within boundary region
         * requires special treatment 'BF' (usually requires modulo operation)
        */
#if DEBUG
	    printf("call Boundary! ");
        print_grid(stdout, t0, t1, l_father_grid);
#endif
		//bf(t0, t1, grid);
		base_case_kernel(t0, t1, l_father_grid, bf);
    } else {
#if DEBUG
        printf("call Interior! ");
		print_grid(stdout, t0, t1, l_father_grid);
#endif
		f(t0, t1, l_father_grid);
    }
	return;
}

/* this is for optimizing base case!!! */
template <int N_RANK, typename Grid_info> template <typename F, typename BF>
inline void Algorithm<N_RANK, Grid_info>::obase_boundary_p(int t0, int t1, Grid_info const grid, F const & f, BF const & bf)
{
	/* cut into exact N_CORES pieces */
	/* Indirect memory access is expensive */
	int lt = t1 - t0;
	bool base_cube = (lt <= dt_recursive_); /* dt_recursive_ : temporal dimension stop */
	bool cut_yet = false, can_cut = false, call_boundary = false;
	index_info lb, thres;
    Grid_info l_father_grid = grid, l_son_grid;
    bool l_touch_boundary[N_RANK];

	for (int i = 0; i < N_RANK; ++i) {
        l_touch_boundary[i] = touch_boundary(i, lt, l_father_grid);
		lb[i] = (l_father_grid.x1[i] - l_father_grid.x0[i]);
		thres[i] = (initial_cut(i)) ?  N_CORES * (2 * slope_[i] * lt) : 2 * (2 * slope_[i] * lt);
		if (l_touch_boundary[i])
			base_cube = base_cube && (lb[i] <= dx_recursive_boundary_[i] || lb[i] < thres[i]); 
		else 
			base_cube = base_cube && (lb[i] <= dx_recursive_[i] || lb[i] < thres[i]); 
		call_boundary |= l_touch_boundary[i];
	}	

	if (base_cube) {
		if (call_boundary) {
            /* for periodic stencils, all elements falling within boundary region
             * requires special treatment 'BF' (usually requires modulo operation)
            */
#if DEBUG
	        printf("call Boundary! ");
            print_grid(stdout, t0, t1, l_father_grid);
#endif
			//bf(t0, t1, grid);
			base_case_kernel(t0, t1, l_father_grid, bf);
        } else {
#if DEBUG
            printf("call Interior! ");
	    	print_grid(stdout, t0, t1, l_father_grid);
#endif
			f(t0, t1, l_father_grid);
        }
		return;
	} else  {
		for (int i = N_RANK-1; i >= 0 && !cut_yet; --i) {
			can_cut = (l_touch_boundary[i]) ? (lb[i] >= thres[i] && lb[i] > dx_recursive_boundary_[i]) : (lb[i] >= thres[i] && lb[i] > dx_recursive_[i]);
			if (can_cut) { 
                l_son_grid = l_father_grid;
                int sep = (initial_cut(i)) ? lb[i]/N_CORES : (2 * slope_[i] * lt);
                //int r = (initial_cut(i)) ? N_CORES : (lb[i]/sep);
                int r = lb[i]/sep;
				int l_start = (l_father_grid.x0[i]);
				int l_end = (l_father_grid.x1[i]);
				int j;
				for (j = 0; j < r-1; ++j) {
					l_son_grid.x0[i] = l_start + sep * j;
					l_son_grid.dx0[i] = slope_[i];
					l_son_grid.x1[i] = l_start + sep * (j+1);
					l_son_grid.dx1[i] = -slope_[i];
                    if (call_boundary) {
                        cilk_spawn obase_boundary_p(t0, t1, l_son_grid, f, bf);
                    } else {
                        cilk_spawn obase_adaptive(t0, t1, l_son_grid, f);
                    }
				}
				l_son_grid.x0[i] = l_start + sep * j;
				l_son_grid.dx0[i] = slope_[i];
				l_son_grid.x1[i] = l_end;
				l_son_grid.dx1[i] = -slope_[i];
                if (call_boundary) {
                    obase_boundary_p(t0, t1, l_son_grid, f, bf);
                } else {
                    obase_adaptive(t0, t1, l_son_grid, f);
                }
#if DEBUG
//				print_sync(stdout);
#endif
				cilk_sync;
				for (j = 1; j < r; ++j) {
					l_son_grid.x0[i] = l_start + sep * j;
					l_son_grid.dx0[i] = -slope_[i];
					l_son_grid.x1[i] = l_start + sep * j;
					l_son_grid.dx1[i] = slope_[i];
                    if (call_boundary) {
                        cilk_spawn obase_boundary_p(t0, t1, l_son_grid, f, bf);
                    } else {
                        cilk_spawn obase_adaptive(t0, t1, l_son_grid, f);
                    }
				}
				if (l_start == initial_grid_.x0[i] && l_end == initial_grid_.x1[i]) {
            //        printf("merge triagles!\n");
					l_son_grid.x0[i] = l_end;
					l_son_grid.dx0[i] = -slope_[i];
					l_son_grid.x1[i] = l_end;
					l_son_grid.dx1[i] = slope_[i];
                    if (call_boundary) {
                        cilk_spawn obase_boundary_p(t0, t1, l_son_grid, f, bf);
                    } else {
                        cilk_spawn obase_adaptive(t0, t1, l_son_grid, f);
                    }
				} else {
					if (l_father_grid.dx0[i] != slope_[i]) {
						l_son_grid.x0[i] = l_start; 
						l_son_grid.dx0[i] = l_father_grid.dx0[i];
						l_son_grid.x1[i] = l_start; 
						l_son_grid.dx1[i] = slope_[i];
                        if (call_boundary) {
                            cilk_spawn obase_boundary_p(t0, t1, l_son_grid, f, bf);
                        } else {
                            cilk_spawn obase_adaptive(t0, t1, l_son_grid, f);
                        }
					}
					if (l_father_grid.dx1[i] != -slope_[i]) {
						l_son_grid.x0[i] = l_end; 
						l_son_grid.dx0[i] = -slope_[i];
						l_son_grid.x1[i] = l_end; 
						l_son_grid.dx1[i] = l_father_grid.dx1[i];
                        if (call_boundary) {
                            cilk_spawn obase_boundary_p(t0, t1, l_son_grid, f, bf);
                        } else {
                            cilk_spawn obase_adaptive(t0, t1, l_son_grid, f);
                        }
					}
				}
				cut_yet = true;
			}/* end if */
		} /* end for */
		if (!cut_yet && lt > dt_recursive_) {
			int halflt = lt / 2;
			l_son_grid = l_father_grid;
            if (call_boundary) {
                obase_boundary_p(t0, t0+halflt, l_son_grid, f, bf);
            } else {
                obase_adaptive(t0, t0+halflt, l_son_grid, f);
            }
#if DEBUG
//			print_sync(stdout);
#endif
			for (int i = 0; i < N_RANK; ++i) {
				l_son_grid.x0[i] = l_father_grid.x0[i] + l_father_grid.dx0[i] * halflt;
				l_son_grid.dx0[i] = l_father_grid.dx0[i];
				l_son_grid.x1[i] = l_father_grid.x1[i] + l_father_grid.dx1[i] * halflt;
				l_son_grid.dx1[i] = l_father_grid.dx1[i];
			}
            if (call_boundary) { 
                obase_boundary_p(t0+halflt, t1, l_son_grid, f, bf);
            } else {
                obase_adaptive(t0+halflt, t1, l_son_grid, f);
            }
			cut_yet = true;
		}
		assert(cut_yet);
		return;
	}
}


#endif /* POCHOIR_WALK_RECURSIVE_HPP */
