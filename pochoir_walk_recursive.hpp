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

#define initial_cut(i) (lb[i] == phys_length_[i])
/* grid.x1[i] >= phys_grid_.x1[i] - stride_[i] - slope_[i] 
 * because we compute the kernel with range [a, b)
 */
template <int N_RANK>
inline bool Algorithm<N_RANK>::touch_boundary(int i, int lt, grid_info<N_RANK> & grid) 
{
    bool interior = false;
    if (grid.x0[i] >= uub_boundary[i] 
     && grid.x0[i] + grid.dx0[i] * lt >= uub_boundary[i]) {
#if (KLEIN == 0)
        /* this is for NON klein bottle */
        interior = true;
        /* by this branch, we are assuming the shape is NOT a Klein bottle */
        grid.x0[i] -= phys_length_[i];
        grid.x1[i] -= phys_length_[i];
#else
        /* this is for klein bottle! */
#if 1
#if DEBUG
        fprintf(stderr, "Before klein_region: \n");
        print_grid(stderr, 0, lt, grid);
#endif
        interior = true;
//        fprintf(stderr, "call klein_region!\n");
        klein_region(grid, phys_grid_);
#if DEBUG
        fprintf(stderr, "After klein_region: \n");
        print_grid(stderr, 0, lt, grid);
#endif
#else
        interior = false;
#endif
#endif
    } else if (grid.x1[i] <= ulb_boundary[i] 
            && grid.x1[i] + grid.dx1[i] * lt <= ulb_boundary[i]
            && grid.x0[i] >= lub_boundary[i]
            && grid.x0[i] + grid.dx0[i] * lt >= lub_boundary[i]) {
        interior = true;
    } else {
        interior = false;
    }
    return !interior;
}

template <int N_RANK>
inline bool Algorithm<N_RANK>::within_boundary(int t0, int t1, grid_info<N_RANK> & grid)
{
    bool l_touch_boundary = false;
    int lt = t1 - t0;
    for (int i = 0; i < N_RANK; ++i) {
        l_touch_boundary |= touch_boundary(i, lt, grid);
    }
    return !l_touch_boundary;
}

template <int N_RANK> template <typename F>
inline void Algorithm<N_RANK>::walk_serial(int t0, int t1, grid_info<N_RANK> const grid, F const & f)
{
    int lt = t1 - t0;
    bool base_cube = (lt <= dt_recursive_); /* dt_recursive_ : temporal dimension stop */
    bool cut_yet = false;
    bool can_cut[N_RANK];
    grid_info<N_RANK> l_grid;

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
        base_case_kernel_boundary(t0, t1, grid, f);
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
template <int N_RANK> template <typename F>
inline void Algorithm<N_RANK>::walk_bicut(int t0, int t1, grid_info<N_RANK> const grid, F const & f)
{
	/* for the initial cut on each dimension, cut into exact N_CORES pieces,
	   for the rest cut into that dimension, cut into as many as we can!
	 */
	int lt = t1 - t0;
	index_info lb, thres;
	grid_info<N_RANK> l_grid;

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
	base_case_kernel_interior(t0, t1, grid, f);
	return;
}

#define push_queue(_dep, _level, _t0, _t1, _grid) \
do { \
    if (queue_len_[_dep] < ALGOR_QUEUE_SIZE) { \
        circular_queue_[_dep][queue_tail_[_dep]].level = _level; \
        circular_queue_[_dep][queue_tail_[_dep]].t0 = _t0; \
        circular_queue_[_dep][queue_tail_[_dep]].t1 = _t1; \
        circular_queue_[_dep][queue_tail_[_dep]].grid = _grid; \
        ++queue_len_[_dep]; \
        queue_tail_[_dep] = pmod((queue_tail_[_dep] + 1), ALGOR_QUEUE_SIZE); \
    } else { \
        fprintf(stderr, "circular queue overflowed!\n"); \
        exit(1); \
    } \
} while(0)

#define top_queue(_dep, _queue_elem) \
do { \
    if (queue_len_[_dep] > 0) { \
        _queue_elem = &(circular_queue_[_dep][queue_head_[_dep]]); \
    } else { \
        fprintf(stderr, "circular queue underflowed!\n"); \
        exit(1); \
    } \
} while(0)

#define pop_queue(_dep) \
do { \
    if (queue_len_[_dep] > 0) { \
        queue_head_[_dep] = pmod((queue_head_[_dep] + 1), ALGOR_QUEUE_SIZE); \
        --queue_len_[_dep]; \
    } else { \
        fprintf(stderr, "circular queue underflowed!\n"); \
        exit(1); \
    } \
} while(0)

/* This is for interior region space cut! */
template <int N_RANK> template <typename F>
inline void Algorithm<N_RANK>::sim_space_cut(int t0, int t1, grid_info<N_RANK> const grid, F const & f)
{
    queue_info *l_father, *l_son;
    queue_info circular_queue_[2][ALGOR_QUEUE_SIZE];
    int queue_head_[2], queue_tail_[2], queue_len_[2];

    for (int i = 0; i < 2; ++i) {
        queue_head_[i] = queue_tail_[i] = queue_len_[i] = 0;
    }

    /* set up the initial grid */
    push_queue(0, 0, t0, t1, grid);
    for (int curr_dep = 0; curr_dep < N_RANK+1; ++curr_dep) {
        const int curr_dep_pointer = (curr_dep & 0x1);
        while (queue_len_[curr_dep_pointer] > 0) {
            top_queue(curr_dep_pointer, l_father);
            if (l_father->level == N_RANK) {
                /* spawn all the grids in circular_queue_[curr_dep][] */
#if DEBUG
                printf("call all sub_grid in dep (%d)\n", curr_dep);
#endif
#if USE_CILK_FOR 
                /* use cilk_for to spawn all the sub-grid */
                cilk_for (int j = 0; j < queue_len_[curr_dep_pointer]; ++j) {
                    int i = pmod((queue_head_[curr_dep_pointer]+j), ALGOR_QUEUE_SIZE);
                    l_son = &(circular_queue_[curr_dep_pointer][i]);
                    /* assert all the sub-grid has done N_RANK spatial cuts */
                    assert(l_son->level == N_RANK);
                    sim_bicut(l_son->t0, l_son->t1, l_son->grid, f);
                } /* end cilk_for */
                queue_head_[curr_dep_pointer] = queue_tail_[curr_dep_pointer] = 0;
                queue_len_[curr_dep_pointer] = 0;
#else
                /* use cilk_spawn to spawn all the sub-grid */
                pop_queue(curr_dep_pointer);
                cilk_spawn sim_bicut(l_father->t0, l_father->t1, l_father->grid, f);
#endif
            } else {
                /* performing a space cut on dimension 'level' */
                pop_queue(curr_dep_pointer);
                const grid_info<N_RANK> l_father_grid = l_father->grid;
                grid_info<N_RANK> l_son_grid = l_father->grid;
                const int t0 = l_father->t0, t1 = l_father->t1;
                const int level = l_father->level;
                const int lb = (l_father_grid.x1[level] - l_father_grid.x0[level]);
                const int sep = (int)lb/2;
                const int r = 2;
                const int l_start = (l_father_grid.x0[level]);
                const int l_end = (l_father_grid.x1[level]);

                /* push one sub-grid into circular queue of (curr_dep) */
                l_son_grid.x0[level] = l_start;
                l_son_grid.dx0[level] = slope_[level];
                l_son_grid.x1[level] = l_start + sep;
                l_son_grid.dx1[level] = -slope_[level];
                push_queue(curr_dep_pointer, level+1, t0, t1, l_son_grid);

                /* push one sub-grid into circular queue of (curr_dep) */
                l_son_grid.x0[level] = l_start + sep;
                l_son_grid.dx0[level] = slope_[level];
                l_son_grid.x1[level] = l_end;
                l_son_grid.dx1[level] = -slope_[level];
                push_queue(curr_dep_pointer, level+1, t0, t1, l_son_grid);

                /* cilk_sync */
                const int next_dep_pointer = (curr_dep + 1) & 0x1;
                /* push one sub-grid into circular queue of (curr_dep + 1)*/
                l_son_grid.x0[level] = l_start + sep;
                l_son_grid.dx0[level] = -slope_[level];
                l_son_grid.x1[level] = l_start + sep;
                l_son_grid.dx1[level] = slope_[level];
                push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);

                if (l_father_grid.dx0[level] != slope_[level]) {
                    l_son_grid.x0[level] = l_start;
                    l_son_grid.dx0[level] = l_father_grid.dx0[level];
                    l_son_grid.x1[level] = l_start;
                    l_son_grid.dx1[level] = slope_[level];
                    push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);
                }
                if (l_father_grid.dx1[level] != -slope_[level]) {
                    l_son_grid.x0[level] = l_end;
                    l_son_grid.dx0[level] = -slope_[level];
                    l_son_grid.x1[level] = l_end;
                    l_son_grid.dx1[level] = l_father_grid.dx1[level];
                    push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);
                }
            }
        } /* end while (queue_len_[curr_dep] > 0) */
