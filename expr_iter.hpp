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

#ifndef EXPR_ITER_HPP
#define EXPR_ITER_HPP

#include "expr_common.hpp"
//#include "expr_array.hpp"
template <typename T, int N_RANK, int TOGGLE> class Pochoir_SArray;

template <typename T, int N_RANK, int TOGGLE=2>
class interior_shadow {
    private:
        Pochoir_SArray<T, N_RANK, TOGGLE> & arr_;
    public:
        explicit interior_shadow(Pochoir_SArray<T, N_RANK, TOGGLE> & _arr) : arr_(_arr) {}
        explicit interior_shadow(interior_shadow<T, N_RANK, TOGGLE> & _shadow) : arr_(_shadow.getArray()) {}
        Pochoir_SArray<T, N_RANK, TOGGLE> & getArray() { return arr_; }
        inline T & operator() (int _t, int _i) {
            return arr_.interior(_t, _i);
        }
        inline T & operator() (int _t, int _i, int _j) {
            return arr_.interior(_t, _i, _j);
        }
        inline T & operator() (int _t, int _i, int _j, int _k) {
            return arr_.interior(_t, _i, _j, _k);
        }
};

template <typename T, int N_RANK, int TOGGLE=2>
class Pochoir_Iterator {
    /* This is the safe version, we don't check index range for 
     * each type conversion or assignment!!
     * For unsafe access, we leave it to the index version!
     */
    private:
        Pochoir_SArray<T, N_RANK, TOGGLE> & arr_;
        T * curr_;
    public:
        explicit Pochoir_Iterator(Pochoir_SArray<T, N_RANK, TOGGLE> & _arr) : arr_(_arr) {
            curr_ = arr_.view()->data();
        }

        inline void set(int _t, int _i, int _j) {
            curr_ = arr_.pointer(_t, _i, _j);
        }

        inline void set(int _t, int _i) {
            curr_ = arr_.pointer(_t, _i);
        }

        inline void inc(int _gap) {
            curr_ += _gap;
        }

        inline void operator++() {
            ++curr_;
        }

        inline void operator++(int) {
            ++curr_;
        }

        inline operator T() const {
            /* type conversion only appears on the right side of '=' */
            return *curr_;
        }

        inline Pochoir_Iterator<T, N_RANK> & operator= (T const & rhs) {
            /* overloaded assignment, for reference appears on the left side of '=' */
            *curr_ = rhs;
            return *this;
        }

        inline T operator*() const {
            return *curr_;
        }

        inline T & operator*() {
            return *curr_;
        }
};

template <typename T>
class SProxy {
    /* This is the safe version, we don't check index range for 
     * each type conversion or assignment!!
     * For unsafe access, we leave it to the index version!
     */
    private:
        T & value_;
        bool set_boundary_;
        T bvalue_;
    public:
        explicit SProxy(T & _v, bool _set_boundary, T const & _bvalue) : value_(_v), set_boundary_(_set_boundary), bvalue_(_bvalue) { }

        inline operator T() const {
            /* type conversion only appears on the right side of '=' */
            return (set_boundary_) ? bvalue_ : value_;
        }

        inline SProxy<T> & operator= (T const & rhs) {
            /* overloaded assignment, for reference appears on the left side of '=' 
             * Because currently, this Proxy can only be called from BValue point,
             * so we just read, we dont write!
             * - We could NOT prevent the assignment of l-value from '=',
             *   otherwise the Periodic stencil won't get the boundary value.
             *   For our version, the assignment always proceeds, and get function
             *   is only for read!!!
             */
            
            value_ = rhs;
            set_boundary_ = false;
            return *this;
        }

        inline T & value() { return value_; }
        inline T const & value() const { return value_; }
        inline bool set_boundary() const { return set_boundary_; }
        inline T bvalue() const { return bvalue_; }

        /* this assignment operator= is for statement like: 
         * a(t, i) = a(t, i-1) = 0; 
         */
        inline SProxy & operator=(SProxy const & rhs) {
            value_ = rhs.value();
            set_boundary_ = rhs.set_boundary();
            bvalue_ = rhs.bvalue();
            return *this;
        }
};

#endif

