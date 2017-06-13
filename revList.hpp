#ifndef __REVLIST_H
#define __REVLIST_H

#include "reversible.hpp"
#include "engine.hpp"

template<class T> class revList {
  Engine::Ptr _engine;
public:
  struct revNode {
    revList<T>*   _owner;
    rev<revNode*> _prev;
    rev<revNode*> _next;
    T            _value;
    revNode(revList<T>* own,Context::Ptr ctx,revNode* p,revNode* n,T&& v)
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
   revList(Engine::Ptr eng) : _engine(eng),_head(eng->getContext(),nullptr) {}
  ~revList() {
     _head = nullptr;
  }
  revNode* emplace_back(T&& v) {
     // Allocate list node on the stack allocator
     return _head = new (_engine->getStore()) revNode(this,_engine->getContext(),nullptr,_head,std::move(v));
  }
  iterator begin()  { return iterator(_head);}
  iterator end()    { return iterator(nullptr);}
  friend std::ostream& operator<<(std::ostream& os,const revList<T>& rl) {
    revNode* cur = rl._head;
    while (cur) {
      os << "," << cur;
      cur = cur->_next;
    }
    return os;
  }
};

#endif
