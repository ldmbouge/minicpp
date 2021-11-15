#include "mddrelax.hpp"
#include <float.h>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <cmath>
#include "RuntimeMonitor.hpp"
#include "heap.hpp"

MDDRelax::MDDRelax(CPSolver::Ptr cp,int width,int maxDistance,int maxSplitIter)
   : MDD(cp),
     _width(width),
     _maxDistance(maxDistance),
     _maxSplitIter(maxSplitIter),
     _rnG(42),
     _sampler(0.0,1.0)
  
{
   _afp = new MDDIntSet[width];
   _src = new MDDNode*[width];
   _fwd = nullptr;
   _bwd = nullptr;
   _pool = new Pool;
   _delta = nullptr;
   _nf->setWidth(width);
}

const MDDState& MDDRelax::pickReference(int layer,int layerSize)
{
   double v = _sampler(_rnG);
   double w = 1.0 / (double)layerSize;
   int c = (int)std::floor(v / w);
   int dirIdx = c;
   return layers[layer].get(dirIdx)->getState();
}

MDDNode* findMatch(const std::multimap<float,MDDNode*>& layer,const MDDState& s,const MDDState& refDir)
{
   float query = s.inner(refDir);
   auto nlt = layer.lower_bound(query);
   while (nlt != layer.end() && nlt->first == query) {
      bool isEqual = nlt->second->getState() == s;
      if (isEqual)
         return nlt->second;
      else nlt++;
   }
   return nullptr;
}

