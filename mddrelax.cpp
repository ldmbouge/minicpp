/*
 * mini-cp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License  v3
 * as published by the Free Software Foundation.
 *
 * mini-cp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.
 * See the GNU Lesser General Public License  for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with mini-cp. If not, see http://www.gnu.org/licenses/lgpl-3.0.en.html
 *
 * Copyright (c)  2018. by Laurent Michel, Pierre Schaus, Pascal Van Hentenryck
 *
 * Contributions by Waldemar Cruz, Rebecca Gentzel, Willem Jan Van Hoeve
 */

#include "mddrelax.hpp"
#include <float.h>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <cmath>
#include "RuntimeMonitor.hpp"
#include "heap.hpp"

// [ldm] This should be only for instrumentation. There is no reason to have globals *ever*
int timeDoingDown = 0, timeDoingSplit = 0, timeDoingUp = 0, timeDoingUpProcess = 0, timeDoingUpFilter = 0;
int fullReboot = 0, partialReboot = 0, fullRebootFirstIteration = 0, partialRebootFirstIteration = 0;
bool firstIteration = true;
bool autoRebootDistance = false;

MDDRelax::MDDRelax(CPSolver::Ptr cp,int width,int maxDistance,int maxSplitIter,bool approxThenExact, int maxConstraintPriority)
   : MDD(cp),
     _width(width),
     _maxDistance(maxDistance),
     _maxSplitIter(maxSplitIter),
     _approxThenExact(approxThenExact),
     _maxConstraintPriority(maxConstraintPriority),
     _rnG(42),
     _sampler(0.0,1.0)
{
   _afp = new MDDIntSet[width];
   _src = new MDDNode*[width];
   _fwd = nullptr;
   _bwd = nullptr;
   _pool = new Pool;
   _deltaDown = nullptr;
   _deltaUp = nullptr;
   _deltaCombined = nullptr;
   _nf->setWidth(width);
   _mddspec.setConstraintPrioritySize(maxConstraintPriority + 1);
}

const MDDState& MDDRelax::pickReference(int layer,int layerSize)
{
   double v = _sampler(_rnG);
   double w = 1.0 / (double)layerSize;
   int c = (int)std::floor(v / w);
   int dirIdx = c;
   return layers[layer].get(dirIdx)->getDownState();
}

void MDDRelax::buildDiagram()
{
   std::cout << "MDDRelax::buildDiagram" << '\n';
   _mddspec.layout();
   _mddspec.compile();
   _deltaDown = new MDDDelta(_nf,_mddspec.sizeDown(),Down);
   _deltaUp = new MDDDelta(_nf,_mddspec.sizeUp(),Up);
   _deltaCombined = new MDDDelta(_nf,_mddspec.sizeCombined(),Bi);

   _fwd = new (mem) MDDFQueue(numVariables+1);
   _bwd = new (mem) MDDBQueue(numVariables+1);
   //std::cout << _mddspec << '\n';
   auto uDom = domRange(x);
   const int sz = uDom.second - uDom.first + 1;
   _domMin = uDom.first;
   _domMax = uDom.second;

   for(auto i=0u;i < _width;i++)
      _afp[i] = MDDIntSet((char*)mem->allocate(sizeof(int) * sz),sz);

   auto rootDownState = _mddspec.rootState(mem);
   MDDState rootUpState(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSizeUp()),Up);
   MDDState rootCombinedState(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSizeCombined()),Bi);
   rootDownState.computeHash();
   root = _nf->makeNode(rootDownState,rootUpState,rootCombinedState,x[0]->size(),0,0);
   _mddspec.updateNode(rootCombinedState,rootDownState,rootUpState);
   layers[0].push_back(root,mem);


   MDDState sinkDownState(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSizeDown()),Down);
   auto sinkUpState = _mddspec.sinkState(mem);
   MDDState sinkCombinedState(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSizeCombined()),Bi);
   sinkUpState.computeHash();
   sink = _nf->makeNode(sinkDownState,sinkUpState,sinkCombinedState,0,(int)numVariables,0);
   layers[numVariables].push_back(sink,mem);


   auto start = RuntimeMonitor::now();
   _refs.emplace_back(rootDownState);
   for(auto i = 0u; i < numVariables; i++) {
      buildNextLayer(i);
      _refs.emplace_back(pickReference(i+1,(int)layers[i+1].size()).clone(mem));
   }
   postUp();
   trimDomains();
   auto dur = RuntimeMonitor::elapsedSince(start);
   std::cout << "build/Relax:" << dur << '\n';
   propagate();
   hookupPropagators();
}


void MDDRelax::buildNextLayer(unsigned int i)
{
   int nbVals = x[i]->size();
   char* buf = (char*)alloca(sizeof(int)*nbVals);
   MDDIntSet xv(buf,nbVals);
   for(int v = x[i]->min(); v <= x[i]->max(); v++) 
      if(x[i]->contains(v)) xv.add(v);
   assert(layers[i].size() == 1);
   auto parent = layers[i][0];
   if (i < numVariables - 1) {
      MDDState downState(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSizeDown()),Down);
      MDDState upState(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSizeUp()),Up);
      MDDState combinedState(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSizeCombined()),Bi);
      _sf->createStateDown(downState,parent->getDownState(),parent->getCombinedState(),i,x[i],xv,false);
      MDDNode* child = _nf->makeNode(downState,upState,combinedState,x[i]->size(),i+1,(int)layers[i+1].size());
      _mddspec.updateNode(combinedState,downState,upState);
      layers[i+1].push_back(child,mem);
      for(auto v : xv) {
         parent->addArc(mem,child,v);
         addSupport(i,v);
      }
   } else {
      MDDState sinkDownState(sink->getDownState());
      MDDState sinkCombinedState(sink->getCombinedState());
      _sf->createStateDown(sinkDownState, parent->getDownState(), parent->getCombinedState(), i, x[i],xv,false);
      _mddspec.updateNode(sinkCombinedState,sink->getDownState(),sink->getUpState());
      assert(sink->getNumParents() == 0);
      for(auto v : xv) {
         parent->addArc(mem,sink,v);
         addSupport(i,v);
      }
   }
   for(auto v : xv) 
      if (getSupport(i,v)==0)
         x[i]->remove(v);
}

