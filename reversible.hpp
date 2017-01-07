#ifndef __REVERSIBLE_H
#define __REVERSIBLE_H

#include <memory>
#include "solver.hpp"

template<class T> class rev {
    Engine::Ptr  _ctx;
    int        _magic;
    T          _value;
public:
    rev(Engine::Ptr ctx,const T& v)
        : _ctx(ctx),_magic(ctx->magic()),_value(v) {}
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
    void trail(int nm) { _magic = nm;_ctx->trail(std::make_unique<RevEntry>(&_value));}
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
