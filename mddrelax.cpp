#include "mddrelax.hpp"
#include <float.h>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <cmath>
#include "RuntimeMonitor.hpp"
#include "heap.hpp"

MDDRelax::MDDRelax(CPSolver::Ptr cp,int width,int maxDistance)
   : MDD(cp),
     _width(width),
     _maxDistance(maxDistance),
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
   //_afp[i] = MDDIntSet((char*)mem->allocate(sizeof(int) * sz * _width),sz * _width);
   
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
      relaxLayer(i+1,std::min(1u,_width));//_width);
   }
   postUp();
   trimDomains();
   auto dur = RuntimeMonitor::elapsedSince(start);
   std::cout << "build/Relax:" << dur << '\n';
   propagate();
   hookupPropagators();
}


// pairwise (k-means) based relaxation.
/*
void MDDRelax::relaxLayer(int i)
{
   _refs.emplace_back(pickReference(i,(int)layers[i].size()).clone(mem));   
   if (layers[i].size() <= _width)
      return;   

   std::mt19937 rNG;
   rNG.seed(42);
   std::vector<int>  cid(_width);
   std::vector<MDDState> means(_width);
   char* buf = new char[_mddspec.layoutSize() * _width];
   std::vector<int> perm(layers[i].size());
   std::iota(perm.begin(),perm.end(),0);
   std::shuffle(perm.begin(),perm.end(),rNG);
   int k = 0;
   for(auto& mk : means) {
      mk = MDDState(&_mddspec,buf + k * _mddspec.layoutSize());
      cid[k] = perm[k];
      mk.initState(layers[i][cid[k++]]->getState());
   }
   std::vector<int> asgn(layers[i].size());
   const int nbIter = 1;
   for(int ni=0;ni < nbIter;ni++) {
      for(int j=0; j < layers[i].size();j++) {
         double bestSim = std::numeric_limits<double>::max();
         int    idx = -1;
         const auto& ln = layers[i][j]->getState();
         for(int c = 0;c < means.size();c++) {
            double sim = _mddspec.similarity(ln,means[c]);
            idx     = (bestSim < sim) ? idx : c;
            bestSim = (bestSim < sim) ? bestSim : sim;
         }
         assert(j >= 0 && j < layers[i].size());
         asgn[j] = idx;
      }      
   }
   for(int j=0;j < asgn.size();j++) {
      MDDNode* target = layers[i][cid[asgn[j]]];
      MDDNode* strip = layers[i][j];
      if (target != strip) {
         _mddspec.relaxation(means[asgn[j]],layers[i][j]->getState());
         means[asgn[j]].relax();
         for(auto i = strip->getParents().rbegin();i != strip->getParents().rend();i++) {
            auto arc = *i;
            arc->moveTo(target,trail,mem);
         }
      }
   }
   std::vector<MDDNode*> nl(_width,nullptr);
   for(int j=0;j < means.size();j++) {
      nl[j] = layers[i][cid[j]];
      nl[j]->setState(means[j],mem);
   }
   
   layers[i].clear();
   k = 0;
   for(MDDNode* n : nl) {
      layers[i].push_back(n,mem);
      n->setPosition(k++,mem);
   }
   delete []buf;
}
*/

// "inner product"  based relaxation.
/*
void MDDRelax::relaxLayer(int i,unsigned int width)
{
   _refs.emplace_back(pickReference(i,(int)layers[i].size()).clone(mem));   
   if (layers[i].size() <= width)
      return;   
   const int iSize = (int)layers[i].size();  
   const MDDState& refDir = _refs[i];

   std::vector<std::tuple<float,MDDNode*>> cl(iSize);
   unsigned int k = 0;
   for(auto& n : layers[i])
      cl[k++] = std::make_tuple(n->getState().inner(refDir),n);

   // std::stable_sort(cl.begin(),cl.end(),[](const auto& p1,const auto& p2) {
   //                                         return std::get<0>(p1) < std::get<0>(p2);
   //                                      });

   const int bucketSize = iSize / width;
   int   rem = iSize % width;
   int   lim = bucketSize + ((rem > 0) ? 1 : 0);
   rem = rem > 0 ? rem - 1 : 0;
   int   from = 0;
   char* buf = (char*)alloca(sizeof(char)*_mddspec.layoutSize());
   
   std::multimap<float,MDDNode*,std::less<float> > cli;
   std::vector<MDDNode*> nl;
   for(k=0;k < width;k++) { // k is the bucket id
      memset(buf,0,_mddspec.layoutSize());
      MDDState acc(&_mddspec,buf);
      MDDNode* target = std::get<1>(cl[from]);
      acc.initState(target->getState());
      for(from++; from < lim;from++) {
         MDDNode* strip = std::get<1>(cl[from]);         
         _mddspec.relaxation(acc,strip->getState());
         acc.relaxDown();
         for(auto i = strip->getParents().rbegin();i != strip->getParents().rend();i++) {
            auto arc = *i;
            arc->moveTo(target,trail,mem);
         }
      }
      //acc.hash();
      target->setState(acc,mem);

      MDDNode* found = findMatch(cli,target->getState(),refDir);
      if (found) {
         for(auto i=target->getParents().rbegin();i != target->getParents().rend();i++)
            (*i)->moveTo(found,trail,mem);
      } else {
         nl.push_back(target);
         cli.insert({target->getState().inner(refDir),target});
      }
      lim += bucketSize + ((rem > 0) ? 1 : 0);
      rem = rem > 0 ? rem - 1 : 0;
   }
   layers[i].clear();
   k = 0;
   for(MDDNode* n : nl) {
      layers[i].push_back(n,mem);
      n->setPosition(k++,mem);
   }
   //std::cout << "UMAP-RELAX[" << i << "] :" << layers[i].size() << '/' << iSize << '\n';
}
*/

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