void MDDRelax::postUp()
{
   if (_mddspec.usesUp()) {
      MDDState sinkCombinedState(sink->getCombinedState());
      _mddspec.updateNode(sinkCombinedState,sink->getDownState(),sink->getUpState());
      for(int i = (int)numVariables - 1;i >= 0;i--) 
         for(auto& n : layers[i]) {
            bool upDirty = processNodeUp(n,i);
            if (upDirty) {
               bool combinedDirty = updateCombinedIncrUp(n);
               if (!_mddspec.consistent(n->getDownState(), n->getUpState(), n->getCombinedState())) {
                  if (i == 0) failNow();
                  delState(n,i);
               }
               filterParents(n,i);
               if (combinedDirty) filterKids(n,i);
            }
         }
   }
}

// -----------------------------------------------------------------------------------
// Propagation code
// -----------------------------------------------------------------------------------

void MDDRelax::trimLayer(unsigned int layer)
{
   if (_firstTime.fresh()) {
      _firstTime = false;
      queue.clear();
      _fwd->clear();
      _bwd->clear();
   }
   auto var = x[layer];
   for(auto i = layers[layer].cbegin(); i != layers[layer].cend();i++) {
      auto& children = (*i)->getChildren();
      for(int i = (int)children.size() - 1; i >= 0 ; i--){
         auto arc = children.get(i);
         if(!var->contains(arc->getValue())) {
            //MDDNode* parent = arc->getParent();
            //MDDNode* child = arc->getChild();
            removeArc(layer,layer+1,arc.get());
            arc->remove(this);
            //if (child->getNumParents()==0) delState(child,layer+1);
            //if (parent->getNumChildren()==0) delState(parent,layer);
         }
      }   
   }
}

void MDDRelax::removeArc(int outL,int inL,MDDEdge* arc) // notified when arc is deleted.
{
   assert(outL + 1 == inL);
   if (_mddspec.usesUp()) {
      auto p = arc->getParent();
      if (p->isActive())
         _bwd->enQueue(p);
   }
   auto c = arc->getChild();
   if (c->isActive())
      _fwd->enQueue(c);
}

void MDDRelax::fullStateDown(MDDState& ms,MDDState& cs,MDDNode* n,int l)
{
   bool first = true;
   for(auto i = 0u;i < _width;i++) {
      if (_src[i]==nullptr) continue;
      auto p = _src[i];                           // this is the parent
      assert(_afp[i].size() > 0);                 // afp[i] is the set of arcs from that parent
      _sf->createStateDown(cs,p->getDownState(),p->getCombinedState(),l-1,x[l-1],_afp[i],true); // compute a full scale transitions (all props).
      if (first) {
         ms.copyState(cs); // install the result into an accumulator
         first = false;
      } else {
         if (ms != cs) {
            _mddspec.relaxationDown(ms,cs);   // compute a full scale relaxation of cs with the accumulator (ms).
            ms.relax();               // indidcate this is a down relaxation.
         }
      }
   }
}

void MDDRelax::incrStateDown(const MDDPropSet& out,MDDState& ms,MDDState& cs,MDDNode* n,int l)
{
   bool first = true;
   for(auto i = 0u;i < _width;i++) {
      if (_src[i]==nullptr) continue;
      auto p = _src[i];                           // this is the parent
      assert(_afp[i].size() > 0);                 // afp[i] is the set of arcs from that parent
      cs.copyState(n->getDownState());       // grab the down information from other properties
      _mddspec.incrStateDown(out,cs,p->getDownState(),p->getCombinedState(),l-1,x[l-1],_afp[i],true); // compute a full scale transitions (all props).
      if (first) {
         ms.copyState(cs); // install the result into an accumulator
         first = false;
      } else {
         if (ms != cs) {
            _mddspec.relaxationDownIncr(out,ms,cs);   // compute an incremental  relaxation of cs with the accumulator (ms). 
            ms.relax();               // indidcate this is a down relaxation.
         }
      }
   }
}
   
void MDDRelax::aggregateValueSet(MDDNode* n)
{
   assert(n->getNumParents() > 0);
   for(auto i=0u;i < _width;i++) {
      _afp[i].clear();
      _src[i] = nullptr;
   }
   for(auto& a : n->getParents()) {
      auto p = a->getParent();
      auto v = a->getValue();
      _afp[p->getPosition()].add(v);
      _src[p->getPosition()] = p;
   }
}

bool MDDRelax::refreshNodeIncr(MDDNode* n,int l)
{
   if (l == 0) {
      assert(n->getNumParents() == 0);
      bool isOk = _mddspec.consistent(n->getDownState(), n->getUpState(), n->getCombinedState());
      if (!isOk) failNow();
      return false;
   }
   aggregateValueSet(n);
   const bool parentsChanged = n->parentsChanged();

   MDDState cs(&_mddspec,(char*)alloca(_mddspec.layoutSizeDown()),Down);
   MDDState ms(&_mddspec,(char*)alloca(_mddspec.layoutSizeDown()),Down);

   // Causes of "parentsChanged":
   // (1) arc removal, (2) arc addition to an existing parent, (3) arc addition to a new parent node.
   // Causes of changes != empty:
   // at least one Parent state is different.
   MDDPropSet out;

   if (parentsChanged) {
      fullStateDown(ms,cs,n,l);
      n->resetParentsChanged();
   } else  {
      MDDPropSet changesDown((long long*)alloca(sizeof(long long)*propNbWords(_mddspec.sizeDown())),_mddspec.sizeDown());
      MDDPropSet changesCombined((long long*)alloca(sizeof(long long)*propNbWords(_mddspec.sizeCombined())),_mddspec.sizeCombined());
      for(auto& a : n->getParents()) 
         changesDown.unionWith(_deltaDown->getDelta(a->getParent()));
      changesCombined.unionWith(_deltaCombined->getDelta(n));
      out = MDDPropSet((long long*)alloca(sizeof(long long)*changesDown.nbWords()),changesDown.nbProps());
      _mddspec.outputSetDown(out,changesDown,changesCombined);
      incrStateDown(out,ms,cs,n,l);
   }
   bool changed = n->getDownState() != ms;
   if (changed) {
      if (parentsChanged)
         _deltaDown->setDelta(n,ms);
      else
         _deltaDown->setDelta(n,ms,out);
      n->setDownState(ms,mem);
   }
   return changed;
}