#if !USE_CILK_FOR
        cilk_sync;
#endif
        assert(queue_len_[curr_dep_pointer] == 0);
    } /* end for (curr_dep < N_RANK+1) */
}

/* This is for boundary region space cut! */
template <int N_RANK> template <typename F, typename BF>
inline void Algorithm<N_RANK>::sim_space_cut_p(int t0, int t1, grid_info<N_RANK> const grid, F const & f, BF const & bf)
{
    queue_info *l_father, *l_son;
    queue_info circular_queue_[2][ALGOR_QUEUE_SIZE];
    int queue_head_[2], queue_tail_[2], queue_len_[2];

    for (int i = 0; i < 2; ++i) {
        queue_head_[i] = queue_tail_[i] = queue_len_[i] = 0;
    }

    /* set up the initial grid */
    push_queue(0, 0, t0, t1, grid);
    for (int curr_dep = 0; curr_dep < N_RANK+1; ++curr_dep) {
        const int curr_dep_pointer = (curr_dep & 0x1);
        while (queue_len_[curr_dep_pointer] > 0) {
            top_queue(curr_dep_pointer, l_father);
            if (l_father->level == N_RANK) {
                /* spawn all the grids in circular_queue_[curr_dep][] */
#if DEBUG
                printf("call all sub_grid in dep (%d)\n", curr_dep);
#endif
#if USE_CILK_FOR 
                /* use cilk_for to spawn all the sub-grid */
                cilk_for (int j = 0; j < queue_len_[curr_dep_pointer]; ++j) {
                    int i = pmod((queue_head_[curr_dep_pointer]+j), ALGOR_QUEUE_SIZE);
//                    l_son = top_queue(curr_dep);
                    l_son = &(circular_queue_[curr_dep_pointer][i]);
                    /* assert all the sub-grid has done N_RANK spatial cuts */
                    assert(l_son->level == N_RANK);
                    if (within_boundary(l_son->t0, l_son->t1, l_son->grid)) {
                        sim_bicut(l_son->t0, l_son->t1, l_son->grid, f);
                    } else {
                        sim_bicut_p(l_son->t0, l_son->t1, l_son->grid, f, bf);
                    }
                } /* end cilk_for */
                queue_head_[curr_dep_pointer] = queue_tail_[curr_dep_pointer] = 0;
                queue_len_[curr_dep_pointer] = 0;
#else
                /* use cilk_spawn to spawn all the sub-grid */
                pop_queue(curr_dep_pointer);
                if (within_boundary(l_father->t0, l_father->t1, l_father->grid)) {
#if DEBUG
                    printf("call interior!\n");
                    print_grid(stdout, l_father->t0, l_father->t1, l_father->grid);
#endif
                    cilk_spawn sim_bicut(l_father->t0, l_father->t1, l_father->grid, f);
                } else {
#if DEBUG
                    printf("call boundary!\n");
                    print_grid(stdout, l_father->t0, l_father->t1, l_father->grid);
#endif
                    cilk_spawn sim_bicut_p(l_father->t0, l_father->t1, l_father->grid, f, bf);
                }
#endif
            } else {
                /* performing a space cut on dimension 'level' */
                pop_queue(curr_dep_pointer);
                const grid_info<N_RANK> l_father_grid = l_father->grid;
                grid_info<N_RANK> l_son_grid = l_father->grid;
                const int t0 = l_father->t0, t1 = l_father->t1;
                const int level = l_father->level;
                const int lb = (l_father_grid.x1[level] - l_father_grid.x0[level]);
                bool initial_cut = (lb == phys_length_[level]);
#if 1
                const int sep = (initial_cut) ? (int)(lb-2*slope_[level])/2 : (int)lb/2;
                const int r = 2;
                const int l_start = (initial_cut) ? (l_father_grid.x0[level]+slope_[level]) : (l_father_grid.x0[level]);
                const int l_end = (initial_cut) ? (l_father_grid.x1[level]-slope_[level]) : (l_father_grid.x1[level]);
#else
                const int sep = (int)lb/2;
                const int r = 2;
                const int l_start = (l_father_grid.x0[level]);
                const int l_end = (l_father_grid.x1[level]);
#endif

                /* push one sub-grid into circular queue of (curr_dep) */
                l_son_grid.x0[level] = l_start;
                l_son_grid.dx0[level] = slope_[level];
                l_son_grid.x1[level] = l_start + sep;
                l_son_grid.dx1[level] = -slope_[level];
                push_queue(curr_dep_pointer, level+1, t0, t1, l_son_grid);

                /* push one sub-grid into circular queue of (curr_dep) */
                l_son_grid.x0[level] = l_start + sep;
                l_son_grid.dx0[level] = slope_[level];
                l_son_grid.x1[level] = l_end;
                l_son_grid.dx1[level] = -slope_[level];
                push_queue(curr_dep_pointer, level+1, t0, t1, l_son_grid);

                /* cilk_sync */
                const int next_dep_pointer = (curr_dep + 1) & 0x1;
                /* push one sub-grid into circular queue of (curr_dep + 1)*/
                l_son_grid.x0[level] = l_start + sep;
                l_son_grid.dx0[level] = -slope_[level];
                l_son_grid.x1[level] = l_start + sep;
                l_son_grid.dx1[level] = slope_[level];
                push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);

                if (initial_cut) {
                    /* merge triangles! */
                    l_son_grid.x0[level] = l_end;
                    l_son_grid.dx0[level] = -slope_[level];
                    l_son_grid.x1[level] = l_end+2*slope_[level];
                    l_son_grid.dx1[level] = slope_[level];
                    push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);
                } else {
                    if (l_father_grid.dx0[level] != slope_[level]) {
                        l_son_grid.x0[level] = l_start;
                        l_son_grid.dx0[level] = l_father_grid.dx0[level];
                        l_son_grid.x1[level] = l_start;
                        l_son_grid.dx1[level] = slope_[level];
                        push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);
                    }
                    if (l_father_grid.dx1[level] != -slope_[level]) {
                        l_son_grid.x0[level] = l_end;
                        l_son_grid.dx0[level] = -slope_[level];
                        l_son_grid.x1[level] = l_end;
                        l_son_grid.dx1[level] = l_father_grid.dx1[level];
                        push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);
                    }
                }
            }
        } /* end while (queue_len_[curr_dep] > 0) */
