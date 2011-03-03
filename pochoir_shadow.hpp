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

#ifndef POCHOIR_SHADOW_HPP
#define POCHOIR_SHADOW_HPP

#include <iostream>

using namespace std;

template <typename T, int N_RANK, int TOGGLE>
class ShadowRef {};

template <typename T, int TOGGLE>
class ShadowRef <T, 1, TOGGLE> {
    private:
        T & ref_;
        SArray<T, 1, TOGGLE> & arr_;
        typedef T (*BValue_1D)(SArray<T, 1> &, size_t, size_t);
        BValue_1D bv1_;
        size_t t_, i_;
        size_t slope_[1], size_[1];
    public:
        explicit ShadowRef (T & _ref, SArray<T, 1, TOGGLE> & _arr, size_t _t, size_t _i) : ref_(_ref), arr_(_arr), t_(_t), i_(_i) { 
            bv1_ = NULL;
        }
        
        void operator= (T const & rhs) {
            cout << "Writing..." << endl;
            ref_ = rhs;
        }

#define check_boundary_1D(_idx1, _idx0) \
    if (_idx0 < 0 + slope_[0] || _idx0 > size_[0]-1-slope_[0]) { \
        if (bv1_ != NULL) { \
            bv1_(arr_, _idx1, _idx0); \
            return arr_(_idx1, _idx0);
        } \
    }

        operator T const &() const {
            cout << "Reading..." << endl;
            for (int i = 0; i < 1; ++i) {
                size_[i] = arr_.phys_size(i);
                slope_[i] = arr_.slope(i);
            }
            bv1_ = arr_.bv_1D();
            check_boundary_1D(t_, i_);
            /* We can't Refetch after set in check_boundary(), 
             * otherwise, it will trap into circular call stack!!!
             */
            return arr_(t_, i_);
        }
};

template <typename T, int TOGGLE>
class ShadowRef <T, 2, TOGGLE> {
    private:
        T & ref_;
        SArray<T, 2, TOGGLE> & arr_;
        typedef T (*BValue_1D)(SArray<T, 2> &, size_t, size_t);
        BValue_1D bv1_;
        size_t t_, i_, j_;
        size_t slope_[2], size_[2];
        T bvalue_;
    public:
        explicit ShadowRef (T & _ref, SArray<T, 2, TOGGLE> & _arr, size_t _t, size_t _i, size_t _j) : ref_(_ref), arr_(_arr), t_(_t), i_(_i), j_(_j) { 
            bv1_ = NULL;
            bvalue_ = 0;
        }
        
        void operator= (T const & rhs) {
            cout << "Writing..." << endl;
            ref_ = rhs;
        }

#define check_boundary_2D(_idx2, _idx1, _idx0) \
    if (_idx0 < 0 + slope_[0] || _idx0 > logic_size_[0]-1-slope_[0] \
        || _idx1 < 0 + slope_[1] || _idx1 > logic_size_[1]-1-slope_[1]) { \
        if (bv2_ != NULL) { \
            bv2_(*this, _idx2, _idx1, _idx0); \
        } \
    }
        operator T const &() const {
            cout << "Reading..." << endl;
            for (int i = 0; i < 2; ++i) {
                size_[i] = arr_.phys_size(i);
                slope_[i] = arr_.slope(i);
            }
            bvalue_ = ref_;
            bv2_ = arr_.bv_2D();
            check_boundary_2D(t_, i_, j_);
            return bvalue_;
        }
};

#endif
