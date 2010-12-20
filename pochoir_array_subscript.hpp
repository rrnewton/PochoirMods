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

#ifndef POCHOIR_ARRAY_H
#define POCHOIR_ARRAY_H
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>

#include "pochoir_range.hpp"
#include "pochoir_common.hpp"
#include "pochoir_wrapper.hpp"
#include "pochoir_iter.hpp"

using namespace std;

template <int DIM>
inline int cal_index(int const * _idx, int const * _stride) {
	return (_idx[DIM] * _stride[DIM]) + cal_index<DIM-1>(_idx, _stride);
}

template <>
inline int cal_index<0>(int const * _idx, int const * _stride) {
	/* 0-dim is always the time dimension */
	return (_idx[0] * _stride[0]);
}

template <int TOGGLE>
inline int toggle_base(int const & _idx0) {
    return (_idx0 % TOGGLE);
}

template <>
inline int toggle_base<4>(int const & _idx0) {
    return (_idx0 & 0x11);
}

#if 0
template <>
inline int toggle_base<3>(int const & _idx0) {
    return (_idx0 % 3);
}
#endif

template <>
inline int toggle_base<2>(int const & _idx0) {
    return (_idx0 & 0x1);
}

template <>
inline int toggle_base<1>(int const & _idx0) {
    return 0;
}

template <typename T>
class Storage {
	private:
		T * storage_;
		int ref_;
	public:
		inline Storage(int _sz) {
			storage_ = new T[_sz];
			ref_ = 1;
			for (int i = 0; i < _sz; ++i)
				storage_[i] = T();
		}

		inline ~Storage() {
			delete[] storage_;
		}

		inline void inc_ref() { 
			++ref_; 
		}

		inline void dec_ref() { 
			--ref_; 
		}

		inline int ref() { 
			return ref_; 
		}

		inline T & operator[] (int _idx) {
			return storage_[_idx];
		}

		inline T const & operator[] (int _idx) const {
			return storage_[_idx];
		}

		T * data() { return storage_; }
};

template <typename T, int N_RANK, int TOGGLE=2>
class Pochoir_Array {
	private:
		Storage<T> * view_; // real storage of elements
        T * data_; /* begining data pointer of view_, reserved for iterator! */
		typedef int size_info[N_RANK];
		size_info logic_size_; // logical of elements in each dimension
		size_info logic_start_, logic_end_; 
		size_info phys_size_; // physical of elements in each dimension
		size_info stride_; // stride of each dimension
		int total_size_;
        typedef T (*BValue_1D)(Pochoir_Array<T, 1, TOGGLE> &, int, int);
        typedef T (*BValue_2D)(Pochoir_Array<T, 2, TOGGLE> &, int, int, int);
        typedef T (*BValue_3D)(Pochoir_Array<T, 3, TOGGLE> &, int, int, int, int);
        T * l_null;
        BValue_1D bv1_;
        BValue_2D bv2_;
        BValue_3D bv3_;
	public:
		/* create array with initial size 
         * - Following dimensions for constructors are spatial dimension
         * - all spatial dimensions are row-majored
         */
        explicit Pochoir_Array (int sz0) {
            logic_size_[0] = phys_size_[0] = sz0;
            logic_start_[0] = 0; logic_end_[0] = sz0;
            stride_[0] = 1; 
            total_size_ = sz0;
            view_ = NULL;
            view_ = new Storage<T>(TOGGLE * total_size_);
            bv1_ = NULL; bv2_ = NULL; bv3_ = NULL;
            data_ = view_->data();
            l_null = (T*) calloc(1, sizeof(T));
        }

		explicit Pochoir_Array (int sz1, int sz0) {
			logic_size_[1] = sz1; logic_size_[0] = sz0; 
			phys_size_[1] = sz1; phys_size_[0] = sz0; 
            logic_start_[0] = 0; logic_end_[0] = sz0;
            logic_start_[1] = 0; logic_end_[1] = sz1;
			stride_[1] = sz0; stride_[0] = 1; 
			total_size_ = phys_size_[0] * phys_size_[1];
			view_ = NULL;
			view_ = new Storage<T>(TOGGLE * total_size_) ;
            bv1_ = NULL; bv2_ = NULL; bv3_ = NULL;
            data_ = view_->data();
            l_null = (T*) calloc(1, sizeof(T));
		}