#if !USE_CILK_FOR
        cilk_sync;
#endif
        assert(queue_len_[curr_dep_pointer] == 0);
    } /* end for (curr_dep < N_RANK+1) */
}

/* This is the version for interior region cut! */
template <int N_RANK> template <typename F>
inline void Algorithm<N_RANK>::sim_bicut(int t0, int t1, grid_info<N_RANK> const grid, F const & f)
{
    const int lt = t1 - t0;
    bool can_cut = true, base_cube_t = (lt <= dt_recursive_), base_cube_s = true;
    index_info lb, thres;
    grid_info<N_RANK> l_son_grid;

    for (int i = N_RANK-1; i >= 0; --i) {
        lb[i] = (grid.x1[i] - grid.x0[i]);
        thres[i] = 2 * (2 * slope_[i] * lt);
        can_cut = can_cut && (lb[i] >= thres[i] && lb[i] > dx_recursive_[i]);
        base_cube_s = base_cube_s && (lb[i] <= dx_recursive_[i]);
    }

    if (base_cube_t || base_cube_s) {
        // base case
#if DEBUG
        printf("call interior!\n");
        print_grid(stdout, t0, t1, grid);
#endif
        base_case_kernel_interior(t0, t1, grid, f);
        return;
    } else if (can_cut) {
        /* cut into space */
        sim_space_cut(t0, t1, grid, f);
        return;
    } else {
        /* cut into time */
        int halflt = lt / 2;
        l_son_grid = grid;
        sim_bicut(t0, t0+halflt, l_son_grid, f);

        for (int i = 0; i < N_RANK; ++i) {
            l_son_grid.x0[i] = grid.x0[i] + grid.dx0[i] * halflt;
            l_son_grid.dx0[i] = grid.dx0[i];
            l_son_grid.x1[i] = grid.x1[i] + grid.dx1[i] * halflt;
            l_son_grid.dx1[i] = grid.dx1[i];
        }
        sim_bicut(t0+halflt, t1, l_son_grid, f);
        return;
    } 
}

/* This is the version for boundary region cut! */
template <int N_RANK> template <typename F, typename BF>
inline void Algorithm<N_RANK>::sim_bicut_p(int t0, int t1, grid_info<N_RANK> const grid, F const & f, BF const & bf)
{
    const int lt = t1 - t0;
    bool can_cut = true, call_boundary = false, base_cube_t = (lt <= dt_recursive_), base_cube_s = true;
    index_info lb, thres;
    grid_info<N_RANK> l_father_grid = grid, l_son_grid;
    bool l_touch_boundary[N_RANK];

    for (int i = N_RANK-1; i >= 0; --i) {
        /* l_father_grid may be mapped to a new region in touch_boundary() */
        l_touch_boundary[i] = touch_boundary(i, lt, l_father_grid);
        call_boundary |= l_touch_boundary[i];
        lb[i] = (l_father_grid.x1[i] - l_father_grid.x0[i]);
        thres[i] = 2 * (2 * slope_[i] * lt);
        /* for the initial cut, we exclude the begining and end point to minimize
         * the overhead on boundary
        */
        /* some dimension touches the boundary, some may NOT! */
        can_cut = can_cut && ((l_touch_boundary[i]) ? ((lb[i] == phys_length_[i]) ? (lb[i] - 2 * slope_[i] >= thres[i] && lb[i] > dx_recursive_boundary_[i]) : (lb[i] >= thres[i] && lb[i] > dx_recursive_boundary_[i])) : (lb[i] >= thres[i] && lb[i] > dx_recursive_[i]));
        // can_cut = can_cut && ((l_touch_boundary[i]) ? (lb[i] >= thres[i] && lb[i] > dx_recursive_boundary_[i]) : (lb[i] >= thres[i] && lb[i] > dx_recursive_[i]));
        base_cube_s = base_cube_s && (l_touch_boundary[i] ? (lb[i] <= dx_recursive_boundary_[i]) : (lb[i] <= dx_recursive_[i]));
    }

    if (base_cube_t || base_cube_s) {
        // base case
        if (call_boundary) {
#if DEBUG
            printf("call boundary!\n");
            print_grid(stdout, t0, t1, l_father_grid);
#endif
            base_case_kernel_boundary(t0, t1, l_father_grid, bf);
        } else {
#if DEBUG
            printf("call interior!\n");
            print_grid(stdout, t0, t1, l_father_grid);
#endif
            base_case_kernel_interior(t0, t1, l_father_grid, f);
        }
        return;
    } else if (can_cut) {
        /* cut into space */
        /* push the first grid that can be cut into the circular queue */
        /* boundary cuts! */
        if (call_boundary) 
            sim_space_cut_p(t0, t1, l_father_grid, f, bf);
        else
            sim_space_cut(t0, t1, l_father_grid, f);
        return;
    } else {
        /* cut into time */
        int halflt = lt / 2;
        l_son_grid = l_father_grid;
        sim_bicut_p(t0, t0+halflt, l_son_grid, f, bf);

        for (int i = 0; i < N_RANK; ++i) {
            l_son_grid.x0[i] = l_father_grid.x0[i] + l_father_grid.dx0[i] * halflt;
            l_son_grid.dx0[i] = l_father_grid.dx0[i];
            l_son_grid.x1[i] = l_father_grid.x1[i] + l_father_grid.dx1[i] * halflt;
            l_son_grid.dx1[i] = l_father_grid.dx1[i];
        }
        sim_bicut_p(t0+halflt, t1, l_son_grid, f, bf);
        return;
    } 
}
/* ************************************************************************************** */
/* following are the procedures for obase */
template <int N_RANK> template <typename F>
inline void Algorithm<N_RANK>::sim_obase_space_cut(int t0, int t1, grid_info<N_RANK> const grid, F const & f)
{
    queue_info *l_father, *l_son;
    queue_info circular_queue_[2][ALGOR_QUEUE_SIZE];
    int queue_head_[2], queue_tail_[2], queue_len_[2];

    for (int i = 0; i < 2; ++i) {
        queue_head_[i] = queue_tail_[i] = queue_len_[i] = 0;
    }

    /* set up the initial grid */
    push_queue(0, 0, t0, t1, grid);
    for (int curr_dep = 0; curr_dep < N_RANK+1; ++curr_dep) {
        const int curr_dep_pointer = (curr_dep & 0x1);
        while (queue_len_[curr_dep_pointer] > 0) {
            top_queue(curr_dep_pointer, l_father);
            if (l_father->level == N_RANK) {
                /* spawn all the grids in circular_queue_[curr_dep][] */
#if DEBUG
                printf("call all sub_grid in dep (%d)\n", curr_dep);
#endif
#if USE_CILK_FOR 
                /* use cilk_for to spawn all the sub-grid */
                cilk_for (int j = 0; j < queue_len_[curr_dep_pointer]; ++j) {
                    int i = pmod((queue_head_[curr_dep_pointer]+j), ALGOR_QUEUE_SIZE);
                    l_son = &(circular_queue_[curr_dep_pointer][i]);
                    /* assert all the sub-grid has done N_RANK spatial cuts */
                    assert(l_son->level == N_RANK);
                    sim_obase_bicut(l_son->t0, l_son->t1, l_son->grid, f);
                } /* end cilk_for */
                queue_head_[curr_dep_pointer] = queue_tail_[curr_dep_pointer] = 0;
                queue_len_[curr_dep_pointer] = 0;
#else
                /* use cilk_spawn to spawn all the sub-grid */
                pop_queue(curr_dep_pointer);
                cilk_spawn sim_obase_bicut(l_father->t0, l_father->t1, l_father->grid, f);
#endif
            } else {
                /* performing a space cut on dimension 'level' */
                pop_queue(curr_dep_pointer);
                const grid_info<N_RANK> l_father_grid = l_father->grid;
                grid_info<N_RANK> l_son_grid = l_father->grid;
                const int t0 = l_father->t0, t1 = l_father->t1;
                const int level = l_father->level;
                const bool cut_lb = (l_father_grid.dx0[level] >= 0 && l_father_grid.dx1[level] <= 0);
                const int lb = (l_father_grid.x1[level] - l_father_grid.x0[level]);
                if (cut_lb) {
                    /* cut_lb */
                    const int sep = (int)lb/2;
                    const int r = 2;
                    const int l_start = (l_father_grid.x0[level]);
                    const int l_end = (l_father_grid.x1[level]);

                    /* push one sub-grid into circular queue of (curr_dep) */
                    l_son_grid.x0[level] = l_start;
                    l_son_grid.dx0[level] = slope_[level];
                    l_son_grid.x1[level] = l_start + sep;
                    l_son_grid.dx1[level] = -slope_[level];
                    assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                    push_queue(curr_dep_pointer, level+1, t0, t1, l_son_grid);

                    /* push one sub-grid into circular queue of (curr_dep) */
                    l_son_grid.x0[level] = l_start + sep;
                    l_son_grid.dx0[level] = slope_[level];
                    l_son_grid.x1[level] = l_end;
                    l_son_grid.dx1[level] = -slope_[level];
                    assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                    push_queue(curr_dep_pointer, level+1, t0, t1, l_son_grid);

                    /* cilk_sync */
                    const int next_dep_pointer = (curr_dep + 1) & 0x1;
                    /* push one sub-grid into circular queue of (curr_dep + 1)*/
                    l_son_grid.x0[level] = l_start + sep;
                    l_son_grid.dx0[level] = -slope_[level];
                    l_son_grid.x1[level] = l_start + sep;
                    l_son_grid.dx1[level] = slope_[level];
                    assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                    push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);

                    if (l_father_grid.dx0[level] != slope_[level]) {
                        l_son_grid.x0[level] = l_start;
                        l_son_grid.dx0[level] = l_father_grid.dx0[level];
                        l_son_grid.x1[level] = l_start;
                        l_son_grid.dx1[level] = slope_[level];
                        assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                        push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);
                    }
                    if (l_father_grid.dx1[level] != -slope_[level]) {
                        l_son_grid.x0[level] = l_end;
                        l_son_grid.dx0[level] = -slope_[level];
                        l_son_grid.x1[level] = l_end;
                        l_son_grid.dx1[level] = l_father_grid.dx1[level];
                        assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                        push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);
                    }
                } /* end if (cut_lb) */ else {
                    /* cut_tb */
                    const int l_start = (l_father_grid.x0[level]);
                    const int l_end = (l_father_grid.x1[level]);

                    assert(slope_[level] >= l_father_grid.dx0[level]);
                    assert(-slope_[level] <= l_father_grid.dx1[level]);
                    /* push one sub-grid into circular queue of (curr_dep) */
                    l_son_grid.x0[level] = l_start;
                    l_son_grid.dx0[level] = slope_[level];
                    l_son_grid.x1[level] = l_end;
                    l_son_grid.dx1[level] = -slope_[level];
                    assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                    push_queue(curr_dep_pointer, level+1, t0, t1, l_son_grid);

                    /* cilk_sync */
                    const int next_dep_pointer = (curr_dep + 1) & 0x1;
                    /* push one sub-grid into circular queue of (curr_dep + 1)*/
                    l_son_grid.x0[level] = l_start;
                    l_son_grid.dx0[level] = l_father_grid.dx0[level];
                    l_son_grid.x1[level] = l_start;
                    l_son_grid.dx1[level] = slope_[level];
                    assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                    push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);

                    l_son_grid.x0[level] = l_end;
                    l_son_grid.dx0[level] = -slope_[level];
                    l_son_grid.x1[level] = l_end;
                    l_son_grid.dx1[level] = l_father_grid.dx1[level];
                    assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                    push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);
                }
            }
        } /* end while (queue_len_[curr_dep] > 0) */
