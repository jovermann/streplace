// tmap<> class template -- improved stl map<>
//
// Copyright (c) 2000-2001 Johannes Overmann
//
// This file is released under the MIT license. See LICENSE for license.

#ifndef _ngw_tmap_h_
#define _ngw_tmap_h_


#ifdef DONT_USE_STL
# include "ttmap.h"
# define tmap_base ttmap
#else
# include <map>
# define tmap_base map
using namespace std;
#endif


#include "texception.h"

// history: start 08 Jul 2000
// 2000:
// 08 Jul: removing tarray and tassocarray, this should provide tmap and tvector
// 2001:
// 15 Sep: DONT_USE_STL feature added
// 16 Sep: splittet tstl.h into tvector.h and tmap.h


template<class K, class T>
class tmap: public tmap_base<K,T> {
 public:
   // 1:1 wrapper

   /// access element (read/write) (this is needed for some strange reason)
   T& operator[](const K& key) { return tmap_base<K,T>::operator[](key); };

   // new functionality

   /// return whether an element with key is contained or not
    bool contains(const K& key) const { return this->find(key) != tmap_base<K,T>::end(); }
   /// access element read only (const)
// g++ 2.95.2 does not allow this:
// const T& operator[](const K& key) const { const_iterator i = this->find(key); if(i != end()) return i->second; else throw TNotFoundException(); } // throw(TNotFoundException)
   const T& operator[](const K& key) const { if(contains(key)) return this->find(key)->second; else throw TNotFoundException(); } // throw(TNotFoundException)
};


#endif /* _ngw_tmap_h_ */