		explicit Pochoir_Array (int sz2, int sz1, int sz0) {
			logic_size_[2] = sz2; logic_size_[1] = sz1; logic_size_[0] = sz0; 
			phys_size_[2] = sz2; phys_size_[1] = sz1; phys_size_[0] = sz0; 
            logic_start_[0] = 0; logic_end_[0] = sz0;
            logic_start_[1] = 0; logic_end_[1] = sz1;
            logic_start_[2] = 0; logic_end_[2] = sz2;
			stride_[0] = 1;  
			total_size_ = phys_size_[2];
			for (int i = 0; i < 2; ++i) {
				total_size_ *= phys_size_[i];
				stride_[i+1] = stride_[i] * phys_size_[i];
			}
			view_ = NULL;
			/* double the total_size_ because we are using toggle array */
			view_ = new Storage<T>(TOGGLE*total_size_) ;
            bv1_ = NULL; bv2_ = NULL; bv3_ = NULL;
            data_ = view_->data();
            l_null = (T*) calloc(1, sizeof(T));
		}

		/* Copy constructor -- create another view of the
		 * same array
		 */
		Pochoir_Array (Pochoir_Array<T, N_RANK, TOGGLE> const & orig) {
			total_size_ = orig.total_size();
			for (int i = 0; i < N_RANK; ++i) {
				phys_size_[i] = orig.phys_size(i);
				logic_size_[i] = orig.logic_size(i);
				stride_[i] = orig.stride(i);
                logic_start_[i] = 0; logic_end_[i] = logic_size_[i];
			}
			view_ = NULL;
			view_ = const_cast<Pochoir_Array<T, N_RANK, TOGGLE> &>(orig).view();
			view_->inc_ref();
            /* We also get the BValue function pointer from orig */
            bv1_ = const_cast<Pochoir_Array<T, N_RANK, TOGGLE> &>(orig).bv_1D(); 
            bv2_ = const_cast<Pochoir_Array<T, N_RANK, TOGGLE> &>(orig).bv_2D(); 
            bv3_ = const_cast<Pochoir_Array<T, N_RANK, TOGGLE> &>(orig).bv_3D(); 
            data_ = view_->data();
            l_null = (T*) calloc(1, sizeof(T));
		}

        /* assignment operator for vector<> */
		Pochoir_Array<T, N_RANK, TOGGLE> & operator= (Pochoir_Array<T, N_RANK, TOGGLE> const & orig) {
			total_size_ = orig.total_size();
			for (int i = 0; i < N_RANK; ++i) {
				phys_size_[i] = orig.phys_size(i);
				logic_size_[i] = orig.logic_size(i);
				stride_[i] = orig.stride(i);
			}
			view_ = NULL;
			view_ = const_cast<Pochoir_Array<T, N_RANK, TOGGLE> &>(orig).view();
			view_->inc_ref();
            /* We also get the BValue function pointer from orig */
            bv1_ = const_cast<Pochoir_Array<T, N_RANK, TOGGLE> &>(orig).bv_1D(); 
            bv2_ = const_cast<Pochoir_Array<T, N_RANK, TOGGLE> &>(orig).bv_2D(); 
            bv3_ = const_cast<Pochoir_Array<T, N_RANK, TOGGLE> &>(orig).bv_3D(); 
            data_ = view_->data();
            l_null = (T*) calloc(1, sizeof(T));
            return *this;
		}

		/* destructor : free memory */
		~Pochoir_Array() {
			view_->dec_ref();
            free(l_null);
		}

		inline Storage<T> * view() {
			return view_;
		}