#if !USE_CILK_FOR
        cilk_sync;
#endif
        assert(queue_len_[curr_dep_pointer] == 0);
    } /* end for (curr_dep < N_RANK+1) */
}

/* This is for boundary region space cut! */
template <int N_RANK> template <typename F, typename BF>
inline void Algorithm<N_RANK>::sim_obase_space_cut_p(int t0, int t1, grid_info<N_RANK> const grid, F const & f, BF const & bf)
{
    queue_info *l_father, *l_son;
    queue_info circular_queue_[2][ALGOR_QUEUE_SIZE];
    int queue_head_[2], queue_tail_[2], queue_len_[2];

    for (int i = 0; i < 2; ++i) {
        queue_head_[i] = queue_tail_[i] = queue_len_[i] = 0;
    }

    /* set up the initial grid */
    push_queue(0, 0, t0, t1, grid);
    for (int curr_dep = 0; curr_dep < N_RANK+1; ++curr_dep) {
        const int curr_dep_pointer = (curr_dep & 0x1);
        while (queue_len_[curr_dep_pointer] > 0) {
            top_queue(curr_dep_pointer, l_father);
            if (l_father->level == N_RANK) {
                /* spawn all the grids in circular_queue_[curr_dep][] */
#if DEBUG
                printf("call all sub_grid in dep (%d)\n", curr_dep);
#endif
#if USE_CILK_FOR 
                /* use cilk_for to spawn all the sub-grid */
                cilk_for (int j = 0; j < queue_len_[curr_dep_pointer]; ++j) {
                    int i = pmod((queue_head_[curr_dep_pointer]+j), ALGOR_QUEUE_SIZE);
//                    l_son = top_queue(curr_dep);
                    l_son = &(circular_queue_[curr_dep_pointer][i]);
                    /* assert all the sub-grid has done N_RANK spatial cuts */
                    assert(l_son->level == N_RANK);
                    if (within_boundary(l_son->t0, l_son->t1, l_son->grid)) {
                        sim_obase_bicut(l_son->t0, l_son->t1, l_son->grid, f);
                    } else {
                        sim_obase_bicut_p(l_son->t0, l_son->t1, l_son->grid, f, bf);
                    }
                } /* end cilk_for */
                queue_head_[curr_dep_pointer] = queue_tail_[curr_dep_pointer] = 0;
                queue_len_[curr_dep_pointer] = 0;
#else
                /* use cilk_spawn to spawn all the sub-grid */
                pop_queue(curr_dep_pointer);
                if (within_boundary(l_father->t0, l_father->t1, l_father->grid)) {
#if DEBUG
                    printf("call interior!\n");
                    print_grid(stdout, l_father->t0, l_father->t1, l_father->grid);
#endif
                    cilk_spawn sim_obase_bicut(l_father->t0, l_father->t1, l_father->grid, f);
                } else {
#if DEBUG
                    printf("call boundary!\n");
                    print_grid(stdout, l_father->t0, l_father->t1, l_father->grid);
#endif
                    cilk_spawn sim_obase_bicut_p(l_father->t0, l_father->t1, l_father->grid, f, bf);
                }
#endif
            } else {
                /* performing a space cut on dimension 'level' */
                pop_queue(curr_dep_pointer);
                const grid_info<N_RANK> l_father_grid = l_father->grid;
                grid_info<N_RANK> l_son_grid = l_father->grid;
                const int t0 = l_father->t0, t1 = l_father->t1;
                const int level = l_father->level;
                const int lb = (l_father_grid.x1[level] - l_father_grid.x0[level]);
                const bool cut_lb = (l_father_grid.dx0[level] >= 0 && l_father_grid.dx1[level] <= 0);
                if (cut_lb) {
                    /* '/ \' */
                    bool initial_cut = (lb == phys_length_[level]);
#if 1
                    const int sep = (initial_cut) ? (int)(lb-2*slope_[level])/2 : (int)lb/2;
                    const int r = 2;
                    const int l_start = (initial_cut) ? (l_father_grid.x0[level]+slope_[level]) : (l_father_grid.x0[level]);
                    const int l_end = (initial_cut) ? (l_father_grid.x1[level]-slope_[level]) : (l_father_grid.x1[level]);
#else
                    const int sep = (int)lb/2;
                    const int r = 2;
                    const int l_start = (l_father_grid.x0[level]);
                    const int l_end = (l_father_grid.x1[level]);
#endif

                    /* push one sub-grid into circular queue of (curr_dep) */
                    l_son_grid.x0[level] = l_start;
                    l_son_grid.dx0[level] = slope_[level];
                    l_son_grid.x1[level] = l_start + sep;
                    l_son_grid.dx1[level] = -slope_[level];
                    assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                    push_queue(curr_dep_pointer, level+1, t0, t1, l_son_grid);

                    /* push one sub-grid into circular queue of (curr_dep) */
                    l_son_grid.x0[level] = l_start + sep;
                    l_son_grid.dx0[level] = slope_[level];
                    l_son_grid.x1[level] = l_end;
                    l_son_grid.dx1[level] = -slope_[level];
                    assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                    push_queue(curr_dep_pointer, level+1, t0, t1, l_son_grid);

                    /* cilk_sync */
                    const int next_dep_pointer = (curr_dep + 1) & 0x1;
                    /* push one sub-grid into circular queue of (curr_dep + 1)*/
                    l_son_grid.x0[level] = l_start + sep;
                    l_son_grid.dx0[level] = -slope_[level];
                    l_son_grid.x1[level] = l_start + sep;
                    l_son_grid.dx1[level] = slope_[level];
                    assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                    push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);

                    if (initial_cut) {
                        /* merge triangles! */
                        l_son_grid.x0[level] = l_end;
                        l_son_grid.dx0[level] = -slope_[level];
                        l_son_grid.x1[level] = l_end+2*slope_[level];
                        l_son_grid.dx1[level] = slope_[level];
                        assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                        push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);
                    } else {
                        if (l_father_grid.dx0[level] != slope_[level]) {
                            l_son_grid.x0[level] = l_start;
                            l_son_grid.dx0[level] = l_father_grid.dx0[level];
                            l_son_grid.x1[level] = l_start;
                            l_son_grid.dx1[level] = slope_[level];
                            assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                            push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);
                        }
                        if (l_father_grid.dx1[level] != -slope_[level]) {
                            l_son_grid.x0[level] = l_end;
                            l_son_grid.dx0[level] = -slope_[level];
                            l_son_grid.x1[level] = l_end;
                            l_son_grid.dx1[level] = l_father_grid.dx1[level];
                            assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                            push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);
                        }
                    }
                } /* end if (cut_lb) */else {
                    /* '\ /' */
                    /* cut_tb */
                    const int l_start = (l_father_grid.x0[level]);
                    const int l_end = (l_father_grid.x1[level]);

                    assert(slope_[level] >= l_father_grid.dx0[level]);
                    assert(-slope_[level] <= l_father_grid.dx1[level]);
                    /* push one sub-grid into circular queue of (curr_dep) */
                    l_son_grid.x0[level] = l_start;
                    l_son_grid.dx0[level] = slope_[level];
                    l_son_grid.x1[level] = l_end;
                    l_son_grid.dx1[level] = -slope_[level];
                    assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                    push_queue(curr_dep_pointer, level+1, t0, t1, l_son_grid);

                    /* cilk_sync */
                    const int next_dep_pointer = (curr_dep + 1) & 0x1;
                    /* push one sub-grid into circular queue of (curr_dep + 1)*/
                    l_son_grid.x0[level] = l_start;
                    l_son_grid.dx0[level] = l_father_grid.dx0[level];
                    l_son_grid.x1[level] = l_start;
                    l_son_grid.dx1[level] = slope_[level];
                    assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                    push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);

                    l_son_grid.x0[level] = l_end;
                    l_son_grid.dx0[level] = -slope_[level];
                    l_son_grid.x1[level] = l_end;
                    l_son_grid.dx1[level] = l_father_grid.dx1[level];
                    assert(l_son_grid.x0[level] <= l_son_grid.x1[level]);
                    push_queue(next_dep_pointer, level+1, t0, t1, l_son_grid);
                }
            }
        } /* end while (queue_len_[curr_dep] > 0) */
