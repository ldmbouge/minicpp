#ifndef __REVERSIBLE_H
#define __REVERSIBLE_H

#include <memory>
#include "trail.hpp"

template<class T> class rev {
   Context::Ptr  _ctx;
   int         _magic;
   T           _value;
public:
   rev() : _ctx(nullptr),_magic(-1),_value(T())  {}
   rev(Context::Ptr ctx,const T& v = T()) : _ctx(ctx),_magic(ctx->magic()),_value(v) {}
   operator T() const { return _value;}
   T value() const { return _value;}
   rev<T>& operator=(const T& v);
   class RevEntry: public Entry {
      T*  _at;
      T  _old;
   public:
      RevEntry(T* at) : _at(at),_old(*at) {}
      void restore() { *_at = _old;}
   };
   void trail(int nm) {
      _magic = nm;
      Entry* entry = new (_ctx) RevEntry(&_value);
      _ctx->trail(entry);
   }
};

template<class T>
rev<T>& rev<T>::operator=(const T& v)
{
   int cm = _ctx->magic();
   if (_magic != cm)
      trail(cm);    
   _value = v;
   return *this;        
}

#endif