void MDDRelax::relaxLayer(int i,unsigned int width)
{
   assert(width == 1);
   _refs.emplace_back(pickReference(i,(int)layers[i].size()).clone(mem));   
   // if (layers[i].size() <= width)
   //    return;   
   // const int iSize = (int)layers[i].size();  
   // const MDDState& refDir = _refs[i];

   // The 'cl' data-structure is, strictly speaking, not necessary. It gives a permutation of the
   // node from most similar to least simiar to the chosen reference.
   // This is _not_ helpful, since we are merging all of the nodes into a single one anyway. So the
   // resulting state _must_ be the same. Yet, the parent list will have, as a result, a permutation of
   // the arcs. And since splitting proceeds by pulling out arcs in order, the splitting will be different
   // in the end. So I'm leaving this in place for now, but ultimately it should go.
   
   // std::vector<std::tuple<float,MDDNode*>> cl(iSize);
   // int k=0;
   // for(auto& n : layers[i])
   //    cl[k++] = std::make_tuple(n->getState().inner(refDir),n);
   // std::stable_sort(cl.begin(),cl.end(),[](const auto& p1,const auto& p2) {
   //                                         return std::get<0>(p1) < std::get<0>(p2);
   //                                      });


   // char* buf = (char*)alloca(sizeof(char)*_mddspec.layoutSize());
   // memset(buf,0,_mddspec.layoutSize());
   // MDDState acc(&_mddspec,buf);
   // MDDNode* target = std::get<1>(cl[0]);
   // acc.initState(target->getState());
   // for(auto k=1u;k < cl.size();k++) {
   //    const auto& strip = std::get<1>(cl[k]);
   //    _mddspec.relaxation(acc,strip->getState());
   //    acc.relaxDown();
   //    for(auto i = strip->getParents().rbegin();i != strip->getParents().rend();i++) 
   //       (*i)->moveTo(target,trail,mem);      
   //    target->setState(acc,mem);
   // }
   // layers[i].clear();
   // layers[i].push_back(target,mem);
   // target->setPosition(0,mem);
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
      if (first)
         ms.copyState(cs); // install the result into an accumulator
      else {
         if (ms != cs) {
            _mddspec.relaxation(ms,cs);   // compute a full scale relaxation of cs with the accumulator (ms). 
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

   // static int nbTC = 0;
   // static int nbDQ = 0;
   // nbDQ++;
   // nbTC += parentsChanged;
   // if (nbDQ % 1000 == 0)
   //    std::cout << ' ' << (double)nbTC / nbDQ << "\t " << _nf->peakNodes() << "\t " << nbDQ << '\n';

   //std::cout << "STATE:" << changes <<  "\t PAR:" << (parentsChanged ? "T" : "F") << '\n';
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
      //std::cout << "PROPS:" << out << '\n';
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
   const TVec<MDDNode*>& _layer;
   bool _ready;
   //const MDDState& _refDir;
   const MDDSpec& _mddspec;
   struct NRec {
      MDDNode* n;
      float   ip;
   };
   std::vector<NRec> _cl;
   std::vector<std::pair<double,MDDNode*>> _pq;
   std::vector<std::pair<double,MDDNode*>>::iterator _first,_last;
   void initialize() {
      _ready = true;
      if (_cl.size()==0) {
         int nbR = sum(_layer,0,[](const auto& n) { return n->getState().isRelaxed();});
         if (nbR==0) return;
         _cl.reserve(32);
         for(auto& n : _layer)
            _cl.push_back(NRec {n,0}); //n->getState().inner(_refDir)});
      }     
   }
public:
   MDDNodeSim(const TVec<MDDNode*>& layer,const MDDState& ref,const MDDSpec& mddspec)
      : _layer(layer),_ready(false),//_refDir(ref),
        _mddspec(mddspec) 
   {
      _pq.reserve(_layer.size());      
      for(auto& n : _layer)
         if (n->getState().isRelaxed() && n->getNumParents() > 1) {
            double key = _mddspec.hasSplitRule() ? _mddspec.splitPriority(*n) 
               : (double)n->getPosition();
            //std::cout << "A(" << key << ',' << n << "),";
            _pq.emplace_back(std::move(key),std::move(n));
         }
      std::make_heap(_pq.begin(),_pq.end(),[](const auto& a,const auto& b) {
                                              return std::get<0>(a) < std::get<0>(b);
                                           });
      _first = _pq.begin();
      _last  = _pq.end();
   }
   MDDNode* findMatch(const MDDState& s) {
      if (!_ready) initialize();
      for(const auto& rec : _cl)
         if (rec.n->getState() == s)
            return rec.n;
      return nullptr;
   }
   void removeMatch(MDDNode* n) {
      if (!_ready) return;
      for(auto i=_cl.begin();i != _cl.end();i++) {
         if ((*i).n == n) {
            _cl.erase(i);
            return;
         }
      }
      assert(false);
   }
   void insert(MDDNode* nc) {
      _cl.emplace_back(NRec { nc, 0 }); //nc->getState().inner(_refDir) } );
   }
   MDDNode* extractNode() {
      while (_first != _last) {
         double key = 0.0;
         MDDNode* value = nullptr;
         std::tie(key,value) = *_first;
         std::pop_heap(_first,_last,[](const auto& a,const auto& b) {
                                       return std::get<0>(a) < std::get<0>(b);
                                    });
         _pq.pop_back();
         _last = _pq.end();
         //std::cout << "X(" << key << ")";
         return value;
      }
      return nullptr;
   }
};

class MDDPotential {
   Pool::Ptr        _mem;
   MDDNode*         _par;
   int            _mxPar;
   int            _nbPar;
   MDDEdge::Ptr*    _arc;
   const MDDState* _child;
   double           _key;
   int       _val;
   int      _nbk;
   bool*   _keepKids;
public:
   MDDPotential(Pool::Ptr pool,const int mxPar,MDDNode* par,MDDEdge::Ptr arc,const MDDState* child,int v,int nbKids,bool* kk)
      : _mem(pool),_par(par),_mxPar(mxPar),_nbPar(0) {
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
      MDDNode* nc = cb(_par,*_child,_val,_nbk,_keepKids);
      for(int i=0;i < _nbPar;i++)
         _arc[i]->moveTo(nc,trail,mem);      
   }
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
   int hasState(const MDDState& s) {
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

int splitCS = 0,pruneCS = 0,potEXEC = 0;

int MDDRelax::splitNode(MDDNode* n,int l,MDDNodeSim& nSim,MDDSplitter& splitter)
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
      MDDNode* bj = nSim.findMatch(*ms);
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
               splitter.addPotential(_pool,nbParents,p,a,ms,v,nbk,(bool*)keepArc);
            }
         }
      } //out-comment
   } // end of loop over parents.
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
   while (l < (int)numVariables && nbSplits < ub) {
      auto& layer = layers[l];
      int lowest = l;
      trimVariable(l-1);
      ++nbScans;
      if (!x[l-1]->isBound() && layers[l].size() < _width) {
         MDDNodeSim nSim(layer,_refs[l],_mddspec);
         MDDNode* n = nullptr;
         _pool->clear();
         MDDSplitter splitter(_pool,_mddspec,_width);
         while (layer.size() < _width && lowest == l && (n = nSim.extractNode()) != nullptr) {
            assert(splitter.size()==0);
            lowest = splitNode(n,l,nSim,splitter);
            splitter.process(layer,_width,trail,mem,
                             [this,&nSim,n,l,&layer](MDDNode* p,const MDDState& ms,int val,int nbk,bool* kk) {
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
                                nSim.insert(nc);
                                return nc;
                             });
            // cout << "SSZ[" << l << "]:" << splitter.size() << endl;
         } // end-while
         ++nbSplits;
      } // end-if
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
         //removeArc(l-1,l,arc.get());
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
         //removeArc(l,l+1,arc.get());
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
      //_delta->setDelta(n,ms);
      n->setState(ms,mem);
   }
   return dirty;
}

int __nbn = 0,__nbf = 0;

void MDDRelax::computeDown(int iter)
{
   if (iter <= 5)
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
   // std::cout << "n/f:" << __nbn << '/' << __nbf << "\t ITER:" << iter << " \tHTS:" << _sf->size() 
   //           << " scan:" << nbScans << " \tsplit:" << nbSplits << " \tM:" << (double)nbSplits/numVariables << std::endl;
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
         //if (iter % 100 == 0) std::cout << "looping in main fix\n";
      } while (change);//  && iter <= 10);
      //iterMDD += iter;
      assert(layers[numVariables].size() == 1);
      _mddspec.reachedFixpoint(sink->getState());
      setScheduled(false);
      //std::cout << "FIX=" << iter << '\n';
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