bool MDDRelax::filterKids(MDDNode* n,int l)
{
   bool changed = false;
   assert(layers[numVariables].size() == 1);
   if (n->isActive()) {
      for(auto i = n->getChildren().rbegin(); i != n->getChildren().rend();i++) {
         auto arc = *i;
         MDDNode* child = arc->getChild();
         int v = arc->getValue();
         if (!_mddspec.exist(n->pack(),child->pack(),x[l],v)) {
            n->unhook(arc);
            changed = true;
            delSupport(l,v);
            removeArc(l,l+1,arc.get());
            if (child->getNumParents()==0) delState(child,l+1);
         } else {
            _fwd->enQueue(child);
         }
      }
      if (n->getNumChildren()==0 && l != (int)numVariables) {
         delState(n,l);
         assert(layers[numVariables].size() == 1);
         changed = true;
      } else if (changed && _mddspec.usesUp()) _bwd->enQueue(n);
   }
   return changed;
}

bool MDDRelax::filterParents(MDDNode* n,int l)
{
   bool changed = false;
   if (n->isActive()) {
      for(auto i = n->getParents().rbegin(); i != n->getParents().rend();i++) {
         auto arc = *i;
         MDDNode* parent = arc->getParent();
         int v = arc->getValue();
         if (!_mddspec.exist(parent->pack(),n->pack(),x[l-1],v)) {
            parent->unhook(arc);
            changed = true;
            delSupport(l-1,v);
            removeArc(l-1,l,arc.get());
            if (parent->getNumChildren()==0) delState(parent,l-1);
         } else if (_mddspec.usesUp()) {
            _bwd->enQueue(parent);
         }
      }
      if (n->getNumParents()==0 && l != 0) {
         delState(n,l);
         changed = true;
      } else if (changed) _fwd->enQueue(n);
   }
   return changed;
}

template <typename Container,typename T,typename Fun> T sum(Container& c,T acc,const Fun& fun) {
   for(auto& term : c) acc += fun(term);
   return acc;
}

class MDDNodeSim {
   struct PQValue {
      double   _key;
      MDDNode* _val;
   };
   struct PQOrder {
      bool operator()(const PQValue& a,const PQValue& b) { return a._key > b._key;}
   };
   Pool::Ptr _mem;
   const TVec<MDDNode*>& _layer;
   const MDDSpec& _mddspec;
   Heap<PQValue,PQOrder>  _pq;
public:
   MDDNodeSim(Pool::Ptr mem,const TVec<MDDNode*>& layer,const MDDState& ref,const MDDSpec& mddspec, int constraintPriority)
      : _mem(mem),
        _layer(layer),
        _mddspec(mddspec),
        _pq(mem,(int)layer.size())
   {
      for(auto& n : _layer)
         if (n->getDownState().isRelaxed() && n->getNumParents() > 1) {
            double key = (_mddspec.hasNodeSplitRule()) ? _mddspec.nodeSplitPriority(*n, constraintPriority) : (double) n->getPosition();
            _pq.insert(PQValue { key, n});
         }
      _pq.buildHeap();
   }
   MDDNode* extractNode() {
      while (!_pq.empty()) {
         PQValue x = _pq.extractMax();
         return x._val;
      }
      return nullptr;
   }
   bool isEmpty() {
      return _pq.empty();
   }
};

class MDDPotential {
   Pool::Ptr        _mem;
   MDDNode*           _n;
   MDDNode*         _par;
   int            _mxPar;
   int            _nbPar;
   MDDEdge::Ptr*    _arc;
   MDDState*      _child;
   double           _key;
   int       _val;
   int      _nbk;
   int _constraintPriority;
   bool*   _keepKids;
public:
   MDDPotential(Pool::Ptr pool,MDDNode* n,const int mxPar,MDDNode* par,MDDEdge::Ptr arc,
                MDDState* child,int v,int nbKids,bool* kk, int constraintPriority)
      : _mem(pool),_n(n),_par(par),_mxPar(mxPar),_nbPar(0),_constraintPriority(constraintPriority) {
      _child = child;
      _arc = new (pool) MDDEdge::Ptr[_mxPar];
      _arc[_nbPar++] = arc;
      _val = v;
      _nbk = nbKids;
      _keepKids = new (_mem) bool[_nbk];
      for(int i=0;i < _nbk;i++) _keepKids[i] = kk[i];
   }
   void computeKey(const MDDSpec& mddspec) {
      _key = mddspec.hasCandidateSplitRule() ? mddspec.candidateSplitPriority(*_child, _arc, _nbPar, _constraintPriority) : (double) _par->getPosition();
   }
   double getKey() const noexcept { return _key;}
   bool hasState(const MDDState& s) const { return *_child == s;}
   void link(MDDEdge::Ptr arc) {
      assert(_nbPar < _mxPar);
      _arc[_nbPar++] = arc;
   }
   template <class Callback> void instantiate(const Callback& cb,Trailer::Ptr trail,Storage::Ptr mem)
   {
      MDDNode* nc = cb(_n,_par,*_child,_val,_nbk,_keepKids);
      for(int i=0;i < _nbPar;i++)
         _arc[i]->moveTo(nc,trail,mem);      
   }
   MDDState* getState() const noexcept { return _child; }
   bool* keepKids() { return _keepKids; }
};


