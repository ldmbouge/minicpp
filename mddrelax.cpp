#include "mddrelax.hpp"
#include <float.h>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <cmath>
#include "RuntimeMonitor.hpp"

MDDRelax::MDDRelax(CPSolver::Ptr cp,int width)
   : MDD(cp),
     _width(width),
     _rnG(42),
     _sampler(0.0,1.0)
  
{
   _afp = new MDDIntSet[width];
   _src = new MDDNode*[width];
   _fwd = nullptr;
   _bwd = nullptr;
   _pool = new Pool;
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
   _fwd = new (mem) MDDFQueue(numVariables+1);
   _bwd = new (mem) MDDBQueue(numVariables+1);
   std::cout << _mddspec << '\n';
   auto uDom = domRange(x);
   const int sz = uDom.second - uDom.first + 1;
   _domMin = uDom.first;
   _domMax = uDom.second;

   for(auto i=0u;i < _width;i++)
      _afp[i] = MDDIntSet((char*)mem->allocate(sizeof(int) * sz * _width),sz * _width);
   
   auto rootState = _mddspec.rootState(mem);
   auto sinkState = _mddspec.rootState(mem);
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

   std::stable_sort(cl.begin(),cl.end(),[](const auto& p1,const auto& p2) {
                                           return std::get<0>(p1) < std::get<0>(p2);
                                        });

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
               for(const auto& pa  : n->getParents())
                  _bwd->enQueue(pa->getParent());
               for(const auto& arc : n->getChildren())
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
   _bwd->enQueue(arc->getParent());
   _fwd->enQueue(arc->getChild());
}


bool MDDRelax::refreshNode(MDDNode* n,int l)
{
   if (l == 0) {
      assert(n->getNumParents() == 0);
      bool isOk = _mddspec.consistent(n->getState(), x[0]);
      if (!isOk) failNow();
      return false;
   }
   MDDState cs(&_mddspec,(char*)alloca(_mddspec.layoutSize()));
   MDDState ms(&_mddspec,(char*)alloca(_mddspec.layoutSize()));
   bool first = true;
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

   for(auto i = 0u;i < _width;i++) {
      if (_src[i]==nullptr) continue;
      auto p = _src[i];      
      assert(_afp[i].size() > 0);
      _mddspec.copyStateUp(cs,n->getState());     
      _mddspec.createState(cs,p->getState(),l-1,x[l-1],_afp[i],true);
      if (first)
         ms.copyState(cs);
      else {
         if (ms != cs) {
            _mddspec.relaxation(ms,cs);
            ms.relaxDown();
         }
      }
      first = false;
   }
   _mddspec.updateNode(ms);
   bool isOk =  _mddspec.consistent(ms,x[l-1]);
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
   if (changed) 
      n->setState(ms,mem);  
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
   const MDDState& _refDir;
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
            _cl.push_back(NRec {n,n->getState().inner(_refDir)});
      }     
   }
