// tvector<> class template -- improved stl vector<>
//
// Copyright (c) 2000-2001 Johannes Overmann
//
// This file is released under the MIT license. See LICENSE for license.

#ifndef _ngw_tvector_h_
#define _ngw_tvector_h_


#ifdef DONT_USE_STL
#include "ttvector.h"
#define tvector_base ttvector
#else
# include <vector>
# define tvector_base vector
using namespace std;
#endif


#include "texception.h"

// history: start 08 Jul 2000
// 2000:
// 08 Jul: removing tarray and tassocarray, this should provide tmap and tvector
// 2001:
// 15 Sep: DONT_USE_STL feature added
// 16 Sep: splittet tstl.h into tvector.h and tmap.h
// 20 Sep: clear() added

template<class T>
class tvector: public tvector_base<T> {
 public:
   // 1:1 wrapper
   
   /// create an empty vector 
   tvector():tvector_base<T>() {}
   /// create a vector with n elements
   tvector(size_t n):tvector_base<T>(n) {}
   /// create a vector with n copies of t
   tvector(size_t n, const T& t):tvector_base<T>(n, t) {}
   
   // new functionality
   
   /// append an element to the end
   const tvector& operator += (const T& a) { this->push_back(a); return *this; }
   /// append another tvector to the end
   const tvector& operator += (const tvector& a) { this->insert(tvector_base<T>::end(), a.tvector_base<T>::begin(), a.tvector_base<T>::end()); return *this; }
   /// direct read only access, safe
   const T& operator[](size_t i) const { if(i < tvector_base<T>::size()) return tvector_base<T>::operator[](i); else throw TZeroBasedIndexOutOfRangeException(i, tvector_base<T>::size()); } // throw(TZeroBasedIndexOutOfRangeException);
   /// direct read/write access, automatically create new elements
   T& operator[](size_t i) { if(i >= tvector_base<T>::size()) operator+=(tvector(i - tvector_base<T>::size() + 1)); return tvector_base<T>::operator[](i); }
   /// clear vector
   void clear() { this->erase(tvector_base<T>::begin(), tvector_base<T>::end()); }
};


#endif /* _ngw_tvector_h_ */