#if !USE_CILK_FOR
        cilk_sync;
#endif
        assert(queue_len_[curr_dep_pointer] == 0);
    } /* end for (curr_dep < N_RANK+1) */
}

/* following are the procedures for obase one-space dimension cut! */
template <int N_RANK> template <typename F>
inline void Algorithm<N_RANK>::obase_one_space_cut(int dim, int t0, int t1, grid_info<N_RANK> const grid, F const & f)
{
    /* performing a space cut on dimension 'level' */
    grid_info<N_RANK> l_son_grid = grid;
    const bool cut_lb = (grid.dx0[dim] >= 0 && grid.dx1[dim] <= 0);
    if (cut_lb) {
        /* cut_lb */
        const int lb = (grid.x1[dim] - grid.x0[dim]);
        const int sep = (int)lb/2;
        const int r = 2;
        const int l_start = (grid.x0[dim]);
        const int l_end = (grid.x1[dim]);

        /* push one sub-grid into circular queue of (curr_dep) */
        l_son_grid.x0[dim] = l_start;
        l_son_grid.dx0[dim] = slope_[dim];
        l_son_grid.x1[dim] = l_start + sep;
        l_son_grid.dx1[dim] = -slope_[dim];
        assert(l_son_grid.x0[dim] <= l_son_grid.x1[dim]);
        cilk_spawn sim_obase_bicut(t0, t1, l_son_grid, f);

        /* push one sub-grid into circular queue of (curr_dep) */
        l_son_grid.x0[dim] = l_start + sep;
        l_son_grid.dx0[dim] = slope_[dim];
        l_son_grid.x1[dim] = l_end;
        l_son_grid.dx1[dim] = -slope_[dim];
        assert(l_son_grid.x0[dim] <= l_son_grid.x1[dim]);
        sim_obase_bicut(t0, t1, l_son_grid, f);

        cilk_sync;
        /* push one sub-grid into circular queue of (curr_dep + 1)*/
        l_son_grid.x0[dim] = l_start + sep;
        l_son_grid.dx0[dim] = -slope_[dim];
        l_son_grid.x1[dim] = l_start + sep;
        l_son_grid.dx1[dim] = slope_[dim];
        assert(l_son_grid.x0[dim] <= l_son_grid.x1[dim]);
        sim_obase_bicut(t0, t1, l_son_grid, f);

        if (grid.dx0[dim] != slope_[dim]) {
            l_son_grid.x0[dim] = l_start;
            l_son_grid.dx0[dim] = grid.dx0[dim];
            l_son_grid.x1[dim] = l_start;
            l_son_grid.dx1[dim] = slope_[dim];
            assert(l_son_grid.x0[dim] <= l_son_grid.x1[dim]);
            sim_obase_bicut(t0, t1, l_son_grid, f);
        }
        if (grid.dx1[dim] != -slope_[dim]) {
            l_son_grid.x0[dim] = l_end;
            l_son_grid.dx0[dim] = -slope_[dim];
            l_son_grid.x1[dim] = l_end;
            l_son_grid.dx1[dim] = grid.dx1[dim];
            assert(l_son_grid.x0[dim] <= l_son_grid.x1[dim]);
            sim_obase_bicut(t0, t1, l_son_grid, f);
        }
    } /* end if (cut_lb) */ else {
        /* cut_tb */
        const int l_start = (grid.x0[dim]);
        const int l_end = (grid.x1[dim]);

        /* push one sub-grid into circular queue of (curr_dep) */
        l_son_grid.x0[dim] = l_start;
        l_son_grid.dx0[dim] = slope_[dim];
        l_son_grid.x1[dim] = l_end;
        l_son_grid.dx1[dim] = -slope_[dim];
        assert(l_son_grid.x0[dim] <= l_son_grid.x1[dim]);
        sim_obase_bicut(t0, t1, l_son_grid, f);

        cilk_sync;
        /* push one sub-grid into circular queue of (curr_dep + 1)*/
        l_son_grid.x0[dim] = l_start;
        l_son_grid.dx0[dim] = grid.dx0[dim];
        l_son_grid.x1[dim] = l_start;
        l_son_grid.dx1[dim] = slope_[dim];
        assert(l_son_grid.x0[dim] <= l_son_grid.x1[dim]);
        cilk_spawn sim_obase_bicut(t0, t1, l_son_grid, f);

        l_son_grid.x0[dim] = l_end;
        l_son_grid.dx0[dim] = -slope_[dim];
        l_son_grid.x1[dim] = l_end;
        l_son_grid.dx1[dim] = grid.dx1[dim];
        assert(l_son_grid.x0[dim] <= l_son_grid.x1[dim]);
        cilk_spawn sim_obase_bicut(t0, t1, l_son_grid, f);
    }
    return;
}

