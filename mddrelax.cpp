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
   std::cout << _mddspec << '\n';
   auto uDom = domRange(x);
   const int sz = uDom.second - uDom.first + 1;

   for(auto i=0u;i < _width;i++)
      _afp[i] = MDDIntSet((char*)mem->allocate(sizeof(int) * sz * _width),sz * _width);
   
   auto rootState = _mddspec.rootState(mem);
   auto sinkState = _mddspec.rootState(mem);
   sink = new (mem) MDDNode(_lastNid++,mem, trail, sinkState, 0,(int) numVariables, 0);
   root = new (mem) MDDNode(_lastNid++,mem, trail, rootState, x[0]->size(),0, 0);
   layers[0].push_back(root,mem);
   layers[numVariables].push_back(sink,mem);

   auto start = RuntimeMonitor::now();
   _refs.emplace_back(rootState);
   for(auto i = 0u; i < numVariables; i++) {
      buildNextLayer(i);
      relaxLayer(i+1,1);//_width);
   }
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
         for(auto i = strip->getParents().rbegin();i != strip->getParents().rend();i++) {
            auto arc = *i;
            arc->moveTo(target,trail,mem);
         }
         acc.relax();
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


void MDDRelax::trimLayer(unsigned int layer)
{
   MDD::trimLayer(layer);
}

bool MDDRelax::refreshNode(MDDNode* n,int l)
{
   if (l == 0) {
      assert(n->getNumParents() == 0);
      bool isOk = _mddspec.consistent(n->getState(), x[0]);
      if (!isOk) failNow();
      n->clearDirty();
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
      cs.copyState(n->getState());     
      _mddspec.createState(cs,p->getState(),l-1,x[l-1],_afp[i],true);
      if (first)
         ms.copyState(cs);
      else _mddspec.relaxation(ms,cs);
      first = false;
   }
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
   if (changed) {
      n->setState(ms,mem);
      for(auto i = n->getChildren().rbegin();i != n->getChildren().rend();i++) {
         MDDNode* child = (*i)->getChild();
         int      v  = (*i)->getValue();         
         child->markDirty();
         if (!_mddspec.exist(n->getState(),child->getState(),x[l],v,true)) {
            n->unhook(*i);
            delSupport(l,v);
         }
      }
   } else n->clearDirty();
   return changed;
}

MDDNode* MDDRelax::findSimilar(const std::multimap<float,MDDNode*>& layer,const MDDState& s,const MDDState& refDir)
{
   float query = s.inner(refDir);
   auto nlt = layer.lower_bound(query);
   if (nlt == layer.end()) {
      nlt--;      
      return nlt->second;
   } else {
      auto cur = nlt;
      while(cur != layer.end() && cur->first == query) {
         bool isEqual = cur->second->getState() == s;
         if (cur->second->isActive() && isEqual)
            return cur->second;
         cur++;
      }
      return nlt->second;
   }
}

void removeMatch(std::multimap<float,MDDNode*>& layer,float key,MDDNode* n)
{
   auto i = layer.lower_bound(key);
   while (i != layer.end() && i->first == key) {
      if (i->second == n) {
         layer.erase(i);
         return;
      }
      i++;
   }
   assert(false);
}

bool MDDRelax::filterKids(MDDNode* n,int l)
{
   bool changed = false;
   for(auto i = n->getChildren().rbegin(); i != n->getChildren().rend();i++) {
      auto arc = *i;
      MDDNode* child = arc->getChild();
      int v = arc->getValue();
      if (!_mddspec.exist(n->getState(),child->getState(),x[l],v,true)) {
         n->unhook(arc);
         child->markDirty();
         changed = true;
         delSupport(l,v);
      }
   }         
   return changed;
}

bool MDDRelax::filter(TVec<MDDNode*>& layer,int l)
{
   // variable x[l-1] connects layer `l-1` to layer `l`
   bool changed = false;
   for(auto i = layer.rbegin();i != layer.rend();i++) {
      auto n = *i; // This is a _destination_ node into layer `l`
      if (n->getNumParents()==0 && l > 0) {
         if (l == (int)numVariables)
            failNow();
         delState(n,l);
         changed = true;
         continue;
      }
      if (n->isDirty())
         changed = refreshNode(n,l) || changed;
      changed = filterKids(n,l) || changed;      
   }
   return changed;
}

template <typename Container,typename T,typename Fun> T sum(Container& c,T acc,const Fun& fun) {
   for(auto& term : c) acc += fun(term);
   return acc;
}