class MDDSplitter {
   struct COrder {
      bool operator()(MDDPotential* a,MDDPotential* b) const noexcept {
         return a->getKey() > b->getKey();
      }
   };
   Pool::Ptr                        _pool;
   int                             _width;
   Heap<MDDPotential*,COrder> _candidates;
   const MDDSpec&                _mddspec;
public:
   MDDSplitter(Pool::Ptr pool,const MDDSpec& spec,int w)
      : _pool(pool),_width(w),
        _candidates(pool, 4 * _width),
        _mddspec(spec) {}
   void clear() { _candidates.clear();}
   auto size() { return _candidates.size();}
   template <class... Args> void addPotential(Args&&... args) {
      _candidates.insert(new (_pool) MDDPotential(std::forward<Args>(args)...));
   }
   int hasState(MDDState& s) {
      for(auto i = 0u;i  < _candidates.size();i++)
         if (_candidates[i]->hasState(s))
            return i;
      return -1;
   }
   int hasMatchingState(MDDState& s, int constraintPriority) {
      for(auto i = 0u;i  < _candidates.size();i++)
         if (_mddspec.equivalentForConstraintPriority(*_candidates[i]->getState(), s, constraintPriority)) {
            return i;
         }
      return -1;
   }
   void linkChild(int reuse,MDDEdge::Ptr arc) {
      _candidates[reuse]->link(arc);
   }
   MDDPotential* getFirstCandidate() {
      return _candidates[0];
   }
   template <class CB>
   void process(TVec<MDDNode*>& layer,unsigned long width,Trailer::Ptr trail,Storage::Ptr mem,const CB& cb) {
      if (_candidates.size() + layer.size() <= width) {
         for(auto i = 0u;i < _candidates.size();++i)
            _candidates[i]->computeKey(_mddspec);
         _candidates.buildHeap();
         for(auto i = 0u;i < _candidates.size() && layer.size() < width;i++) 
            _candidates[i]->instantiate(cb,trail,mem);
         _candidates.clear();
      } else {
         for(auto i = 0u;i < _candidates.size();++i)
            _candidates[i]->computeKey(_mddspec);
         _candidates.buildHeap();
         while (!_candidates.empty()) {
            MDDPotential* p = _candidates.extractMax();
            p->instantiate(cb,trail,mem);
            if (layer.size() >= width)
               break;
         }
      }
   }
};

MDDNode* findMatchInLayer(const TVec<MDDNode*>& layer,const MDDState& s)
{
   for(MDDNode* p : layer)
      if (p->getDownState() == s)
         return p;   
   return nullptr;
}


int splitCS = 0,pruneCS = 0,potEXEC = 0;