/* following are the procedures for obase_p one-space dimension cut! */
template <int N_RANK> template <typename F, typename BF>
inline void Algorithm<N_RANK>::obase_one_space_cut_p(int dim, int t0, int t1, grid_info<N_RANK> const grid, F const & f, BF const & bf)
{
    /* performing a space cut on dimension 'level' */
    grid_info<N_RANK> l_son_grid = grid;
    const bool cut_lb = (grid.dx0[dim] >= 0 && grid.dx1[dim] <= 0);
    if (cut_lb) {
        /* cut_lb */
        const int lb = (grid.x1[dim] - grid.x0[dim]);
        const int sep = (int)lb/2;
        const int r = 2;
        const int l_start = (grid.x0[dim]);
        const int l_end = (grid.x1[dim]);

        /* push one sub-grid into circular queue of (curr_dep) */
        l_son_grid.x0[dim] = l_start;
        l_son_grid.dx0[dim] = slope_[dim];
        l_son_grid.x1[dim] = l_start + sep;
        l_son_grid.dx1[dim] = -slope_[dim];
        assert(l_son_grid.x0[dim] <= l_son_grid.x1[dim]);
        if (within_boundary(t0, t1, l_son_grid)) {
            cilk_spawn sim_obase_bicut(t0, t1, l_son_grid, f);
        } else {
            cilk_spawn sim_obase_bicut_p(t0, t1, l_son_grid, f, bf);
        }

        /* push one sub-grid into circular queue of (curr_dep) */
        l_son_grid.x0[dim] = l_start + sep;
        l_son_grid.dx0[dim] = slope_[dim];
        l_son_grid.x1[dim] = l_end;
        l_son_grid.dx1[dim] = -slope_[dim];
        assert(l_son_grid.x0[dim] <= l_son_grid.x1[dim]);
        if (within_boundary(t0, t1, l_son_grid)) {
            sim_obase_bicut(t0, t1, l_son_grid, f);
        } else {
            sim_obase_bicut_p(t0, t1, l_son_grid, f, bf);
        }

        cilk_sync;
        /* push one sub-grid into circular queue of (curr_dep + 1)*/
        l_son_grid.x0[dim] = l_start + sep;
        l_son_grid.dx0[dim] = -slope_[dim];
        l_son_grid.x1[dim] = l_start + sep;
        l_son_grid.dx1[dim] = slope_[dim];
        assert(l_son_grid.x0[dim] <= l_son_grid.x1[dim]);
        if (within_boundary(t0, t1, l_son_grid)) {
            cilk_spawn sim_obase_bicut(t0, t1, l_son_grid, f);
        } else {
            cilk_spawn sim_obase_bicut_p(t0, t1, l_son_grid, f, bf);
        }

        if (grid.dx0[dim] != slope_[dim]) {
            l_son_grid.x0[dim] = l_start;
            l_son_grid.dx0[dim] = grid.dx0[dim];
            l_son_grid.x1[dim] = l_start;
            l_son_grid.dx1[dim] = slope_[dim];
            assert(l_son_grid.x0[dim] <= l_son_grid.x1[dim]);
            if (within_boundary(t0, t1, l_son_grid)) {
                cilk_spawn sim_obase_bicut(t0, t1, l_son_grid, f);
            } else {
                cilk_spawn sim_obase_bicut_p(t0, t1, l_son_grid, f, bf);
            }
        }
        if (grid.dx1[dim] != -slope_[dim]) {
            l_son_grid.x0[dim] = l_end;
            l_son_grid.dx0[dim] = -slope_[dim];
            l_son_grid.x1[dim] = l_end;
            l_son_grid.dx1[dim] = grid.dx1[dim];
            assert(l_son_grid.x0[dim] <= l_son_grid.x1[dim]);
            if (within_boundary(t0, t1, l_son_grid)) {
                cilk_spawn sim_obase_bicut(t0, t1, l_son_grid, f);
            } else {
                cilk_spawn sim_obase_bicut_p(t0, t1, l_son_grid, f, bf);
            }
        }
    } /* end if (cut_lb) */ else {
        /* cut_tb */
        const int l_start = (grid.x0[dim]);
        const int l_end = (grid.x1[dim]);

        /* push one sub-grid into circular queue of (curr_dep) */
        l_son_grid.x0[dim] = l_start;
        l_son_grid.dx0[dim] = slope_[dim];
        l_son_grid.x1[dim] = l_end;
        l_son_grid.dx1[dim] = -slope_[dim];
        assert(l_son_grid.x0[dim] <= l_son_grid.x1[dim]);
        if (within_boundary(t0, t1, l_son_grid)) {
            cilk_spawn sim_obase_bicut(t0, t1, l_son_grid, f);
        } else {
            cilk_spawn sim_obase_bicut_p(t0, t1, l_son_grid, f, bf);
        }

        cilk_sync;
        /* push one sub-grid into circular queue of (curr_dep + 1)*/
        l_son_grid.x0[dim] = l_start;
        l_son_grid.dx0[dim] = grid.dx0[dim];
        l_son_grid.x1[dim] = l_start;
        l_son_grid.dx1[dim] = slope_[dim];
        assert(l_son_grid.x0[dim] <= l_son_grid.x1[dim]);
        if (within_boundary(t0, t1, l_son_grid)) {
            cilk_spawn sim_obase_bicut(t0, t1, l_son_grid, f);
        } else {
            cilk_spawn sim_obase_bicut_p(t0, t1, l_son_grid, f, bf);
        }

        l_son_grid.x0[dim] = l_end;
        l_son_grid.dx0[dim] = -slope_[dim];
        l_son_grid.x1[dim] = l_end;
        l_son_grid.dx1[dim] = grid.dx1[dim];
        assert(l_son_grid.x0[dim] <= l_son_grid.x1[dim]);
        if (within_boundary(t0, t1, l_son_grid)) {
            cilk_spawn sim_obase_bicut(t0, t1, l_son_grid, f);
        } else {
            cilk_spawn sim_obase_bicut_p(t0, t1, l_son_grid, f, bf);
        }
    }
    return;
}


