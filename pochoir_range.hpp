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

#ifndef POCHOIR_RANGE_H
#define POCHOIR_RANGE_H

#include <cstddef>
#include <cassert>
#include <iostream>

/* Pochoir Range */
class Pochoir_pRange {
	protected:
		int first_, last_, stride_;
		int index_, shift_;

	public:
		typedef int T_numtype;

		Pochoir_pRange() : first_(0), last_(0), stride_(1), shift_(0), index_(0) { }

		Pochoir_pRange(Pochoir_pRange const & r) {
			first_ = r.first();
			last_ = r.last();
			stride_ = r.stride();
			shift_ = r.shift();
			index_ = first_;
		}

		Pochoir_pRange(int first, int last, int stride=1, int shift=0)
			: first_(first), last_(last), stride_(stride), shift_(shift), index_(first) {}

		int first() const { 
			return first_; 
		}

		int last() const {
			return last_;
		}

		int stride() const { 
			return stride_; 
		}

		int shift() const {
			return shift_;
		}

		inline int size() const {
			return (last_ - first_) / stride_ + 1;
		}

		bool isUnitStride() const { 
			return stride_ == 1; 
		}

		/* We don't change the original 'range' */
		inline Pochoir_pRange const operator-(int shift) const { 
			return Pochoir_pRange(first_ - shift, last_ - shift, stride_, shift); 
		}

		/* We don't change the original 'range' */
		inline Pochoir_pRange const operator+(int shift) const { 
			return Pochoir_pRange(first_ + shift, last_ + shift, stride_, shift); 
		}

		/* put a 'virtual' keyword , and 'inheritance' to rewrite the function 
		 * in base class is a BAD BAD idea, which incurs very heavy overhead
		*/
		inline int operator() (int _idx) const {
			return (first_ + _idx * stride_);
		}

		inline int operator[] (int _idx) const {
			return (first_ + _idx * stride_);
		}

		friend std::ostream& operator<<(std::ostream& os, Pochoir_pRange const & range);
};

/* unit-stride Range */
class Pochoir_Domain {
	protected:
		int first_, last_;
		int index_, shift_;

	public:
		Pochoir_Domain() : first_(0), last_(0), index_(0), shift_(0) { }

		Pochoir_Domain(Pochoir_Domain const & r) {
			first_ = r.first();
			last_ = r.last();
			index_ = first_;
			shift_ = r.shift();
		}

        /* Now Pochoir_Domain is of [a, b) */
		Pochoir_Domain(int first, int last, int shift=0)
			: first_(first), last_(last), index_(first), shift_(shift) {}

		int first() const { 
			return first_; 
		}

		int last() const {
			return last_;
		}

		int stride() const { 
			return 1; 
		}

		int shift() const {
			return shift_;
		}

        /* Now Pochoir_Domain is of [a, b) */
		inline int size() const {
			return (last_ - first_);
		}

		bool isUnitStride() const { 
			return true; 
		}

		/* We don't change the original 'range' */
		inline Pochoir_Domain const operator-(int shift) const { 
			return Pochoir_Domain(first_ - shift, last_ - shift, shift); 
		}

		/* We don't change the original 'range' */
		inline Pochoir_Domain const operator+(int shift) const { 
			return Pochoir_Domain(first_ + shift, last_ + shift, shift); 
		}

		inline int operator() (int _idx) const {
			return (first_ + _idx);
		}

		inline int operator[] (int _idx) const {
			return (first_ + _idx);
		}

		friend std::ostream& operator<<(std::ostream& os, Pochoir_Domain const & range);
};

#if 0
std::ostream& operator<<(std::ostream& os, Pochoir_pRange const & range)
{
	os << "Pochoir_pRange(" 
		<< range.first() << "," << range.last() << "," << range.stride() << ")" 
		<< std::endl;
	return os;
}

std::ostream& operator<<(std::ostream& os, Pochoir_Domain const & range)
{
	os << "Pochoir_Domain(" 
		<< range.first() << "," << range.last() << "," << range.stride() << ")" 
		<< std::endl;
	return os;
}
#endif

#endif /* POCHOIR_RANGE_H */