        inline T * data() { return data_; }
        /* return the function pointer which generates the boundary value! */
        BValue_1D bv_1D(void) { return bv1_; }
        BValue_2D bv_2D(void) { return bv2_; }
        BValue_3D bv_3D(void) { return bv3_; }

        /* guarantee that only one version of boundary function is registered ! */
        void registerBV(BValue_1D _bv1) { bv1_ = _bv1;  bv2_ = NULL; bv3_ = NULL; }
        void registerBV(BValue_2D _bv2) { bv2_ = _bv2;  bv1_ = NULL; bv3_ = NULL; }
        void registerBV(BValue_3D _bv3) { bv3_ = _bv3;  bv1_ = NULL; bv2_ = NULL; }

        void unregisterBV(void) { bv1_ = NULL;  bv2_ = NULL; bv3_ = NULL; }

        void registerDomain(grid_info<N_RANK> initial_grid) {
            for (int i = 0; i < N_RANK; ++i) {
                logic_start_[i] = initial_grid.x0[i];
                logic_end_[i] = initial_grid.x1[i];
                logic_size_[i] = initial_grid.x1[i] - initial_grid.x0[i];
            }
        }

		/* return size */
		int phys_size(int _dim) const { return phys_size_[_dim]; }
		int logic_size(int _dim) const { return logic_size_[_dim]; }
        /* the size() function is for user's convenience! */
		int size(int _dim) const { return phys_size_[_dim]; }

		/* return total_size_ */
		int total_size() const { return total_size_; }

		/* return stride */
		int stride (int _dim) const { return stride_[_dim]; }

        inline bool check_boundary(size_info const & _idx) const {
            bool touch_boundary = false;
            for (int i = 0; i < N_RANK; ++i) {
                touch_boundary |= (_idx[i] < logic_start_[i]
                                || _idx[i] >= logic_end_[i]);
            }
            return touch_boundary;
        }

        inline bool check_boundary(int _idx1, int _idx0) {
            return (_idx0 < logic_start_[0] || _idx0 >= logic_end_[0]);
        }

        inline bool check_boundary(int _idx2, int _idx1, int _idx0) {
            return (_idx0 < logic_start_[0] || _idx0 >= logic_end_[0]
                 || _idx1 < logic_start_[1] || _idx1 >= logic_end_[1]);
        }

        inline bool check_boundary(int _idx3, int _idx2, int _idx1, int _idx0) {
            return (_idx0 < logic_start_[0] || _idx0 >= logic_end_[0]
                 || _idx1 < logic_start_[1] || _idx1 >= logic_end_[1]
                 || _idx2 < logic_start_[2] || _idx2 >= logic_end_[2]);
        }

        /* 
         * orig_value() is reserved for "ostream" : cout << Pochoir_Array
         */
        inline T orig_value (int _timestep, size_info & _idx) {
            bool l_boundary = check_boundary(_idx);
            bool set_boundary = false;
            T l_bvalue = 0;
            if (l_boundary && bv1_ != NULL) {
                l_bvalue = bv1_(*this, _timestep, _idx[0]);
                set_boundary = true;
            } else if (l_boundary && bv2_ != NULL) {
                l_bvalue = bv2_(*this, _timestep, _idx[1], _idx[0]);
                set_boundary = true;
            } else if (l_boundary && bv3_ != NULL) {
                l_bvalue = bv3_(*this, _timestep, _idx[2], _idx[1], _idx[0]);
                set_boundary = true;
            }
            /* the highest dimension is time dimension! */
            int l_idx = cal_index<N_RANK>(_idx, stride_) + toggle_base<TOGGLE>(_timestep) * total_size_;
            return (set_boundary) ? l_bvalue : (*view_)[l_idx];
        }

