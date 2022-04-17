#ifndef __MDDRELAX_H
#define __MDDRELAX_H

#include "mdd.hpp"
#include "trailable.hpp"
#include "mddnode.hpp"
#include "mdddelta.hpp"
#include "queue.hpp"
#include <set>
#include <tuple>
#include <random>

class MDDNodeSet {
   MDDNode**   _data;
   const short  _msz;
   short         _sz;
   bool       _stack;
public:
   MDDNodeSet() : _data(nullptr),_msz(0),_sz(0),_stack(false) {}
   MDDNodeSet(int sz) : _msz(sz),_sz(0),_stack(false) {
      _data = new MDDNode*[_msz];
   }
   MDDNodeSet(int sz,char* buf) : _msz(sz),_sz(0),_stack(true) {
      _data = reinterpret_cast<MDDNode**>(buf);
   }
   MDDNodeSet(MDDNodeSet&& other) : _msz(other._msz),_sz(other._sz),_stack(other._stack)
   {
      _data = other._data;
      other._data = nullptr;
   }
   ~MDDNodeSet() {
      if (!_stack && _data) delete[] _data;
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
   MDDNode* pop() noexcept {
      assert(_sz > 0);
      MDDNode* retVal = _data[--_sz];
      return retVal;
   }
   MDDNodeSet& operator=(const MDDNodeSet& other) {
      _sz = other._sz;
      memcpy(_data,other._data,sizeof(MDDNode*)*other._sz);
      return *this;
   }
   int size() const noexcept { return _sz;}  
   class iterator  { //: public std::iterator<std::input_iterator_tag,MDDNode*,long> {
      MDDNode** _data;
      long       _idx;
      iterator(MDDNode** d,long idx=0) : _data(d),_idx(idx) {}
   public:
      using iterator_category = std::input_iterator_tag;
      using value_type = MDDNode*;
      using difference_type = long;
      using pointer = MDDNode**;
      using reference = MDDNode*&;
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


template <class op> class MDDQueue {
   CQueue<MDDNode*>* _queues;
   int _nbq;
   int _nbe;
   int _cl;
   int _init;
   enum Direction _dir;
public:
   MDDQueue(int nb) : _nbq(nb),_nbe(0),_cl(0) {
      _init = std::is_same<op,std::plus<int>>::value ? 0 : _nbq - 1;
      _dir  = std::is_same<op,std::plus<int>>::value ? Down : Up;
      _queues = new CQueue<MDDNode*>[_nbq];
   }
   ~MDDQueue()  { delete []_queues;}
   void clear() {
      assert(_nbe >= 0);
      if (_nbe > 0) {
         for(int i = 0;i < _nbq;i++)
            while (!_queues[i].empty())
               _queues[i].deQueue()->leaveQueue(_dir);
         _nbe = 0;
      }
   }
   void init() noexcept  { _cl = _init;}
   bool empty() const    { return _nbe == 0;}
   void retract(MDDNode* n) {
      assert(_nbe >= 1);
      assert(n->inQueue(_dir));
      auto& tc = _queues[n->getLayer()]; // queue to clear
      if (std::is_same<op,std::plus<int>>::value) {
         tc.retract(n->_fq);
         _nbe -= n->_fq != nullptr;
      } else {
         tc.retract(n->_bq);
         _nbe -= n->_bq != nullptr;
      }      
      n->leaveQueue(_dir);   
   }
   void enQueue(MDDNode* n) {
      assert(n->isActive());
      if (!n->inQueue(_dir)) {
         Location<MDDNode*>* lq = _queues[n->getLayer()].enQueue(n);
         assert(n->inQueue(_dir) == false);
         n->enterQueue(_dir);
         if (std::is_same<op,std::plus<int>>::value)
            n->_fq = lq;
         else n->_bq = lq;
         _nbe += 1;
      }
      assert(_nbe>=0);
   }
   MDDNode* deQueue() {
      op opName;
      MDDNode* rv = nullptr;
      do {
         while (_cl >= 0 && _cl < _nbq && _queues[_cl].empty())
            _cl = opName(_cl,1);
         if (_cl < 0 || _cl >= _nbq) {
            return nullptr;
         }
         rv = _queues[_cl].deQueue();
         if (rv)
            rv->leaveQueue(_dir);
         _nbe -= 1;
         assert(_nbe >= 0);
      } while (rv==nullptr || !rv->isActive());      
      return rv;
   }
};

using MDDFQueue = MDDQueue<std::plus<int>>;
using MDDBQueue = MDDQueue<std::minus<int>>;
class MDDSplitter;

class MDDRelax : public MDD {
   const unsigned int _width;
   const int    _maxDistance;
   const int    _maxSplitIter;
   const bool _approxThenExact;
   const int _maxConstraintPriority;
   ::trail<unsigned> _lowest;
   std::mt19937 _rnG;
   std::uniform_real_distribution<double> _sampler;
   std::vector<MDDState> _refs;
   MDDIntSet*             _afp;
   MDDNode**              _src;
   MDDFQueue*             _fwd;
   MDDBQueue*             _bwd;
   Pool::Ptr             _pool;
   MDDDelta*        _deltaDown;
   MDDDelta*          _deltaUp;
   MDDDelta*    _deltaCombined;
   int _domMin,_domMax;
   const MDDState& pickReference(int layer,int layerSize);
   void checkGraph();
   void fullStateDown(MDDState& ms,MDDState& cs,MDDNode* n,int l);
   void incrStateDown(const MDDPropSet& out,MDDState& ms,MDDState& cs,MDDNode* n,int l);
   void fullStateUp(MDDState& ms,MDDState& cs,MDDNode* n,int l);
   void incrStateUp(const MDDPropSet& out,MDDState& ms,MDDState& cs,MDDNode* n,int l);
   void fullStateCombined(MDDState& state,MDDNode* n);
   void incrStateCombined(const MDDPropSet& out,MDDState& state,MDDNode* n);
   bool updateCombinedIncrDown(MDDNode* n);
   bool updateCombinedIncrUp(MDDNode* n);
   void aggregateValueSet(MDDNode* n);
   bool refreshNodeIncr(MDDNode* n,int l);
   bool trimVariable(int i);
   bool filterKids(MDDNode* n,int l);
   bool filterParents(MDDNode* n,int l);
   int splitNode(MDDNode* n,int l,MDDSplitter& splitter);
   int splitNodeForConstraintPriority(MDDNode* n,int l,MDDSplitter& splitter, int constraintPriority);
   int splitNodeApprox(MDDNode* n,int l,MDDSplitter& splitter, int constraintPriority);
   void splitLayers(bool approximate, int constraintPriority = 0); // delta is essentially an out argument. 
   int delState(MDDNode* state,int l); // return lowest layer where a deletion occurred.
   bool processNodeUp(MDDNode* n,int i); // i is the layer number
   void computeUp();
   void computeDown(int iter);
   void postUp();
   void removeArc(int outL,int inL,MDDEdge* arc) override;
   void refreshAll() override;
   const MDDState& ref(int l) const noexcept { return _refs[l];}
public:
   MDDRelax(CPSolver::Ptr cp,int width = 32,int maxDistance = std::numeric_limits<int>::max(),int maxSplitIter = 5,bool approxThenExact = true, int maxConstraintPriority = 0);
   void buildDiagram() override;
   void buildNextLayer(unsigned int i) override;
   void propagate() override;
   void trimLayer(unsigned int layer) override;
   void debugGraph() override;
   void printRefs() {
      for(auto i=0u;i < numVariables;i++)
         std::cout << "R[" << i << "] = " << ref(i) << std::endl;
   }
};

#endif