/* This is the version for interior region cut! */
template <int N_RANK> template <typename F>
inline void Algorithm<N_RANK>::sim_obase_bicut(int t0, int t1, grid_info<N_RANK> const grid, F const & f)
{
    const int lt = t1 - t0;
    bool sim_can_cut = true, base_cube_t = (lt <= dt_recursive_), base_cube_s = true;
    int largest_dim = -1, largest_dim_size = 0;
    grid_info<N_RANK> l_son_grid;

    for (int i = N_RANK-1; i >= 0; --i) {
        int lb, thres, tb;
        lb = (grid.x1[i] - grid.x0[i]);
        tb = (grid.x1[i] + grid.dx1[i] * lt - grid.x0[i] - grid.dx0[i] * lt);
        /* cut_lb = '/ \' */
        bool cut_lb = (grid.dx0[i] >= 0 && grid.dx1[i] <= 0);
        thres = (2 * slope_[i] * lt);
        bool l_can_cut = (cut_lb ? (lb >= 2 * thres && lb > dx_recursive_[i]) : (lb >= thres && tb > dx_recursive_[i]));
        sim_can_cut = sim_can_cut && l_can_cut; 
        largest_dim = (l_can_cut ? (cut_lb ? ((lb > largest_dim_size) ? i : largest_dim) : ((tb > largest_dim_size) ? i : largest_dim)) : largest_dim); 
        largest_dim_size = (l_can_cut ? (cut_lb ? (lb > largest_dim_size ? lb : largest_dim_size) : (tb > largest_dim_size ? tb : largest_dim_size)) : largest_dim_size); 
        base_cube_s = base_cube_s && (cut_lb ? (lb <= dx_recursive_[i]) : (tb <= dx_recursive_[i]));
    }

    if (base_cube_s || base_cube_t) {
        // base case
#if DEBUG
        printf("call interior!\n");
        print_grid(stdout, t0, t1, grid);
#endif
        f(t0, t1, grid);
//        base_case_kernel_interior(t0, t1, grid, f);
        return;
    } else if (sim_can_cut) {
        /* cut into space */
        sim_obase_space_cut(t0, t1, grid, f);
        return;
    } else if (largest_dim_size > 0) {
        /* cut into dim 'largest_dim' */
        assert(largest_dim >= 0);
        obase_one_space_cut(largest_dim, t0, t1, grid, f);
    } else {
        /* cut into time */
        assert(dt_recursive_ >= r_t);
        int halflt = lt / r_t;
        l_son_grid = grid;
        sim_obase_bicut(t0, t0+halflt, l_son_grid, f);

        int j;
        for (j = 1; j < r_t-1; ++j) {
            for (int i = 0; i < N_RANK; ++i) {
                l_son_grid.x0[i] = grid.x0[i] + grid.dx0[i] * j * halflt;
                l_son_grid.dx0[i] = grid.dx0[i];
                l_son_grid.x1[i] = grid.x1[i] + grid.dx1[i] * j * halflt;
                l_son_grid.dx1[i] = grid.dx1[i];
            }
            sim_obase_bicut(t0+j*halflt, t0+(j+1)*halflt, l_son_grid, f);
        }
        for (int i = 0; i < N_RANK; ++i) {
            l_son_grid.x0[i] = grid.x0[i] + grid.dx0[i] * j * halflt;
            l_son_grid.dx0[i] = grid.dx0[i];
            l_son_grid.x1[i] = grid.x1[i] + grid.dx1[i] * j * halflt;
            l_son_grid.dx1[i] = grid.dx1[i];
        }
        sim_obase_bicut(t0+j*halflt, t1, l_son_grid, f);
        return;
    } 
}

/* This is the version for boundary region cut! */
template <int N_RANK> template <typename F, typename BF>
inline void Algorithm<N_RANK>::sim_obase_bicut_p(int t0, int t1, grid_info<N_RANK> const grid, F const & f, BF const & bf)
{
    const int lt = t1 - t0;
    bool sim_can_cut = true, base_cube_t = (lt <= dt_recursive_boundary_), base_cube_s = true;
    int largest_dim = -1, largest_dim_size = 0;
    grid_info<N_RANK> l_son_grid;

    for (int i = N_RANK-1; i >= 0; --i) {
        int lb, thres, tb;
        bool cut_lb, l_can_cut;
        lb = (grid.x1[i] - grid.x0[i]);
        tb = (grid.x1[i] + grid.dx1[i] * lt - grid.x0[i] - grid.dx0[i] * lt);
        /* cut_lb = '/ \' */
        cut_lb = (grid.dx0[i] >= 0 && grid.dx1[i] <= 0);
        thres = (2 * slope_[i] * lt);
        /* l_father_grid may be mapped to a new region in touch_boundary() */
        /* for the initial cut, we exclude the begining and end point to minimize
         * the overhead on boundary
        */
        l_can_cut = (cut_lb ? ((lb == phys_length_[i]) ? (lb - 2 * slope_[i] >= 2 * thres && lb > dx_recursive_boundary_[i]) : (lb >= 2 * thres && lb > dx_recursive_boundary_[i])) : (lb >= thres && tb > dx_recursive_boundary_[i]));
        sim_can_cut = sim_can_cut && l_can_cut;
        largest_dim = (l_can_cut ? (cut_lb ? ((lb > largest_dim_size) ? i : largest_dim) : ((tb > largest_dim_size) ? i : largest_dim)) : largest_dim);
        largest_dim_size = (l_can_cut ? (cut_lb ? ((lb > largest_dim_size) ? lb : largest_dim_size) : ((tb > largest_dim_size) ? tb : largest_dim_size)) : largest_dim_size);
        base_cube_s = base_cube_s && (cut_lb ? (lb <= dx_recursive_boundary_[i]) : (tb <= dx_recursive_boundary_[i]));
    }

    if (base_cube_s || base_cube_t) {
        /* for base_cube_t: -- prevent too small time cut! 
         *      (cut_lb && lb > dx_recursive_boundary_ && lb < 2 * thres)
         *  ||  (!cut_lb && tb > dx_recursive_boundary_ && lb < thres)
         */
        // base case
        base_case_kernel_boundary(t0, t1, grid, bf);
        return;
    } else if (sim_can_cut) {
        /* cut into space */
        /* push the first l_father_grid that can be cut into the circular queue */
        /* boundary cuts! */
        sim_obase_space_cut_p(t0, t1, grid, f, bf);
        return;
    } else if (largest_dim_size > 0) {
        /* cut into dim 'largest_dim' */
        assert(largest_dim >= 0);
        obase_one_space_cut_p(largest_dim, t0, t1, grid, f, bf);
    } else {
        /* cut into time */
        assert(dt_recursive_boundary_ >= r_t);
        int halflt = lt / r_t;
        l_son_grid = grid;
        if (within_boundary(t0, t0+halflt, l_son_grid)) {
            sim_obase_bicut(t0, t0+halflt, l_son_grid, f);
        } else {
            sim_obase_bicut_p(t0, t0+halflt, l_son_grid, f, bf);
        }

        int j;
        for (j = 1; j < r_t-1; ++j) {
            for (int i = 0; i < N_RANK; ++i) {
                l_son_grid.x0[i] = grid.x0[i] + grid.dx0[i] * j * halflt;
                l_son_grid.dx0[i] = grid.dx0[i];
                l_son_grid.x1[i] = grid.x1[i] + grid.dx1[i] * j * halflt;
                l_son_grid.dx1[i] = grid.dx1[i];
            }
            if (within_boundary(t0+j*halflt, t0+(j+1)*halflt, l_son_grid)) {
                sim_obase_bicut(t0+j*halflt, t0+(j+1)*halflt, l_son_grid, f);
            } else {
                sim_obase_bicut_p(t0+j*halflt, t0+(j+1)*halflt, l_son_grid, f, bf);
            }
        }
        for (int i = 0; i < N_RANK; ++i) {
            l_son_grid.x0[i] = grid.x0[i] + grid.dx0[i] * j * halflt;
            l_son_grid.dx0[i] = grid.dx0[i];
            l_son_grid.x1[i] = grid.x1[i] + grid.dx1[i] * j * halflt;
            l_son_grid.dx1[i] = grid.dx1[i];
        }
        if (within_boundary(t0+j*halflt, t1, l_son_grid)) {
            sim_obase_bicut(t0+j*halflt, t1, l_son_grid, f);
        } else {
            sim_obase_bicut_p(t0+j*halflt, t1, l_son_grid, f, bf);
        }
        return;
    } 
}