		/* index operator() for the format of a(i, j, k) 
         * - The highest dimension is always time dimension
         * - this is the uninterior version
         */
#if 0
		inline SProxy<T> operator() (int _idx1, int _idx0) const {
            bool l_boundary = check_boundary(_idx1, _idx0);
            /* we have to guard the use of bv_ by conditional, 
             * otherwise it may lead to some segmentation fault!
             */
            (*l_null) = (l_boundary && bv1_ != NULL) ? bv1_(*this, _idx1, _idx0) : (*l_null);
            bool set_boundary = (l_boundary && bv1_ != NULL);
			int l_idx = _idx0 * stride_[0] + toggle_base<TOGGLE>(_idx1) * total_size_;
			return SProxy<T>((*view_)[l_idx], set_boundary, (*l_null));
		}

		inline SProxy<T> operator() (int _idx2, int _idx1, int _idx0) const {
            bool l_boundary = check_boundary(_idx2, _idx1, _idx0);
            (*l_null) = (l_boundary && bv2_ != NULL) ? bv2_(*this, _idx2, _idx1, _idx0) : (*l_null);
            bool set_boundary = (l_boundary && bv2_ != NULL);
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + toggle_base<TOGGLE>(_idx2) * total_size_;
			return SProxy<T>((*view_)[l_idx], set_boundary, (*l_null));
		}

		inline SProxy<T> operator() (int _idx3, int _idx2, int _idx1, int _idx0) const {
            bool l_boundary = check_boundary(_idx3, _idx2, _idx1, _idx0);
            (*l_null) = (l_boundary && bv3_ != NULL) ? bv3_(*this, _idx3, _idx2, _idx1, _idx0) : (*l_null);
            bool set_boundary = (l_boundary && bv3_ != NULL);
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + _idx2 * stride_[2] + toggle_base<TOGGLE>(_idx3) * total_size_;
			return SProxy<T>((*view_)[l_idx], set_boundary, (*l_null));
		}

		inline SProxy<T> operator() (int _idx1, int _idx0) {
            bool l_boundary = check_boundary(_idx1, _idx0);
            (*l_null) = (l_boundary && bv1_ != NULL) ? bv1_(*this, _idx1, _idx0) : (*l_null);
            bool set_boundary = (l_boundary && bv1_ != NULL);
			int l_idx = _idx0 * stride_[0] + toggle_base<TOGGLE>(_idx1) * total_size_;
			return SProxy<T>((*view_)[l_idx], set_boundary, (*l_null));
		}

		inline SProxy<T> operator() (int _idx2, int _idx1, int _idx0) {
            bool l_boundary = check_boundary(_idx2, _idx1, _idx0);
            (*l_null) = (l_boundary && bv2_ != NULL) ? bv2_(*this, _idx2, _idx1, _idx0) : (*l_null);
            bool set_boundary = (l_boundary && bv2_ != NULL);
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + toggle_base<TOGGLE>(_idx2) * total_size_;
			return SProxy<T>((*view_)[l_idx], set_boundary, (*l_null));
		}

		inline SProxy<T> operator() (int _idx3, int _idx2, int _idx1, int _idx0) {
            bool l_boundary = check_boundary(_idx3, _idx2, _idx1, _idx0);
            (*l_null) = (l_boundary && bv3_ != NULL) ? bv3_(*this, _idx3, _idx2, _idx1, _idx0) : (*l_null);
            bool set_boundary = (l_boundary && bv3_ != NULL);
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + _idx2 * stride_[2] + toggle_base<TOGGLE>(_idx3) * total_size_;
			return SProxy<T>((*view_)[l_idx], set_boundary, (*l_null));
		}
#else
		inline T operator() (int _idx1, int _idx0) const {
            bool l_boundary = check_boundary(_idx1, _idx0);
            /* we have to guard the use of bv_ by conditional, 
             * otherwise it may lead to some segmentation fault!
             */
            bool set_boundary = (l_boundary && bv1_ != NULL);
            T l_bvalue = (set_boundary) ? bv1_(*this, _idx1, _idx0) : (*l_null);
			int l_idx = _idx0 * stride_[0] + toggle_base<TOGGLE>(_idx1) * total_size_;
            return (set_boundary ? (l_bvalue) : (*view_)[l_idx]);
		}