void MDDRelax::buildDiagram()
{
   std::cout << "MDDRelax::buildDiagram" << '\n';
   _mddspec.layout();
   _mddspec.compile();
   _delta = new MDDDelta(_nf,_mddspec.size());

   _fwd = new (mem) MDDFQueue(numVariables+1);
   _bwd = new (mem) MDDBQueue(numVariables+1);
   std::cout << _mddspec << '\n';
   auto uDom = domRange(x);
   const int sz = uDom.second - uDom.first + 1;
   _domMin = uDom.first;
   _domMax = uDom.second;

   for(auto i=0u;i < _width;i++)
      _afp[i] = MDDIntSet((char*)mem->allocate(sizeof(int) * sz),sz);

   auto rootState = _mddspec.rootState(mem);
   auto sinkState = _mddspec.rootState(mem);
   sinkState.computeHash();
   rootState.computeHash();
   sink = _nf->makeNode(sinkState,0,(int)numVariables,0);
   root = _nf->makeNode(rootState,x[0]->size(),0,0);
   layers[0].push_back(root,mem);
   layers[numVariables].push_back(sink,mem);

   auto start = RuntimeMonitor::now();
   _refs.emplace_back(rootState);
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
   MDDState state(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSize()));
   int nbVals = x[i]->size();
   char* buf = (char*)alloca(sizeof(int)*nbVals);
   MDDIntSet xv(buf,nbVals);
   for(int v = x[i]->min(); v <= x[i]->max(); v++) 
      if(x[i]->contains(v)) xv.add(v);
   assert(layers[i].size() == 1);
   auto parent = layers[i][0];
   if (i < numVariables - 1) {
      _sf->createState(state,parent->getState(),i,x[i],xv,false);
      MDDNode* child = _nf->makeNode(state,x[i]->size(),i+1,(int)layers[i+1].size());
      layers[i+1].push_back(child,mem);
      for(auto v : xv) {
         parent->addArc(mem,child,v);
         addSupport(i,v);
      }
   } else {
      MDDState sinkState(sink->getState());
      _mddspec.copyStateUp(state,sink->getState());
      _sf->createState(state, parent->getState(), i, x[i],xv,false);
      assert(sink->getNumParents() == 0);
      sinkState.copyState(state);
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
      MDDState ss(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSize()));
      ss.copyState(sink->getState());
      _mddspec.updateNode(ss);
      if (ss != sink->getState()) 
         sink->setState(ss,mem);      
      for(int i = (int)numVariables - 1;i >= 0;i--) 
         for(auto& n : layers[i]) {
            bool dirty = processNodeUp(n,i);
            if (dirty) {
               if (_mddspec.usesUp()) {
                  for(const auto& pa  : n->getParents())
                     if (pa->getParent()->isActive())
                        _bwd->enQueue(pa->getParent());
               }
               for(const auto& arc : n->getChildren())
                  if (arc->getChild()->isActive())
                     _fwd->enQueue(arc->getChild());
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
            removeArc(layer,layer+1,arc.get());
            arc->remove(this);            
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

bool MDDRelax::fullStateDown(MDDState& ms,MDDState& cs,MDDNode* n,int l)
{
   bool first = true;
   for(auto i = 0u;i < _width;i++) {
      if (_src[i]==nullptr) continue;
      auto p = _src[i];                           // this is the parent
      assert(_afp[i].size() > 0);                 // afp[i] is the set of arcs from that parent
      _mddspec.copyStateUp(cs,n->getState());     // grab the up information from the old state
      _sf->createState(cs,p->getState(),l-1,x[l-1],_afp[i],true); // compute a full scale transitions (all props).
      //assert(cs.getBS(6) == cs.getBS(7));
      if (first)
         ms.copyState(cs); // install the result into an accumulator
      else {
         if (ms != cs) {
            //assert(ms.getBS(6) == ms.getBS(7));
            //assert(cs.getBS(6) == cs.getBS(7));
            _mddspec.relaxation(ms,cs);   // compute a full scale relaxation of cs with the accumulator (ms).
            //assert(ms.getBS(6) == ms.getBS(7));
            ms.relaxDown();               // indidcate this is a down relaxation.
         }
      }
      first = false;
   }
   _mddspec.updateNode(ms);
   bool isOk =  _mddspec.consistent(ms,x[l-1]);
   return isOk;
}

bool MDDRelax::incrStateDown(const MDDPropSet& out,MDDState& ms,MDDState& cs,MDDNode* n,int l)
{
   bool first = true;
   for(auto i = 0u;i < _width;i++) {
      if (_src[i]==nullptr) continue;
      auto p = _src[i];                           // this is the parent
      assert(_afp[i].size() > 0);                 // afp[i] is the set of arcs from that parent
      cs.copyState(n->getState());       // grab the up information from the old state
      _mddspec.createStateIncr(out,cs,p->getState(),l-1,x[l-1],_afp[i],true); // compute a full scale transitions (all props).
      if (first)
         ms.copyState(cs); // install the result into an accumulator
      else {
         if (ms != cs) {
            _mddspec.relaxationIncr(out,ms,cs);   // compute an incremental  relaxation of cs with the accumulator (ms). 
            ms.relaxDown();               // indidcate this is a down relaxation.
         } else ms.relaxDown(n->getState().isDownRelaxed());
      }
      first = false;
   }
   _mddspec.updateNode(ms);
   bool isOk =  _mddspec.consistent(ms,x[l-1]);
   return isOk;
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

bool MDDRelax::refreshNodeFull(MDDNode* n,int l)
{
   if (l == 0) {
      assert(n->getNumParents() == 0);
      bool isOk = _mddspec.consistent(n->getState(), x[0]);
      if (!isOk) failNow();
      return false;
   }
   aggregateValueSet(n);
   MDDState cs(&_mddspec,(char*)alloca(_mddspec.layoutSize()));
   MDDState ms(&_mddspec,(char*)alloca(_mddspec.layoutSize()));

   bool isOk = fullStateDown(ms,cs,n,l);
   bool internal = l > 0 && l < (int)numVariables;
   if (!internal) {
      if (!isOk) failNow();
   } else {
      if (!isOk) {
         delState(n,l);
         return true;
      }
   }
   bool changed = n->getState() != ms;
   if (changed) {
      _delta->setDelta(n,ms);
      n->setState(ms,mem);
   }
   return changed;
}

bool MDDRelax::refreshNodeIncr(MDDNode* n,int l)
{
   if (l == 0) {
      assert(n->getNumParents() == 0);
      bool isOk = _mddspec.consistent(n->getState(), x[0]);
      if (!isOk) failNow();
      return false;
   }
   aggregateValueSet(n);
   MDDPropSet changes((long long*)alloca(sizeof(long long)*propNbWords(_mddspec.size())),_mddspec.size());   
   for(auto& a : n->getParents()) 
      changes.unionWith(_delta->getDelta(a->getParent()));
   const bool parentsChanged = n->parentsChanged();
      
   MDDState cs(&_mddspec,(char*)alloca(_mddspec.layoutSize()));
   MDDState ms(&_mddspec,(char*)alloca(_mddspec.layoutSize()));

   // Causes of "parentsChanged":
   // (1) arc removal, (2) arc addition to an existing parent, (3) arc addition to a new parent node.
   // Causes of changes != empty:
   // at least one Parent state is different.
   MDDPropSet out((long long*)alloca(sizeof(long long)*changes.nbWords()),changes.nbProps());
   _mddspec.outputSet(out,changes);

   bool isOk;
   if (parentsChanged) 
      isOk = fullStateDown(ms,cs,n,l);
   else  {
      isOk = incrStateDown(out,ms,cs,n,l);
   }

   bool internal = l > 0 && l < (int)numVariables;
   if (!internal) {
      if (!isOk) failNow();
   } else {
      if (!isOk) {
         delState(n,l);
         return true;
      }
   }
   bool changed = n->getState() != ms;
   if (parentsChanged) {
      if (changed) {
         _delta->setDelta(n,ms);
         n->setState(ms,mem);
      }
   } else {
      if (changed) {
         _delta->setDelta(n,out);
         n->setState(ms,mem);
      }
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
         if (!_mddspec.exist(n->getState(),child->getState(),x[l],v,true)) {
            n->unhook(arc);
            changed = true;
            delSupport(l,v);
            removeArc(l,l+1,arc.get());
            if (child->getNumParents()==0) delState(child,l+1);
         }
      }
      if (n->getNumChildren()==0 && l != (int)numVariables) {
         delState(n,l);
         assert(layers[numVariables].size() == 1);
         changed = true;
      }
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
      bool operator()(const PQValue& a,const PQValue& b) { return a._key < b._key;}
   };
   Pool::Ptr _mem;
   const TVec<MDDNode*>& _layer;
   const MDDSpec& _mddspec;
   Heap<PQValue,PQOrder>  _pq;
public:
   MDDNodeSim(Pool::Ptr mem,const TVec<MDDNode*>& layer,const MDDState& ref,const MDDSpec& mddspec)
      : _mem(mem),
        _layer(layer),
        _mddspec(mddspec),
        _pq(mem,layer.size())
   {
      for(auto& n : _layer)
         if (n->getState().isRelaxed() && n->getNumParents() > 1) {
            double key = (_mddspec.hasSplitRule()) ? _mddspec.splitPriority(*n) : (double)n->getPosition();
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
   bool*   _keepKids;
public:
   MDDPotential(Pool::Ptr pool,MDDNode* n,const int mxPar,MDDNode* par,MDDEdge::Ptr arc,
                MDDState* child,int v,int nbKids,bool* kk)
      : _mem(pool),_n(n),_par(par),_mxPar(mxPar),_nbPar(0) {
      _child = child;
      _arc = new (pool) MDDEdge::Ptr[_mxPar];
      _arc[_nbPar++] = arc;
      _val = v;
      _nbk = nbKids;
      _keepKids = new (_mem) bool[_nbk];
      for(int i=0;i < _nbk;i++) _keepKids[i] = kk[i];
   }
   void computeKey(const MDDSpec& mddspec) {
      if  (mddspec.hasSplitRule())
         _key = mddspec.splitPriority(*_par);
      else _key = (double) _par->getPosition();
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
   void linkChild(int reuse,MDDEdge::Ptr arc) {
      _candidates[reuse]->link(arc);
   }
   template <class CB>
   void process(TVec<MDDNode*>& layer,unsigned long width,Trailer::Ptr trail,Storage::Ptr mem,const CB& cb) {
      if (_candidates.size() + layer.size() <= width) {
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
      if (p->getState() == s)
         return p;   
   return nullptr;
}


int splitCS = 0,pruneCS = 0,potEXEC = 0;

int MDDRelax::splitNode(MDDNode* n,int l,MDDSplitter& splitter)
{
   int lowest = l;
   MDDState* ms = nullptr;
   const int nbParents = (int) n->getNumParents();
   auto last = --(n->getParents().rend());
   for(auto pit = n->getParents().rbegin(); pit != last;pit++) {
      auto a = *pit;                // a is the arc p --(v)--> n
      auto p = a->getParent();      // p is the parent
      auto v = a->getValue();       // value on arc from parent
      bool isOk = _sf->splitState(ms,n,p->getState(),l-1,x[l-1],v);
      splitCS++;         
      if (!isOk) {
         pruneCS++;
         p->unhook(a);
         if (p->getNumChildren()==0) lowest = std::min(lowest,delState(p,l-1));
         delSupport(l-1,v);
         removeArc(l-1,l,a.get());
         if (_mddspec.usesUp() && p->isActive()) _bwd->enQueue(p);
         if (lowest < l) return lowest;
         continue;
      }         
      MDDNode* bj = findMatchInLayer(layers[l],*ms);
      if (bj) {
         if (bj != n) 
            a->moveTo(bj,trail,mem);            
         // If we matched to n nothing to do. We already point to n.
      } else { // There is an approximate match
         // So, if there is room create a new node
         int reuse = splitter.hasState(*ms);
         if (reuse != -1) {
            splitter.linkChild(reuse,a);
         } else {
            int nbk = n->getNumChildren();
            bool keepArc[nbk];
            unsigned idx = 0,cnt = 0;
            for(auto ca : n->getChildren()) 
               cnt += keepArc[idx++] = _mddspec.exist(*ms,ca->getChild()->getState(),x[l],ca->getValue(),true);
            if (cnt == 0) {
               pruneCS++;               
               p->unhook(a);
               if (p->getNumChildren()==0) lowest = std::min(lowest,delState(p,l-1));
               delSupport(l-1,v);
               removeArc(l-1,l,a.get());
               if (_mddspec.usesUp() && p->isActive()) _bwd->enQueue(p);
               if (lowest < l) return lowest;
            } else {
               splitter.addPotential(_pool,n,nbParents,p,a,ms,v,nbk,(bool*)keepArc);
            }
         }
      } //out-comment
   } // end of loop over parents.
   _fwd->enQueue(n);
   _bwd->enQueue(n);
   return lowest;
}
int MDDRelax::splitNodeApprox(MDDNode* n,int l,MDDSplitter& splitter)
{
   std::map<int,MDDState*> equivalenceClasses;
   std::multimap<int,MDDEdge::Ptr> arcsPerClass;
   int lowest = l;
   MDDState* ms = nullptr;
   const int nbParents = (int) n->getNumParents();
   auto last = --(n->getParents().rend());
   for(auto pit = n->getParents().rbegin(); pit != last;pit++) {
      auto a = *pit;                // a is the arc p --(v)--> n
      auto p = a->getParent();      // p is the parent
      auto v = a->getValue();       // value on arc from parent
      bool isOk = _sf->splitState(ms,n,p->getState(),l-1,x[l-1],v);
      splitCS++;         
      if (!isOk) {
         pruneCS++;
         p->unhook(a);
         if (p->getNumChildren()==0) lowest = std::min(lowest,delState(p,l-1));
         delSupport(l-1,v);
         removeArc(l-1,l,a.get());
         if (_mddspec.usesUp() && p->isActive()) _bwd->enQueue(p);
         if (lowest < l) return lowest;
         continue;
      }         
      MDDNode* bj = findMatchInLayer(layers[l],*ms);
      if (bj && bj != n) {
         a->moveTo(bj,trail,mem);            
      } else { // There is an approximate match
         // So, if there is room create a new node
         int equivalenceValue = _mddspec.equivalenceValue(p->getState(),*ms,x[l-1],v);
         if (equivalenceClasses.count(equivalenceValue)) {
            auto existing = equivalenceClasses.at(equivalenceValue);
            if (ms != existing) {
               _mddspec.relaxation(*existing,*ms);
               equivalenceClasses.at(equivalenceValue)->relaxDown();
            }
            arcsPerClass.insert(std::make_pair(equivalenceValue, a));
         } else {
            int nbk = n->getNumChildren();
            bool keepArc[nbk];
            unsigned idx = 0,cnt = 0;
            for(auto ca : n->getChildren()) 
               cnt += keepArc[idx++] = _mddspec.exist(*ms,ca->getChild()->getState(),x[l],ca->getValue(),true);
            if (cnt == 0) {
               pruneCS++;               
               p->unhook(a);
               if (p->getNumChildren()==0) lowest = std::min(lowest,delState(p,l-1));
               delSupport(l-1,v);
               removeArc(l-1,l,a.get());
               if (_mddspec.usesUp() && p->isActive()) _bwd->enQueue(p);
               if (lowest < l) return lowest;
            } else {
               equivalenceClasses[equivalenceValue] = ms;
               arcsPerClass.insert({equivalenceValue, a});
            }
         }
      } //out-comment
   } // end of loop over parents.

   for (auto it = equivalenceClasses.begin(); it != equivalenceClasses.end(); ++it) {
      int equivalenceValue = it->first;
      MDDState* newState = it->second;
      int nbk = n->getNumChildren();
      bool keepArc[nbk];
      unsigned idx = 0,cnt = 0;
      for(auto ca : n->getChildren())
         cnt += keepArc[idx++] = _mddspec.exist(*newState,ca->getChild()->getState(),x[l],ca->getValue(),true);
      bool addedToSplitter = false;
      for (auto arcIt = arcsPerClass.begin(); arcIt != arcsPerClass.end(); ++arcIt) {
         if (arcIt->first == equivalenceValue) {
            MDDEdge::Ptr arc = arcIt->second;
            if (addedToSplitter) {
               splitter.linkChild(splitter.hasState(*newState), arc);
            } else {
               addedToSplitter = true;
               auto p = arc->getParent();
               auto v = arc->getValue();
               splitter.addPotential(_pool,n,nbParents,p,arc,newState,v,nbk,(bool*)keepArc);
            }
         }
      }
   }
   _fwd->enQueue(n);
   _bwd->enQueue(n);
   return lowest;
}


void MDDRelax::splitLayers() // this can use node from recycled or add node to recycle
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
         MDDNodeSim nSim(_pool,layers[l],_refs[l],_mddspec);
         MDDNode* n = nullptr;
         while (layer.size() < _width && lowest==l) {
            while(splitter.size() == 0)  {
               n = nSim.extractNode();
               if (n) {
                  if (_mddspec.approxEquivalence())
                     lowest = splitNodeApprox(n,l,splitter);
                  else
                     lowest = splitNode(n,l,splitter);
               } else break;
            }
            if (lowest < l) break;
            splitter.process(layer,_width,trail,mem,
                             [this,l,&layer](MDDNode* n,MDDNode* p,const MDDState& ms,int val,int nbk,bool* kk) {
                                potEXEC++;
                                MDDNode* nc = _nf->makeNode(ms,x[l-1]->size(),l,(int)layer.size());
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
            if (n==nullptr) break;
         }
         ++nbSplits;
      } // end-if (there is room and variable is not bound)
      auto jump = std::min(l - lowest,_maxDistance);
      l = (lowest < l) ? l-jump : l + 1;      
   }
}

struct MDDStateEqual {
   bool operator()(const MDDState* s1,const MDDState* s2) const { return *s1 == *s2;}
};

int MDDRelax::delState(MDDNode* node,int l)
{
   if (l==0 || l == (int)numVariables) return numVariables+1;
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

bool MDDRelax::processNodeUp(MDDNode* n,int i) // i is the layer number
{
   MDDState cs(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSize()));
   MDDState ms(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSize()));
   bool first = true;
   for(auto k=0u;k<_width;k++)
      _afp[k].clear();
   for(auto& a : n->getChildren()) {
      auto kid = a->getChild();
      int v = a->getValue();
      _afp[kid->getPosition()].add(v);
   }
   auto wub = std::min(_width,(unsigned)layers[i+1].size());
   for(auto k=0u;k < wub;k++) {
      if (_afp[k].size() > 0) {
         cs.copyState(n->getState());
         auto c = layers[i+1][k];
         _mddspec.updateState(cs,c->getState(),i,x[i],_afp[k]);
         if (first)
            ms.copyState(cs);
         else {
            if (ms != cs) {
               _mddspec.relaxation(ms,cs);
               ms.relaxUp();
            }
         }
         first = false;
      }
   }
   _mddspec.updateNode(ms);
   bool dirty = (n->getState() != ms);
   if (dirty) {
      n->setState(ms,mem);
   }
   return dirty;
}

int __nbn = 0,__nbf = 0;

void MDDRelax::computeDown(int iter)
{
   if (iter <= _maxSplitIter)
      splitLayers();
   _sf->disable();
   while(!_fwd->empty()) {
      MDDNode* node = _fwd->deQueue();
      if (node==nullptr) break;            
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
      bool dirty = refreshNodeIncr(node,l);
      filterKids(node,l);   // must filter unconditionally after a refresh since children may have changed.
      if (dirty && node->isActive()) {
         if (_mddspec.usesUp()) {
            for(const auto& arc : node->getParents())
               if (arc->getParent()->isActive())
                  _bwd->enQueue(arc->getParent());
         }
         for(const auto& arc : node->getChildren())
            if (arc->getChild()->isActive())
               _fwd->enQueue(arc->getChild());
      } 
   }
}

void MDDRelax::computeUp()
{
   if (_mddspec.usesUp())  {
      while (!_bwd->empty()) {
         MDDNode* n = _bwd->deQueue();
         if (n==nullptr) break;
         bool dirty = processNodeUp(n,n->getLayer());
         filterKids(n,n->getLayer());
         if (dirty && n->isActive()) {
            if (_mddspec.usesUp()) {
               for(const auto& pa  : n->getParents())
                  if (pa->getParent()->isActive())
                     _bwd->enQueue(pa->getParent());
            }
            for(const auto& arc : n->getChildren())
               if (arc->getChild()->isActive())
                  _fwd->enQueue(arc->getChild());
         }
      }
   } else _bwd->clear();
}

int iterMDD = 0;

void MDDRelax::propagate()
{
   try {
      setScheduled(true);
      bool change = false;
      MDD::propagate();
      int iter = 0;
      do {
         _fwd->init(); 
         _bwd->init();
         ++iterMDD;++iter;
         _delta->clear();
         computeUp();
         _sf->clear();
         computeDown(iter);
         assert(layers[numVariables].size() == 1);
         if (!_mddspec.usesUp()) _bwd->clear();
         change = !_fwd->empty() || !_bwd->empty();
         for(int l=0;l < (int) numVariables;l++)
            trimVariable(l);
      } while (change);
      assert(layers[numVariables].size() == 1);
      _mddspec.reachedFixpoint(sink->getState());
      setScheduled(false);
  } catch(Status s) {
      queue.clear();
      setScheduled(false);
      throw s;
   }
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
         cout << i << ":   " << layers[l][i]->getState()  << '\n';
      }
   }
}