bool MDDRelax::split(MDDNodeSet& delta,TVec<MDDNode*>& layer,int l) // this can use node from recycled or add node to recycle
{
   using namespace std;
   if (l==0 || l==(int)numVariables) return false; // We never go through the splitting logic at root/sink
   bool changed = false;
   std::multimap<float,MDDNode*,std::less<float> > cl;
   const MDDState& refDir = _refs[l];
   bool xb = x[l-1]->isBound();
   MDDState ms(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSize()));
   for(auto i = layer.rbegin();i != layer.rend() && !xb && layer.size() < _width;i++) {
      if (cl.size()==0) {
         int nbR = sum(layer,0,[](const auto& n) { return n->getState().isRelaxed();});
         if (nbR==0) break;
         for(auto& n : layer) cl.insert({n->getState().inner(refDir),n});        
      }     
      auto n = *i;
      assert(n->getNumParents() > 0);
      if (!n->getState().isRelaxed()) continue;
      ms.copyState(n->getState());
      for(auto pit = n->getParents().rbegin(); pit != n->getParents().rend();pit++) {
         auto a = *pit;                // a is the arc p --(v)--> n
         auto p = a->getParent();      // p is the parent
         auto v = a->getValue();       // value on arc from parent
         _mddspec.createState(ms,p->getState(),l-1,x[l-1],MDDIntSet(v),true);
         bool isOk = _mddspec.consistent(ms,x[l-1]);
         if (!isOk) {
            delSupport(l-1,v);
            p->unhook(a);
            changed = true;
            continue;
         }
         MDDNode* bj = findSimilar(cl,ms,refDir);
         if (bj->getState() == ms) { // there is a perfect match
            if (bj != n) 
               a->moveTo(bj,trail,mem);            
            // If we matched to n nothing to do. We already point to n.
         } else { // There is an approximate match
            // So, if there is room create a new node
            if (layer.size() >= _width)  continue;
            bool keepArc[n->getNumChildren()];
            unsigned idx = 0,cnt = 0;
            for(auto ca : n->getChildren()) 
               cnt += keepArc[idx++] = _mddspec.exist(ms,ca->getChild()->getState(),x[l],ca->getValue(),true);
            if (cnt == 0) {
               delSupport(l-1,v);
               p->unhook(a);
               changed = true;
            } else {
               MDDNode* nc = new (mem) MDDNode(_lastNid++,mem,trail,ms.clone(mem),x[l-1]->size(),l,(int)layer.size());
               layer.push_back(nc,mem);
               a->moveTo(nc,trail,mem);
               idx = 0;
               for(auto ca : n->getChildren()) {
                  if (keepArc[idx++]) {
                     nc->addArc(mem,ca->getChild(),ca->getValue());
                     addSupport(l,ca->getValue());
                     ca->getChild()->markDirty();
                  }
               }
               changed = true;
               cl.insert({nc->getState().inner(refDir), nc});
            }
         }
      }
      if (n->getNumParents()==0) {
         delState(n,l);
         removeMatch(cl,n->getState().inner(refDir),n);
         changed = true;
      } else
         changed = refreshNode(n,l) || changed;
   }
   return changed;
}

struct MDDStateEqual {
   bool operator()(const MDDState* s1,const MDDState* s2) const { return *s1 == *s2;}
};

void MDDRelax::delState(MDDNode* node,int l)
{
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
         arc->getParent()->unhookOutgoing(arc);
         delSupport(l-1,arc->getValue());         
      }
      node->clearParents();
   }
   if (node->getNumChildren() > 0) {
      for(auto& arc : node->getChildren()) {
         arc->getChild()->unhookIncoming(arc);
         arc->getChild()->markDirty();
         delSupport(l,arc->getValue());
      }
      node->clearChildren();
   }
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

bool MDDRelax::rebuild()
{
   MDDNodeSet delta(2 * _width,(char*)alloca(sizeof(MDDNode*)*2*_width));
   bool changed = false;
   for(unsigned l = 0u; l <= numVariables;l++) {
      // First refresh the down information in the nodes of layer l based on whether those are dirty.
      changed = filter(layers[l],l) || changed;
      changed = split(delta,layers[l],l) || changed; // delta is an _output_ argument from split (splits add to it). 
      bool trim = (l>0) ? trimVariable(l-1) : false;
      changed |= trim;
   }
   return changed;
}

bool MDDRelax::trimDomains()
{
   bool changed = false;
   for(auto i = 1u; i < numVariables;i++) {
      const auto& layer = layers[i];      
      for(int j = (int)layer.size() - 1;j >= 0;j--) {
         if(layer[j]->disconnected()) {
            removeNode(layer[j]);
            changed = true;
         }
      }
   }
   return changed;
}

void MDDRelax::computeUp()
{
   if (_mddspec.usesUp()) {
      //std::cout << "up(" << _lf << " - " << _ff << ") : ";
      MDDState original(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSize()));
      //sink->markDirty();
      for(int i = (int)numVariables - 1;i >= _ff;i--) {
         for(auto& n : layers[i]) {
            bool first = true;           
            for(auto k=0u;k<_width;k++)
               _afp[k].clear();
            for(auto& a : n->getChildren()) {
               auto kid = a->getChild();
               int v = a->getValue();
               _afp[kid->getPosition()].add(v);
            }
            MDDState dest(n->getState());  // This is a direct reference to the internals of n->getState()
            auto wub = std::min(_width,(unsigned)layers[i+1].size());
            for(auto k=0u;k < wub;k++) {
               if (_afp[k].size() > 0) {
                  original.copyState(dest);
                  auto c = layers[i+1][k];
                  _mddspec.updateState(first,dest,c->getState(),i,x[i],_afp[k]);
                  if (original != dest) // && i > 0)
                     n->markDirty();
                  first = false;
               }
            }
         }
      }
      //std::cout << "\n";
   }
}


void MDDRelax::propagate()
{
   try {      
      bool change = false;
      int iter = 0;
      do {
         iter++;
         MDD::propagate();
         computeUp();
         change = rebuild();
         trimDomains();
      } while (change);
      assert(layers[numVariables].size() == 1);
      _mddspec.reachedFixpoint(sink->getState());
      // adjust first and last free inward.
      while (_ff < (int)numVariables && x[_ff]->isBound())
         _ff+=1;
      // while (_lf >= 0 && x[_lf]->isBound())
      //    _lf-=1;
   } catch(Status s) {
      queue.clear();
      throw s;
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