		inline T operator() (int _idx2, int _idx1, int _idx0) const {
            bool l_boundary = check_boundary(_idx2, _idx1, _idx0);
            bool set_boundary = (l_boundary && bv2_ != NULL);
            T l_bvalue = (set_boundary) ? bv2_(*this, _idx2, _idx1, _idx0) : (*l_null);
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + toggle_base<TOGGLE>(_idx2) * total_size_;
            return (set_boundary ? l_bvalue : (*view_)[l_idx]);
		}

		inline T operator() (int _idx3, int _idx2, int _idx1, int _idx0) const {
            bool l_boundary = check_boundary(_idx3, _idx2, _idx1, _idx0);
            bool set_boundary = (l_boundary && bv3_ != NULL);
            T l_bvalue = (set_boundary) ? bv3_(*this, _idx3, _idx2, _idx1, _idx0) : (*l_null);
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + _idx2 * stride_[2] + toggle_base<TOGGLE>(_idx3) * total_size_;
            return (set_boundary ? l_bvalue : (*view_)[l_idx]);
		}

		inline T & operator() (int _idx1, int _idx0) {
            bool l_boundary = check_boundary(_idx1, _idx0);
            bool set_boundary = (l_boundary && bv1_ != NULL);
            T l_bvalue = (set_boundary) ? bv1_(*this, _idx1, _idx0) : (*l_null);
			int l_idx = _idx0 * stride_[0] + toggle_base<TOGGLE>(_idx1) * total_size_;
            return (set_boundary ? l_bvalue : (*view_)[l_idx]);
		}

		inline T & operator() (int _idx2, int _idx1, int _idx0) {
            bool l_boundary = check_boundary(_idx2, _idx1, _idx0);
            bool set_boundary = (l_boundary && bv2_ != NULL);
            T l_bvalue = (set_boundary) ? bv2_(*this, _idx2, _idx1, _idx0) : (*l_null);
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + toggle_base<TOGGLE>(_idx2) * total_size_;
            return (set_boundary ? l_bvalue : (*view_)[l_idx]);
		}

		inline T & operator() (int _idx3, int _idx2, int _idx1, int _idx0) {
            bool l_boundary = check_boundary(_idx3, _idx2, _idx1, _idx0);
            bool set_boundary = (l_boundary && bv3_ != NULL);
            T l_bvalue = (set_boundary) ? bv3_(*this, _idx3, _idx2, _idx1, _idx0) : (*l_null);
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + _idx2 * stride_[2] + toggle_base<TOGGLE>(_idx3) * total_size_;
            return (set_boundary ? l_bvalue : (*view_)[l_idx]);
		}
#endif
        /* set()/get() pair to set/get boundary value in user supplied bvalue function */
		inline T & set (int _idx1, int _idx0) {
			int l_idx = _idx0 * stride_[0] + toggle_base<TOGGLE>(_idx1) * total_size_;
			return (*view_)[l_idx];
		}

		inline T & set (int _idx2, int _idx1, int _idx0) {
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + toggle_base<TOGGLE>(_idx2) * total_size_;
			return (*view_)[l_idx];
		}

		inline T & set (int _idx3, int _idx2, int _idx1, int _idx0) {
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + _idx2 * stride_[2] + toggle_base<TOGGLE>(_idx3) * total_size_;
			return (*view_)[l_idx];
		}

		inline T & get (int _idx1, int _idx0) {
			int l_idx = _idx0 * stride_[0] + toggle_base<TOGGLE>(_idx1) * total_size_;
			return (*view_)[l_idx];
		}

		inline T & get (int _idx2, int _idx1, int _idx0) {
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + toggle_base<TOGGLE>(_idx2) * total_size_;
			return (*view_)[l_idx];
		}

		inline T & get (int _idx3, int _idx2, int _idx1, int _idx0) {
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + _idx2 * stride_[2] + toggle_base<TOGGLE>(_idx3) * total_size_;
			return (*view_)[l_idx];
		}