public:
   MDDNodeSim(const TVec<MDDNode*>& layer,const MDDState& ref,const MDDSpec& mddspec)
      : _layer(layer),_ready(false),_refDir(ref),_mddspec(mddspec) 
   {
      _pq.reserve(_layer.size());      
      for(auto& n : _layer)
         if (n->getState().isRelaxed()) {            
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
      //std::cout << "CLSIZE:" << _cl.size() << '\n';
      //float query = s.inner(_refDir);
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
      _cl.emplace_back(NRec { nc,nc->getState().inner(_refDir) } );
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
   MDDState       _child;
   double           _key;
   int       _val;
   int      _nbk;
   bool*   _keepKids;
public:
   MDDPotential(Pool::Ptr pool,const int mxPar,MDDNode* par,MDDEdge::Ptr arc,const MDDState& child,int v,int nbKids,bool* kk)
      : _mem(pool),_par(par),_mxPar(mxPar),_nbPar(0) {
      MDDStateSpec* spec = child.getSpec();
      _child = MDDState(spec,new (_mem) char[spec->layoutSize()]);
      _child.copyState(child);
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
   bool hasState(const MDDState& s) const { return _child == s;}
   void link(MDDEdge::Ptr arc) {
      assert(_nbPar < _mxPar);
      _arc[_nbPar++] = arc;
   }
   template <class Callback> void instantiate(const Callback& cb,Trailer::Ptr trail,Storage::Ptr mem)
   {
      MDDNode* nc = cb(_par,_child,_val,_nbk,_keepKids);
      for(int i=0;i < _nbPar;i++)
         _arc[i]->moveTo(nc,trail,mem);      
   }
};

class MDDSplitter {
   Pool::Ptr                        _pool;
   int                             _width;
   std::vector<MDDPotential*> _candidates;
   const MDDSpec&                _mddspec;
public:
   MDDSplitter(Pool::Ptr pool,const MDDSpec& spec,int w) : _pool(pool),_width(w),_mddspec(spec){
      _candidates.reserve(2 * _width);
   }
   void clear() { _candidates.clear();}
   auto size() { return _candidates.size();}
   template <class... Args> 
   void addPotential(Args&&... args) {
      _candidates.push_back(new (_pool) MDDPotential(std::forward<Args>(args)...));
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
      } else {
         for(auto& c : _candidates)
            c->computeKey(_mddspec);
         auto order = [](const auto& a,const auto& b) { return a->getKey() < b->getKey();};
         std::make_heap(_candidates.begin(),_candidates.end(),order);
         auto first = _candidates.begin(),last = _candidates.end();
         while (first != last) {
            MDDPotential* p = *first;
            std::pop_heap(first,last,order);
            _candidates.pop_back();
            last = _candidates.end();
            p->instantiate(cb,trail,mem);
            if (layer.size() >= width)
               break;
         }
      }
   }
};

int MDDRelax::split(TVec<MDDNode*>& layer,int l) // this can use node from recycled or add node to recycle
{
   using namespace std;
   int lowest = l;
   //std::cout << "\nStarting: LS=" << layer.size() << "\n";
   MDDNodeSim nSim(layer,_refs[l],_mddspec);
   MDDState ms(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSize()));
   MDDNode* n = nullptr;
   MDDSplitter splitter(_pool,_mddspec,_width);
   _pool->clear();
   while (layer.size() < _width && lowest == l && (n = nSim.extractNode()) != nullptr) {
      //assert(n->getNumParents() > 0);
      assert(n->getState().isRelaxed());
      splitter.clear();
      const int nbParents = n->getNumParents();
      for(auto pit = n->getParents().rbegin(); pit != n->getParents().rend();pit++) {
         auto a = *pit;                // a is the arc p --(v)--> n
         auto p = a->getParent();      // p is the parent
         auto v = a->getValue();       // value on arc from parent
         _mddspec.copyStateUp(ms,n->getState());
         _mddspec.createState(ms,p->getState(),l-1,x[l-1],MDDIntSet(v),true);
         _mddspec.updateNode(ms);
         bool isOk = _mddspec.consistent(ms,x[l-1]);
         if (!isOk) {
            p->unhook(a);
            if (p->getNumChildren()==0) lowest = std::min(lowest,delState(p,l-1));
            delSupport(l-1,v);
            removeArc(l-1,l,a.get());
            _bwd->enQueue(p);
            if (lowest < l) return lowest;
            continue;
         }
         
         MDDNode* bj = nSim.findMatch(ms);
         if (bj) {
         //if (bj->getState() == ms) { // there is a perfect match
            if (bj != n) 
               a->moveTo(bj,trail,mem);            
            // If we matched to n nothing to do. We already point to n.
         } else { // There is an approximate match
            // So, if there is room create a new node
            //if (layer.size() >= _width)  continue;
            
            int nbk = n->getNumChildren();
            bool keepArc[nbk];
            unsigned idx = 0,cnt = 0;
            for(auto ca : n->getChildren()) 
               cnt += keepArc[idx++] = _mddspec.exist(ms,ca->getChild()->getState(),x[l],ca->getValue(),true);
            if (cnt == 0) {
               p->unhook(a);
               if (p->getNumChildren()==0) lowest = std::min(lowest,delState(p,l-1));
               delSupport(l-1,v);
               removeArc(l-1,l,a.get());
               _bwd->enQueue(p);
               if (lowest < l) return lowest;
            } else {
               int reuse = splitter.hasState(ms);
               if (reuse != -1) {
                  splitter.linkChild(reuse,a);
               } else {
                  splitter.addPotential(_pool,nbParents,p,a,ms,v,nbk,(bool*)keepArc);
               }
            }
         }//out-comment
      } // end of loop over parents.
      splitter.process(layer,_width,trail,mem,
                       [this,&nSim,n,l,&layer](MDDNode* p,const MDDState& ms,int val,int nbk,bool* kk) {
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
                          _bwd->enQueue(nc);
                          nSim.insert(nc);
                          return nc;
                       });
      
      if (n->getNumParents()==0) {
         delState(n,l);
         nSim.removeMatch(n);
      } else {
         bool refreshed = refreshNode(n,l);
         filterKids(n,l);
         if (refreshed && n->isActive()) {
            for(const auto& arc : n->getParents())
               _bwd->enQueue(arc->getParent());
            for(const auto& arc : n->getChildren())
               _fwd->enQueue(arc->getChild());
         }
      }
   } // end of loop over relaxed node in layer l
   return lowest;
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
         removeArc(l-1,l,arc.get());
      }
      node->clearParents();
   }
   if (node->getNumChildren() > 0) {
      for(auto& arc : node->getChildren()) {
         if (arc->getChild()->unhookIncoming(arc))
            lowest = std::min(lowest,delState(arc->getChild(),l+1));
         delSupport(l,arc->getValue());
         removeArc(l,l+1,arc.get());
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
   if (dirty)
      n->setState(ms,mem);
   return dirty;
}

void MDDRelax::computeDown(int iter)
{
   if (true || iter <= 1) {
      int l=1;
      while (l < (int) numVariables) {
         int lowest = l;
         if (!x[l-1]->isBound() && layers[l].size() < _width) 
            lowest = split(layers[l],l);
         l = (lowest < l) ? lowest : l + 1;
         //l += 1;
      }
   }
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
      assert(layers[numVariables].size() == 1);
      assert(node->isActive());
      bool dirty = refreshNode(node,l);
      assert(layers[numVariables].size() == 1);
      filterKids(node,l);   // must filter unconditionally after a refresh since children may have changed.
      assert(layers[numVariables].size() == 1);
      if (dirty && node->isActive()) {
         for(const auto& arc : node->getParents())
            _bwd->enQueue(arc->getParent());
         for(const auto& arc : node->getChildren())
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
            for(const auto& pa  : n->getParents())
               _bwd->enQueue(pa->getParent());
            for(const auto& arc : n->getChildren())
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
         computeUp();
         computeDown(iter);
         assert(layers[numVariables].size() == 1);
         if (!_mddspec.usesUp()) _bwd->clear();
         change = !_fwd->empty() || !_bwd->empty();
      } while (change); //  && iter <= 2);
      //iterMDD += iter;
      assert(layers[numVariables].size() == 1);
      _mddspec.reachedFixpoint(sink->getState());
      for(int l=0;l < (int) numVariables;l++)
         trimVariable(l);
      setScheduled(false);
      //std::cout << "FIX=" << iter << '\n';
  } catch(Status s) {
      queue.clear();
      setScheduled(false);
      throw s;
   }
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
         cout << i << ":   " << layers[l][i]->getState() << ":"
              << layers[l][i]->getState().inner(_refs[l]) << endl;
      }
   }
}