/* ************************************************************************************** */
/* walk_adaptive() is just for interior region */
template <int N_RANK> template <typename F>
inline void Algorithm<N_RANK>::walk_adaptive(int t0, int t1, grid_info<N_RANK> const grid, F const & f)
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
	grid_info<N_RANK> l_grid;

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
		base_case_kernel_interior(t0, t1, grid, f);
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
template <int N_RANK> template <typename F, typename BF>
inline void Algorithm<N_RANK>::walk_bicut_boundary_p(int t0, int t1, grid_info<N_RANK> const grid, F const & f, BF const & bf)
{
	/* cut into exact N_CORES pieces */
	/* Indirect memory access is expensive */
	int lt = t1 - t0;
	bool base_cube = (lt <= dt_recursive_); /* dt_recursive_ : temporal dimension stop */
	bool can_cut = false, call_boundary = false;
	index_info lb, thres;
    grid_info<N_RANK> l_father_grid = grid, l_son_grid;
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

			if (l_start == phys_grid_.x0[i] && l_end == phys_grid_.x1[i]) {
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
		base_case_kernel_boundary(t0, t1, l_father_grid, bf);
    } else {
#if DEBUG
        printf("call Interior! ");
	    print_grid(stdout, t0, t1, l_father_grid);
#endif
	    base_case_kernel_interior(t0, t1, l_father_grid, f);
    }
    return;
}


/* walk_ncores_boundary_p() will be called for -split-shadow mode */
template <int N_RANK> template <typename F, typename BF>
inline void Algorithm<N_RANK>::walk_ncores_boundary_p(int t0, int t1, grid_info<N_RANK> const grid, F const & f, BF const & bf)
{
	/* cut into exact N_CORES pieces */
	/* Indirect memory access is expensive */
	int lt = t1 - t0;
	bool base_cube = (lt <= dt_recursive_); /* dt_recursive_ : temporal dimension stop */
	bool cut_yet = false, can_cut = false, call_boundary = false;
	index_info lb, thres;
    grid_info<N_RANK> l_father_grid = grid, l_son_grid;
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
			base_case_kernel_boundary(t0, t1, l_father_grid, bf);
        } else {
#if DEBUG
            printf("call Interior! ");
	    	print_grid(stdout, t0, t1, l_father_grid);
#endif
			base_case_kernel_interior(t0, t1, l_father_grid, f);
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
				if (l_start == phys_grid_.x0[i] && l_end == phys_grid_.x1[i]) {
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
template <int N_RANK> template <typename F>
inline void Algorithm<N_RANK>::obase_bicut(int t0, int t1, grid_info<N_RANK> const grid, F const & f)
{
	/* for the initial cut on each dimension, cut into exact N_CORES pieces,
	   for the rest cut into that dimension, cut into as many as we can!
	 */
	int lt = t1 - t0;
	index_info lb, thres;
	grid_info<N_RANK> l_grid;

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
template <int N_RANK> template <typename F>
inline void Algorithm<N_RANK>::obase_adaptive(int t0, int t1, grid_info<N_RANK> const grid, F const & f)
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
	grid_info<N_RANK> l_grid;

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
template <int N_RANK> template <typename BF>
inline void Algorithm<N_RANK>::obase_bicut_boundary_p(int t0, int t1, grid_info<N_RANK> const grid, BF const & bf)
{
	/* cut into exact N_CORES pieces */
	/* Indirect memory access is expensive */
	int lt = t1 - t0;
	bool can_cut = false, call_boundary = false;
	index_info lb, thres;
    grid_info<N_RANK> l_father_grid = grid, l_son_grid;
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
			if (l_start == phys_grid_.x0[i] && l_end == phys_grid_.x1[i]) {
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
	base_case_kernel_boundary(t0, t1, l_father_grid, bf);
	return;
}


/* this is the version for executable spec!!! */
template <int N_RANK> template <typename BF>
inline void Algorithm<N_RANK>::obase_boundary_p(int t0, int t1, grid_info<N_RANK> const grid, BF const & bf)
{
	/* cut into exact N_CORES pieces */
	/* Indirect memory access is expensive */
	int lt = t1 - t0;
	bool base_cube = (lt <= dt_recursive_); /* dt_recursive_ : temporal dimension stop */
	bool cut_yet = false, can_cut = false, call_boundary = false;
	index_info lb, thres;
    grid_info<N_RANK> l_father_grid = grid, l_son_grid;
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
		base_case_kernel_boundary(t0, t1, l_father_grid, bf);
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
				if (l_start == phys_grid_.x0[i] && l_end == phys_grid_.x1[i]) {
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
template <int N_RANK> template <typename F, typename BF>
inline void Algorithm<N_RANK>::obase_bicut_boundary_p(int t0, int t1, grid_info<N_RANK> const grid, F const & f, BF const & bf)
{
	/* cut into exact N_CORES pieces */
	/* Indirect memory access is expensive */
	int lt = t1 - t0;
	bool can_cut = false, call_boundary = false;
	index_info lb, thres;
    grid_info<N_RANK> l_father_grid = grid, l_son_grid;
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

			if (l_start == phys_grid_.x0[i] && l_end == phys_grid_.x1[i]) {
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
		base_case_kernel_boundary(t0, t1, l_father_grid, bf);
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
template <int N_RANK> template <typename F, typename BF>
inline void Algorithm<N_RANK>::obase_boundary_p(int t0, int t1, grid_info<N_RANK> const grid, F const & f, BF const & bf)
{
	/* cut into exact N_CORES pieces */
	/* Indirect memory access is expensive */
	int lt = t1 - t0;
	bool base_cube = (lt <= dt_recursive_); /* dt_recursive_ : temporal dimension stop */
	bool cut_yet = false, can_cut = false, call_boundary = false;
	index_info lb, thres;
    grid_info<N_RANK> l_father_grid = grid, l_son_grid;
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
			base_case_kernel_boundary(t0, t1, l_father_grid, bf);
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
				if (l_start == phys_grid_.x0[i] && l_end == phys_grid_.x1[i]) {
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
