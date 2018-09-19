#ifndef __MATRIX_H
#define __MATRIX_H

#include <vector>
#include <array>

template <class T,int a> class matrix;

template <class T,int a,int d> class MSlice {
   matrix<T,a>* _mtx;
   int         _flat;      
   MSlice(matrix<T,a>* m,int idx) : _mtx(m),_flat(idx) {}      
public:
   friend class matrix<T,a>;
   friend class MSlice<T,a,d+1>;
   MSlice<T,a,d-1> operator[](int i);
   const MSlice<T,a,d-1> operator[](int i) const;
   int size() const { return _mtx->_dim[a-d];}
};
template<class T,int a> class MSlice<T,a,1> { 
   matrix<T,a>* _mtx;
   int        _flat;      
   friend class MSlice<T,a,2>;
   MSlice(matrix<T,a>* m,int idx) : _mtx(m),_flat(idx) {}
public:        
   friend class matrix<T,a>;
   T& operator[](int i);
   const T& operator[](int i) const;
   int size() const { return _mtx->_dim[a-1];}
};

template<class T,int a> class matrix {
   std::vector<T>    _data;
   std::array<int,a>  _dim;
public:
   friend class MSlice<T,a,a-1>;
   matrix(std::initializer_list<int> li) {
      std::copy(li.begin(),li.end(),_dim.begin());
      int ttl = 1;
      for(int i=0;i <a;i++) 
         ttl *= _dim[i];      
      _data.resize(ttl);
   }
   const std::vector<T>& flat() const { return _data;}
   const int size(int d) const { return _dim[d];}
   MSlice<T,a,a-1> operator[](int i) { return MSlice<T,a,a-1>(this,i);}
};

template <class T,int a,int d>
MSlice<T,a,d-1> MSlice<T,a,d>::operator[](int i) {
   return MSlice<T,a,d-1>(_mtx,_flat * _mtx->_dim[a - d] + i);
}
template<class T,int a>
T& MSlice<T,a,1>::operator[](int i) {
   return _mtx->_data[_flat * _mtx->_dim[a - 1] + i];
}
template <class T,int a,int d>
const MSlice<T,a,d-1> MSlice<T,a,d>::operator[](int i) const {
   return MSlice<T,a,d-1>(_mtx,_flat * _mtx->_dim[a - d] + i);
}
template<class T,int a>
const T& MSlice<T,a,1>::operator[](int i) const {
   return _mtx->_data[_flat * _mtx->_dim[a - 1] + i];
}

template <class T,typename Body> std::vector<T> slice(int l,int u,Body b) {
   std::vector<T> x;
   for(int k=l;k < u;k++)
      x.emplace_back(b(k));
   return x;
}

#endif