int MDDRelax::splitNode(MDDNode* n,int l,MDDSplitter& splitter)
{
   bool foundNonviableCandidate = false;
   int lowest = l;
   MDDState* ms = nullptr;
   const int nbParents = (int) n->getNumParents();
   auto last = n->getParents().rend();
   for(auto pit = n->getParents().rbegin(); pit != last;pit++) {
      auto a = *pit;                // a is the arc p --(v)--> n
      auto p = a->getParent();      // p is the parent
      auto v = a->getValue();       // value on arc from parent
      _sf->splitState(ms,n,p->getDownState(),p->getCombinedState(),l-1,x[l-1],v);
      splitCS++;         
      MDDNode* bj = findMatchInLayer(layers[l],*ms);
      if (bj && bj != n) {
         a->moveTo(bj,trail,mem);            
         // If we matched to n nothing to do. We already point to n.
      } else { // There is an approximate match
         // So, if there is room create a new node
         int reuse = splitter.hasState(*ms);
         if (reuse != -1) {
            splitter.linkChild(reuse,a);
         } else {
            int nbk = (int)n->getNumChildren();
            bool keepArc[nbk];
            unsigned idx = 0,cnt = 0;
            for(auto ca : n->getChildren()) {
               MDDPack pack(*ms,p->getUpState(),p->getCombinedState());
               cnt += keepArc[idx++] = _mddspec.exist(pack,ca->getChild()->pack(),x[l],ca->getValue());
            }
            if (cnt == 0) {
               foundNonviableCandidate = true;
               pruneCS++;               
               p->unhook(a);
               if (p->getNumChildren()==0) lowest = std::min(lowest,delState(p,l-1));
               delSupport(l-1,v);
               removeArc(l-1,l,a.get());
               if (_mddspec.usesUp() && p->isActive()) _bwd->enQueue(p);
            } else {
               splitter.addPotential(_pool,n,nbParents,p,a,ms,v,nbk,(bool*)keepArc,0);
            }
         }
      } //out-comment
   } // end of loop over parents.
   if ((autoRebootDistance ? _mddspec.rebootFor(l-1) : _maxDistance) && lowest < l) return lowest;

   if (splitter.size() == 1 && !foundNonviableCandidate) {
      splitter.clear();
   }

   _fwd->enQueue(n);
   //_bwd->enQueue(n);
   return lowest;
}
int MDDRelax::splitNodeForConstraintPriority(MDDNode* n,int l,MDDSplitter& splitter, int constraintPriority)
{
   std::vector<MDDState*> candidateStates;
   std::vector<std::vector<MDDEdge::Ptr>> arcsPerCandidate;
   int lowest = l;
   MDDState* ms = nullptr;
   MDDState* existing = nullptr;
   const int nbParents = (int) n->getNumParents();
   auto last = n->getParents().rend();
   for(auto pit = n->getParents().rbegin(); pit != last;pit++) {
      auto a = *pit;                // a is the arc p --(v)--> n
      auto p = a->getParent();      // p is the parent
      auto v = a->getValue();       // value on arc from parent
      _sf->splitState(ms,n,p->getDownState(),p->getCombinedState(),l-1,x[l-1],v);
      splitCS++;         
      MDDNode* bj = findMatchInLayer(layers[l],*ms);
      if (bj && bj != n) {
         a->moveTo(bj,trail,mem);            
         // If we matched to n nothing to do. We already point to n.
      } else { // There is an approximate match
         // So, if there is room create a new node
         int reuse = -1;
         for(auto i = 0u;i  < candidateStates.size();i++)
            if (_mddspec.equivalentForConstraintPriority(*candidateStates[i], *ms, constraintPriority)) {
               reuse = i;
               break;
            }
         if (reuse != -1) {
            existing = candidateStates[reuse];
            if (ms != existing) {
               _mddspec.relaxationDown(*existing,*ms);
               existing->relax();
            }
            arcsPerCandidate[reuse].emplace_back(a);
         } else {
            candidateStates.emplace_back(ms);
            arcsPerCandidate.emplace_back(std::vector<MDDEdge::Ptr>{a});
         }
      } //out-comment
   } // end of loop over parents.

   bool foundNonviableClass = false;
   int reuseIndex = 0;
   for (unsigned int candidateIndex = 0u; candidateIndex < candidateStates.size(); candidateIndex++) {
      MDDState* newState = candidateStates[candidateIndex];
      int nbk = (int)n->getNumChildren();
      bool keepArc[nbk];
      unsigned idx = 0,cnt = 0;
      for(auto ca : n->getChildren()) {
         MDDPack pack(*newState,ca->getParent()->getUpState(),ca->getParent()->getCombinedState());
         cnt += keepArc[idx++] = _mddspec.exist(pack,ca->getChild()->pack(),x[l],ca->getValue());
      }
      if (cnt == 0) {
         for (auto arc : arcsPerCandidate[candidateIndex]) {
            auto p = arc->getParent();      // p is the parent
            auto v = arc->getValue();       // value on arc from parent
            pruneCS++;               
            p->unhook(arc);
            if (p->getNumChildren()==0) lowest = std::min(lowest,delState(p,l-1));
            delSupport(l-1,v);
            removeArc(l-1,l,arc.get());
            if (_mddspec.usesUp() && p->isActive()) _bwd->enQueue(p);
         }
         foundNonviableClass = true;
      } else {
         MDDEdge::Ptr arc = arcsPerCandidate[candidateIndex][0];
         auto p = arc->getParent();
         auto v = arc->getValue();
         splitter.addPotential(_pool,n,nbParents,p,arc,newState,v,nbk,(bool*)keepArc,constraintPriority);
         for (unsigned int arcIndex = 1u; arcIndex < arcsPerCandidate[candidateIndex].size(); arcIndex++) {
            MDDEdge::Ptr arc = arcsPerCandidate[candidateIndex][arcIndex];
            splitter.linkChild(reuseIndex, arc);
         }
         reuseIndex++;
      }
   }
   if ((autoRebootDistance ? _mddspec.rebootFor(l-1) : _maxDistance) && lowest < l) return lowest;
   if (splitter.size() == 1 && !foundNonviableClass) {
      splitter.clear();
   }
   _fwd->enQueue(n);
   //_bwd->enQueue(n);
   return lowest;
}
int MDDRelax::splitNodeApprox(MDDNode* n,int l,MDDSplitter& splitter, int constraintPriority)
{
   std::map<std::vector<int>,MDDState*> equivalenceClasses;
   std::multimap<std::vector<int>,MDDEdge::Ptr> arcsPerClass;
   int lowest = l;
   MDDState* ms = nullptr;
   const int nbParents = (int) n->getNumParents();
   auto last = n->getParents().rend();
   for(auto pit = n->getParents().rbegin(); pit != last;pit++) {
      auto a = *pit;                // a is the arc p --(v)--> n
      auto p = a->getParent();      // p is the parent
      auto v = a->getValue();       // value on arc from parent
      _sf->splitState(ms,n,p->getDownState(),p->getCombinedState(),l-1,x[l-1],v);
      splitCS++;         
      MDDNode* bj = findMatchInLayer(layers[l],*ms);
      if (bj && bj != n) {
         a->moveTo(bj,trail,mem);            
      } else { // There is an approximate match
         // So, if there is room create a new node
         std::vector<int> equivalenceValue = _mddspec.equivalenceValue(*ms,n->getUpState(),constraintPriority);
         if (equivalenceClasses.count(equivalenceValue)) {
            auto existing = equivalenceClasses.at(equivalenceValue);
            if (ms != existing) {
               _mddspec.relaxationDown(*existing,*ms);
               existing->relax();
            }
            arcsPerClass.insert(std::make_pair(equivalenceValue, a));
         } else {
            equivalenceClasses[equivalenceValue] = ms;
            arcsPerClass.insert({equivalenceValue, a});
         }
      } //out-comment
   } // end of loop over parents.

   bool foundNonviableClass = false;

   for (auto it = equivalenceClasses.begin(); it != equivalenceClasses.end(); ++it) {
      std::vector<int> equivalenceValue = it->first;
      MDDState* newState = it->second;
      int nbk = (int)n->getNumChildren();
      bool keepArc[nbk];
      unsigned idx = 0,cnt = 0;
      for(auto ca : n->getChildren()) {
         MDDPack pack(*newState,ca->getParent()->getUpState(),ca->getParent()->getCombinedState());
         cnt += keepArc[idx++] = _mddspec.exist(pack,ca->getChild()->pack(),x[l],ca->getValue());
      }
      if (cnt == 0) {
         for (auto arcIt = arcsPerClass.begin(); arcIt != arcsPerClass.end(); ++arcIt) {
            if (arcIt->first == equivalenceValue) {
               MDDEdge::Ptr arc = arcIt->second;
               auto p = arc->getParent();      // p is the parent
               auto v = arc->getValue();       // value on arc from parent
               pruneCS++;               
               p->unhook(arc);
               if (p->getNumChildren()==0) lowest = std::min(lowest,delState(p,l-1));
               delSupport(l-1,v);
               removeArc(l-1,l,arc.get());
               if (_mddspec.usesUp() && p->isActive()) _bwd->enQueue(p);
            }
         }
         foundNonviableClass = true;
      } else {
         int splitterStateIndex = -1;
         for (auto arcIt = arcsPerClass.begin(); arcIt != arcsPerClass.end(); ++arcIt) {
            if (arcIt->first == equivalenceValue) {
               MDDEdge::Ptr arc = arcIt->second;
               if (splitterStateIndex >= 0) {
                  splitter.linkChild(splitterStateIndex, arc);
               } else {
                  splitterStateIndex = splitter.size();
                  auto p = arc->getParent();
                  auto v = arc->getValue();
                  splitter.addPotential(_pool,n,nbParents,p,arc,newState,v,nbk,(bool*)keepArc,constraintPriority);
               }
            }
         }
      }
   }
   if ((autoRebootDistance ? _mddspec.rebootFor(l-1) : _maxDistance) && lowest < l) return lowest;
   if (splitter.size() == 1 && !foundNonviableClass) {
      splitter.clear();
   }
   _fwd->enQueue(n);
   //_bwd->enQueue(n);
   return lowest;
}