        /* pointer() is for SIter */
		inline T * pointer (int _idx1, int _idx0) {
			int l_idx = _idx0 * stride_[0] + toggle_base<TOGGLE>(_idx1) * total_size_;
			return data_ + l_idx;
		}

		inline T * pointer (int _idx2, int _idx1, int _idx0) {
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + toggle_base<TOGGLE>(_idx2) * total_size_;
			return data_ + l_idx;
		}

		inline T * pointer (int _idx3, int _idx2, int _idx1, int _idx0) {
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + _idx2 * stride_[2] + toggle_base<TOGGLE>(_idx3) * total_size_;
			return data_ + l_idx;
		}

		/* index operator() for the format of a.interior(i, j, k) 
         * - The highest dimension is always time dimension
         * - this is the interior (non-checking) version
         */
		inline T interior (int _idx1, int _idx0) const {
			int l_idx = _idx0 * stride_[0] + toggle_base<TOGGLE>(_idx1) * total_size_;
			return (*view_)[l_idx];
		}

		inline T interior (int _idx2, int _idx1, int _idx0) const {
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + toggle_base<TOGGLE>(_idx2) * total_size_;
			return (*view_)[l_idx];
		}

		inline T interior (int _idx3, int _idx2, int _idx1, int _idx0) const {
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + _idx2 * stride_[2] + toggle_base<TOGGLE>(_idx3) * total_size_;
			return (*view_)[l_idx];
		}

		inline T & interior (int _idx1, int _idx0) {
			int l_idx = _idx0 * stride_[0] + toggle_base<TOGGLE>(_idx1) * total_size_;
			return (*view_)[l_idx];
		}

		inline T & interior (int _idx2, int _idx1, int _idx0) {
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + toggle_base<TOGGLE>(_idx2) * total_size_;
			return (*view_)[l_idx];
		}

		inline T & interior (int _idx3, int _idx2, int _idx1, int _idx0) {
			int l_idx = _idx0 * stride_[0] + _idx1 * stride_[1] + _idx2 * stride_[2] + toggle_base<TOGGLE>(_idx3) * total_size_;
			return (*view_)[l_idx];
		}

		/* size_info is of type int[] */
		static inline bool update_index(int * index, bool & line_break, int const * head_index, int const * tail_index)
		{
			int i = 0;
			bool done = false, whole_done = false;
			while (!done && i < N_RANK) {
				if (index[i] == (tail_index[i] - 1)) {
					index[i] = head_index[i];
					line_break = true;
					if (i == N_RANK-1)
						whole_done = true;
					i++;
				} else {
					index[i]++;
					done = true;
				}
			}
			return whole_done;
		}

		template <typename T2, int N2>
		friend std::ostream& operator<<(std::ostream& os, Pochoir_Array<T2, N2> const & x); 
};

template<typename T2, int N2>
std::ostream& operator<<(std::ostream& os, Pochoir_Array<T2, N2> const & x) { 
	typedef int size_info[N2];
	size_info l_index, l_head_index, l_tail_index;
	bool done = false, line_break = false;
	int i = 0;

	os << " Pochoir_Array : "; 
	for (int i = 0; i < N2; ++i) {
		l_index[i] = 0;
		l_head_index[i] = 0;
		l_tail_index[i] = x.phys_size(i);
		os << "Dim " << i << ", size<" << x.phys_size(i) << "> ; ";
	}
	os << std::endl;

	while (!done) {
		T2 x0, x1;
		x0 = const_cast<Pochoir_Array<T2, N2> &>(x).orig_value(0, l_index);
		x1 = const_cast<Pochoir_Array<T2, N2> &>(x).orig_value(1, l_index);
		os << std::setw(9) << x0 << " (" << x1 << ")" << " "; 
		done = const_cast<Pochoir_Array<T2, N2> &>(x).update_index(l_index, line_break, l_head_index, l_tail_index);
		if (line_break) {
			os << std::endl;
			line_break = false;
		}
	}
	return os; 
}
#endif // POCHOIR_ARRAY_H
