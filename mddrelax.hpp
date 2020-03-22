#ifndef __MDDRELAX_H
#define __MDDRELAX_H

#include "mdd.hpp"
#include "trailable.hpp"
#include "mddnode.hpp"
#include <set>
#include <tuple>
#include <random>

class MDDNodeSet {
   const int  _msz;
   int         _sz;
   MDDNode** _data;
public:
   MDDNodeSet() : _msz(0) { _sz = 0;_data = nullptr;}
   MDDNodeSet(int sz) : _msz(sz),_sz(0) {
      _data = new MDDNode*[_msz];
   }
   MDDNodeSet(MDDNodeSet&& other) : _msz(other._msz),_sz(other._sz)
   {
      _data = other._data;
      other._data = nullptr;
   }
   ~MDDNodeSet() {
      if (_data) delete[] _data;
   }
   bool member(MDDNode* p) const noexcept {
      for(int i=_sz-1;i>=0;i--) 
         if (_data[i] == p)
            return true;
      return false;
   }
   void insert(MDDNode* n) {
      if (!member(n)) {
         assert(_sz < _msz);
         _data[_sz++] = n;
      }
   }
   MDDNodeSet& operator=(const MDDNodeSet& other) {
      _sz = other._sz;
      memcpy(_data,other._data,sizeof(MDDNode*)*other._sz);
      return *this;
   }
   int size() const noexcept { return _sz;}  
   class iterator: public std::iterator<std::input_iterator_tag,MDDNode*,long> {
      MDDNode** _data;
      long       _idx;
      iterator(MDDNode** d,long idx=0) : _data(d),_idx(idx) {}
   public:
      iterator& operator++()   { _idx = _idx + 1; return *this;}
      iterator operator++(int) { iterator retval = *this; ++(*this); return retval;}
      iterator& operator--()   { _idx = _idx - 1; return *this;}
      iterator operator--(int) { iterator retval = *this; --(*this); return retval;}
      bool operator==(iterator other) const {return _idx == other._idx;}
      bool operator!=(iterator other) const {return !(*this == other);}
      MDDNode*& operator*() const noexcept { return _data[_idx];}
      friend class MDDNodeSet;
   };
   auto begin() const noexcept { return iterator(_data,0);}
   auto end() const noexcept { return iterator(_data,_sz);}
   void unionWith(const MDDNodeSet& other) {
      for(auto n : other) {
         if (!member(n)) {
            assert(_sz < _msz);
            _data[_sz++] = n;
         }
      }
   }
};

struct MDDNodePtrOrder {
   bool operator()(const MDDNode* a,const MDDNode* b)  const noexcept {
      return a->getId() < b->getId();
   }
};

class MDDRelax : public MDD {
   const unsigned int _width;
   ::trail<unsigned> _lowest;
   std::mt19937 _rnG;
   std::uniform_real_distribution<double> _sampler;
   std::vector<MDDState> _refs;
   const MDDState& pickReference(int layer,int layerSize); 
   void rebuild();
   bool refreshNode(MDDNode* n,int l);
   MDDNodeSet split(TVec<MDDNode*>& layer,int l);
   void spawn(MDDNodeSet& delta,TVec<MDDNode*>& layer,unsigned int l);
   MDDNode* findSimilar(const std::multimap<float,MDDNode*>& layer,const MDDState& s,const MDDState& refDir);
   MDDNode* resetState(MDDNode* from,MDDNode* to,MDDState& s,int v,int l);
   void delState(MDDNode* state,int l);
   void computeUp();
public:
   MDDRelax(CPSolver::Ptr cp,int width = 32);
   void trimDomains() override;
   void buildDiagram() override;
   void relaxLayer(int i);
   void propagate() override;
   void trimLayer(unsigned int layer) override;
   void debugGraph() override;
   const MDDState& ref(int l) const { return _refs[l];}
   void printRefs() {
      for(auto i=0u;i < numVariables;i++)
         std::cout << "R[" << i << "] = " << ref(i) << std::endl;
   }
};

#endif
