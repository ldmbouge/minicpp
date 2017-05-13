#ifndef __REVERSIBLE_H
#define __REVERSIBLE_H

#include <memory>
#include "engine.hpp"

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

template<class T> class revList {
   Engine::Ptr _ctx;
public:
   struct revNode {
      revList<T>*   _owner;
      rev<revNode*> _prev;
      rev<revNode*> _next;
      T            _value;
      revNode(revList<T>* own,Engine::Ptr ctx,revNode* p,revNode* n,T&& v)
         : _owner(own),_prev(ctx,p),_next(ctx,n),_value(std::move(v)) {
         if (p) p->_next = this;
         if (n) n->_prev = this;
      }
      void detach() {
         revNode* p = _prev;
         revNode* n = _next;
         if (p) p->_next = n;
         else _owner->_head = n;
         if (n) n->_prev = p;
      }
   };
   class iterator {
      friend class revList<T>;
      revNode* _cur;
   protected:
      iterator(revNode* c) : _cur(c) {}
   public:
      T operator*() const  { return _cur->_value;}
      T& operator*()       { return _cur->_value;}
      const iterator& operator++() { _cur = _cur->_next;return *this;}
      iterator operator++(int) { iterator copy(_cur);_cur = _cur->_next;return copy;}
      bool operator==(const iterator& other) const { return _cur == other._cur;}
      bool operator!=(const iterator& other) const { return _cur != other._cur;}
   };
private:
   rev<revNode*> _head;
public:
   revList(Engine::Ptr ctx) : _ctx(ctx),_head(ctx,nullptr) {}
   revNode* emplace_back(T&& v) {
      return _head = new revNode(this,_ctx,nullptr,_head,std::move(v));
   }
   iterator begin()  { return iterator(_head);}
   iterator end()    { return iterator(nullptr);}
};

#endif
