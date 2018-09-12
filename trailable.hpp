#ifndef __TRAILABLE_H
#define __TRAILABLE_H

#include "trail.hpp"

template<class T> class trail {
   Trailer::Ptr       _ctx;
   int              _magic;
   T                _value;
   void save(int nm) {
      _magic = nm;
      Entry* entry = new (_ctx) TrailEntry(&_value);
      _ctx->trail(entry);
   }
public:
   trail() : _ctx(nullptr),_magic(-1),_value(T())  {}
   trail(Trailer::Ptr ctx,const T& v = T()) : _ctx(ctx),_magic(ctx->magic()),_value(v) {}
   operator T() const { return _value;}
   T value() const { return _value;}
   trail<T>& operator=(const T& v);
   class TrailEntry: public Entry {
      T*  _at;
      T  _old;
   public:
      TrailEntry(T* at) : _at(at),_old(*at) {}
      void restore() { *_at = _old;}
   };
};

template<class T>
trail<T>& trail<T>::operator=(const T& v)
{
   int cm = _ctx->magic();
   if (_magic != cm)
      save(cm);    
   _value = v;
   return *this;        
}

#endif