void MDDRelax::splitLayers(bool approximate, int constraintPriority) // this can use node from recycled or add node to recycle
{
   using namespace std;
   int nbScans = 0,nbSplits = 0;
   int l = 1;
   const int ub = 300 * (int)numVariables;
   _pool->clear();
   MDDSplitter splitter(_pool,_mddspec,_width);
   while (l < (int)numVariables && nbSplits < ub) {
      auto& layer = layers[l];
      int lowest = l;
      trimVariable(l-1);
      ++nbScans;
      if (!x[l-1]->isBound() && layers[l].size() < _width) {
         splitter.clear();
         MDDNodeSim nSim(_pool,layers[l],_refs[l],_mddspec,constraintPriority);
         MDDNode* n = nullptr;
         while (layer.size() < _width && lowest==l) {
            while(splitter.size() == 0)  {
               n = nSim.extractNode();
               if (n) {
                  if (approximate)
                     lowest = splitNodeApprox(n,l,splitter,constraintPriority);
                  else if (_maxConstraintPriority)
                     lowest = splitNodeForConstraintPriority(n, l, splitter, constraintPriority);
                  else
                     lowest = splitNode(n,l,splitter);
               } else break;
            }
            if (n==nullptr) break;
            if (n->getNumParents() == 0) {
               delState(n,l);
            }
            if ((autoRebootDistance ? _mddspec.rebootFor(l-1) : _maxDistance) && lowest < l) break;
            splitter.process(layer,_width,trail,mem,
                             [this,l,&layer](MDDNode* n,MDDNode* p,const MDDState& ms,int val,int nbk,bool* kk) {
                                potEXEC++;
                                MDDState up(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSizeUp()),Up);
                                up.copyState(n->getUpState());
                                MDDState combined(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSizeCombined()),Bi);
                                MDDNode* nc = _nf->makeNode(ms,up,combined,x[l-1]->size(),l,(int)layer.size());
                                fullStateCombined(combined,nc);
                                layer.push_back(nc,mem);
                                unsigned int idx = 0;
                                for(auto ca : n->getChildren()) {
                                   if (kk[idx++]) {
                                      nc->addArc(mem,ca->getChild(),ca->getValue());
                                      addSupport(l,ca->getValue());
                                      _fwd->enQueue(ca->getChild());
                                   }
                                }
                                if (_mddspec.usesUp()) _bwd->enQueue(nc);
                                return nc;
                             });
            if (n->getNumParents() == 0) {
               delState(n,l);
            }
         }
         ++nbSplits;
      } // end-if (there is room and variable is not bound)
      auto jump = std::min(l - lowest, autoRebootDistance ? _mddspec.rebootFor(l-1) : _maxDistance);
      if (lowest != l) {
        if (jump == l - lowest) fullReboot++;
        else partialReboot++;
      }
      if (jump) l -= jump;
      else l++;
   }
}

struct MDDStateEqual {
   bool operator()(const MDDState* s1,const MDDState* s2) const { return *s1 == *s2;}
};

void MDDRelax::removeNode(MDDNode* node)
{
   delState(node, node->getLayer());
}

int MDDRelax::delState(MDDNode* node,int l)
{
   if (layers[l].size() == 1) failNow();
   int lowest = l;
   assert(node->isActive());
   node->deactivate();
   assert(l == node->getLayer());
   const int at = node->getPosition();
   assert(node == layers[l].get(at));
   layers[l].remove(at);
   node->setPosition((int)layers[l].size(),mem);
   layers[l].get(at)->setPosition(at,mem);
   if (node->getNumParents() > 0) {
      for(auto& arc : node->getParents()) {
         if (arc->getParent()->unhookOutgoing(arc))
            lowest = std::min(lowest,delState(arc->getParent(),l-1));
         delSupport(l-1,arc->getValue());
         if (arc->getParent()->isActive())
            _bwd->enQueue(arc->getParent());
      }
      node->clearParents();
   }
   if (node->getNumChildren() > 0) {
      for(auto& arc : node->getChildren()) {
         if (arc->getChild()->unhookIncoming(arc))
            lowest = std::min(lowest,delState(arc->getChild(),l+1));
         delSupport(l,arc->getValue());
         if (arc->getChild()->isActive())
            _fwd->enQueue(arc->getChild());
      }
      node->clearChildren();
   }   
   switch(node->curQueue()) {
      case Down: _fwd->retract(node);break;
      case Up: _bwd->retract(node);break;
      case Bi:
         _fwd->retract(node);
         _bwd->retract(node);
         break;
      case None: break;
   }
   _nf->returnNode(node);
   return lowest;
}

bool MDDRelax::trimVariable(int i)
{
   bool trim = false;
   for(int v = x[i]->min(); v <= x[i]->max();v++) {
      if (x[i]->contains(v) && getSupport(i,v)==0) {
         x[i]->remove(v);
         trim |= true;
      }
   }
   return trim;
}

void MDDRelax::fullStateUp(MDDState& ms,MDDState& cs,MDDNode* n,int l)
{
   bool first = true;
   auto wub = std::min(_width,(unsigned)layers[l+1].size());
   for(auto k=0u;k < wub;k++) {
      if (_afp[k].size() > 0) {
         auto c = layers[l+1][k];
         _sf->createStateUp(cs,c->getUpState(),c->getCombinedState(),l,x[l],_afp[k]); // compute a full scale transitions (all props).
         if (first) {
            ms.copyState(cs);
            first = false;
         } else {
            if (ms != cs) {
               _mddspec.relaxationUp(ms,cs);
               ms.relax();
            }
         }
      }
   }
}

void MDDRelax::incrStateUp(const MDDPropSet& out,MDDState& ms,MDDState& cs,MDDNode* n,int l)
{
   bool first = true;
   auto wub = std::min(_width,(unsigned)layers[l+1].size());
   for(auto k=0u;k < wub;k++) {
      if (_afp[k].size() > 0) {
         cs.copyState(n->getUpState());
         auto c = layers[l+1][k];
         _mddspec.incrStateUp(out,cs,c->getUpState(),c->getCombinedState(),l,x[l],_afp[k]);
         if (first) {
            ms.copyState(cs);
            first = false;
         } else {
            if (ms != cs) {
               _mddspec.relaxationUpIncr(out,ms,cs);
               ms.relax();
            }
         }
      }
   }
}

bool MDDRelax::processNodeUp(MDDNode* n,int i) // i is the layer number
{
   if (i == (int)numVariables) {
      assert(n->getNumChildren() == 0);
      bool isOk = _mddspec.consistent(n->getDownState(),n->getUpState(),n->getCombinedState());
      if (!isOk) failNow();
      return false;
   }
   for(auto k=0u;k<_width;k++)
      _afp[k].clear();
   for(auto& a : n->getChildren()) {
      auto kid = a->getChild();
      int v = a->getValue();
      _afp[kid->getPosition()].add(v);
   }
   const bool childrenChanged = n->childrenChanged();

   MDDState cs(&_mddspec,(char*)alloca(_mddspec.layoutSizeUp()),Up);
   MDDState ms(&_mddspec,(char*)alloca(_mddspec.layoutSizeUp()),Up);

   MDDPropSet out;

   if (childrenChanged) {
      fullStateUp(ms,cs,n,i);
      n->resetChildrenChanged();
   } else {
      MDDPropSet changesUp((long long*)alloca(sizeof(long long)*propNbWords(_mddspec.sizeUp())),_mddspec.sizeUp());
      MDDPropSet changesCombined((long long*)alloca(sizeof(long long)*propNbWords(_mddspec.sizeCombined())),_mddspec.sizeCombined());
      for(auto& a : n->getChildren())
         changesUp.unionWith(_deltaUp->getDelta(a->getChild()));
      changesCombined.unionWith(_deltaCombined->getDelta(n));
      out = MDDPropSet((long long*)alloca(sizeof(long long)*changesUp.nbWords()),changesUp.nbProps());
      _mddspec.outputSetUp(out,changesUp,changesCombined);
      incrStateUp(out,ms,cs,n,i);
   }

   bool changed = n->getUpState() != ms;
   if (changed) {
      if (childrenChanged)
         _deltaUp->setDelta(n,ms);
      else
         _deltaUp->setDelta(n,ms,out);
      n->setUpState(ms,mem);
   }
   return changed;
}

void MDDRelax::fullStateCombined(MDDState& state,MDDNode* n)
{
   if (!_mddspec.usesCombined()) return;
   state.copyState(n->getCombinedState());
   _mddspec.updateNode(state,n->getDownState(),n->getUpState());
}

void MDDRelax::incrStateCombined(const MDDPropSet& out,MDDState& state,MDDNode* n)
{
   if (!_mddspec.usesCombined()) return;
   state.copyState(n->getCombinedState());
   _mddspec.updateNode(state,n->getDownState(),n->getUpState());
}

bool MDDRelax::updateCombinedIncrDown(MDDNode* n)
{
   if (!_mddspec.usesCombined()) return false;
   MDDState state(&_mddspec,(char*)alloca(_mddspec.layoutSizeCombined()),Bi);

   MDDPropSet out;

   MDDPropSet changes((long long*)alloca(sizeof(long long)*propNbWords(_mddspec.sizeDown())),_mddspec.sizeDown());
   changes.unionWith(_deltaDown->getDelta(n));
   out = MDDPropSet((long long*)alloca(sizeof(long long)*propNbWords(_mddspec.sizeCombined())),_mddspec.sizeCombined());
   _mddspec.outputSetCombinedFromDown(out,changes);

   incrStateCombined(out,state,n);

   bool changed = n->getCombinedState() != state;
   if (changed) {
      _deltaCombined->setDelta(n,state,out);
      n->setCombinedState(state,mem);
   }
   return changed;
}

bool MDDRelax::updateCombinedIncrUp(MDDNode* n)
{
   if (!_mddspec.usesCombined()) return false;
   MDDState state(&_mddspec,(char*)alloca(_mddspec.layoutSizeCombined()),Bi);

   MDDPropSet out;

   MDDPropSet changes((long long*)alloca(sizeof(long long)*propNbWords(_mddspec.sizeUp())),_mddspec.sizeUp());
   changes.unionWith(_deltaUp->getDelta(n));
   out = MDDPropSet((long long*)alloca(sizeof(long long)*propNbWords(_mddspec.sizeCombined())),_mddspec.sizeCombined());
   _mddspec.outputSetCombinedFromUp(out,changes);

   incrStateCombined(out,state,n);

   bool changed = n->getCombinedState() != state;
   if (changed) {
      _deltaCombined->setDelta(n,state,out);
      n->setCombinedState(state,mem);
   }
   return changed;
}

int __nbn = 0,__nbf = 0;

void MDDRelax::computeDown(int iter)
{
   //auto start = RuntimeMonitor::now();
   if (iter <= _maxSplitIter) {
      //std::cout << "Start refinement iteration" << iter << "\n";
      for (int i = 0; i <= _maxConstraintPriority; i++) {
         if (_mddspec.approxEquivalence()) {
            splitLayers(true, i);
            if (_approxThenExact) {
               splitLayers(false, i);
            }
         } else {
            splitLayers(false, i);
         }
      }
   }
   if (firstIteration) {
     fullRebootFirstIteration = fullReboot;
     partialRebootFirstIteration = partialReboot;
     std::cout << "Full Reboot First Iteration: " << fullRebootFirstIteration << "\n";
     std::cout << "Partial Reboot First Iteration: " << partialRebootFirstIteration << "\n";
     firstIteration = false;
   }
   //timeDoingSplit += RuntimeMonitor::elapsedSinceMicro(start);
   //_sf->disable();
   //start = RuntimeMonitor::now();
   while(!_fwd->empty()) {
      MDDNode* node = _fwd->deQueue();
      //if (node==nullptr) break;  // the queue could be "filled" with inactive nodes
      int l = node->getLayer();
      if (l > 0 && node->getNumParents() == 0) {
         if (l == (int)numVariables) failNow();
         delState(node,l);
         continue;
      }
      if (l < (int)numVariables && node->getNumChildren() == 0) {
         if (l == 0) failNow();
         delState(node,l);
         continue;
      }
      bool downDirty = refreshNodeIncr(node,l);
      if (downDirty) {
         bool combinedDirty = updateCombinedIncrDown(node);
         if (!_mddspec.consistent(node->getDownState(), node->getUpState(), node->getCombinedState())) {
            if (l == (int)numVariables) failNow();
            delState(node,l);
         }
         filterKids(node,l);   // must filter unconditionally after a refresh since children may have changed.
         if (combinedDirty) filterParents(node, l);
      }
   }
   //timeDoingDown += RuntimeMonitor::elapsedSinceMicro(start);
}

void MDDRelax::computeUp()
{
   if (_mddspec.usesUp())  {
      while (!_bwd->empty()) {
         MDDNode* n = _bwd->deQueue();
         bool upDirty = processNodeUp(n,n->getLayer());
         if (upDirty) {
            bool combinedDirty = updateCombinedIncrUp(n);
            if (!_mddspec.consistent(n->getDownState(), n->getUpState(), n->getCombinedState())) {
               if (n->getLayer() == 0) failNow();
               delState(n,n->getLayer());
            }
            filterParents(n,n->getLayer());
            if (combinedDirty) filterKids(n,n->getLayer());
         }
      }
   } else _bwd->clear();
}

int iterMDD = 0;

void MDDRelax::propagate()
{
   TRYFAIL
      setScheduled(true);
      bool change = false;
      MDD::propagate();
      int iter = 0;
      do {
         _fwd->init(); 
         _bwd->init();
         ++iterMDD;++iter;
         _deltaDown->clear();
         _deltaUp->clear();
         _deltaCombined->clear();
         //auto start = RuntimeMonitor::now();
         computeUp();
         //timeDoingUp += RuntimeMonitor::elapsedSinceMicro(start);
         _sf->clear();
         computeDown(iter);
         assert(layers[numVariables].size() == 1);
         if (!_mddspec.usesUp()) _bwd->clear();
         change = !_fwd->empty() || !_bwd->empty();
         for(int l=0;l < (int) numVariables;l++)
            trimVariable(l);
      } while (change);
      assert(layers[numVariables].size() == 1);
      _mddspec.reachedFixpoint(sink->getDownState(),sink->getUpState(),sink->getCombinedState());
      setScheduled(false);
  ONFAIL    
      queue.clear();
      setScheduled(false);
      failNow();
  ENDFAIL
}

void MDDRelax::refreshAll()
{
   _fwd->clear();
   _bwd->clear();
   for(unsigned l=0u;l < numVariables;++l) {
      for(unsigned p=0u;p < layers[l].size();++p) {
         auto n = layers[l][p];
         assert(n->isActive());
         _fwd->enQueue(n);
         if (_mddspec.usesUp())
            _bwd->enQueue(n);
      }
   }
   propagate();
}

void MDDRelax::checkGraph()
{
   for(unsigned l=0u;l < numVariables;l++) {
      for(unsigned i=0u;i < layers[l].size();i++) {
         auto n = layers[l][i];
         n->isActive(); // silence the compiler warnings
         assert(n->isActive());
         assert(l == 0 || n->getNumParents() > 0);
         assert(l == numVariables || n->getNumChildren() > 0);
      }
   }
}

void MDDRelax::debugGraph()
{
   using namespace std;
   for(unsigned l=0u;l < numVariables;l++) {
      cout << "L[" << l <<"] = " << layers[l].size() << endl;
      cout << "REF:" << _refs[l] << endl;
      for(unsigned i=0u;i < layers[l].size();i++) {
         cout << i << ":   " << layers[l][i]->getDownState()  << '\n';
      }
   }
}



namespace Factory {
   MDD* makeMDD(CPSolver::Ptr cp)
   {
      return new MDDRelax(cp,256);
      //return new MDD(cp);
   }
   
   MDDRelax* makeMDDRelax(CPSolver::Ptr cp,
                          int width,
                          int maxDistance,
                          int maxSplitIter,
                          bool approxThenExact,
                          int maxConstraintPriority)
   {
      return new MDDRelax(cp,width,maxDistance,maxSplitIter,approxThenExact,maxConstraintPriority);
   }
}
