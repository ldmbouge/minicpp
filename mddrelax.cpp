#include "mddrelax.hpp"
#include "mddnode.hpp"
#include <float.h>
#include <unordered_map>
#include <algorithm>

void MDDRelax::buildDiagram() 
{
   std::cout << "MDDRelax::buildDiagram" << std::endl;
   _mddspec.layout();
   std::cout << _mddspec << std::endl;
   auto rootState = _mddspec.rootState(mem);
   sink = new (mem) MDDNode(mem, trail, (int) numVariables, 0);
   root = new (mem) MDDNode(mem, trail, rootState, x[0]->size(),0, 0);
   layers[0].push_back(root,mem);
   layers[numVariables].push_back(sink,mem);

   for(int i = 0; i < numVariables; i++) {
      buildNextLayer(i);   
      relaxLayer(i+1);
   }
   trimDomains();
   propagate();
   hookupPropagators();
}

void MDDRelax::relaxLayer(int i)
{
   if (layers[i].size() <= _width)
      return;
   std::vector<std::tuple<int,int,double>> sims;
   std::vector<bool> merged(layers[i].size(),false);

   for(int j = 0;j < layers[i].size(); j++) {
      for(int k=j+1;k < layers[i].size();k++) {
         MDDNode* a = layers[i][j];
         MDDNode* b = layers[i][k];
         double simAB = _mddspec.similarity(a->getState(),b->getState());
         sims.emplace_back(std::make_tuple(j,k,simAB));
      }
   }
   std::sort(sims.begin(),sims.end(), [](const auto& a,const auto& b) {
                                         return std::get<2>(a) < std::get<2>(b);
                                      });
   int nbNodes = 0;
   int x = 0;
   std::vector<MDDNode*> nl;
   while (x < sims.size() && nbNodes < _width) {
      int j,k;
      double s;
      std::tie(j,k,s) = sims[x++];
      MDDNode* a = layers[i][j];
      MDDNode* b = layers[i][k];
      if (merged[j] || merged[k])
         continue;
      merge(nl,a,b,true);
      merged[j] = merged[k] = true;
      nbNodes++;
   }
   for(int j = 0;j < layers[i].size(); j++) {
      MDDNode* b = layers[i][j];
      if (merged[j])
         continue;
      if (nl.size() < _width) {
         nl.push_back(b);
      } else {
         double best = DBL_MAX;
         MDDNode* sa = nullptr;
         for(int k=0;k < nl.size();k++) {
            MDDNode* a = nl[k];
            double simAB = _mddspec.similarity(a->getState(),b->getState());
            sa   = simAB < best ? a : sa;
            best = simAB < best ? simAB : best;
         }
         assert(sa != nullptr);
         merge(nl,sa,b,false); // not first merge of sa
         merged[j] = true;
      }
   }
   layers[i].clear();
   int k = 0;
   for(MDDNode* n : nl) {
      layers[i].push_back(n,mem);
      n->setPosition(k++,mem);
   }
}

void MDDRelax::merge(std::vector<MDDNode*>& nl,MDDNode* a,MDDNode* b,bool firstMerge)
{
   MDDState ns = _mddspec.relaxation(mem,a->getState(),b->getState());
   // Let's reuse 'a', move everyone pointing to 'b' to now point to 'a'
   // Let's make the state of 'a' and b'  be the new state 'ns'
   // And rebuild a new vector with only the merged nodes. 
   // loop backwards since this modifies b's parent container. 
   auto end = b->getParents().rend();
   for(auto i = b->getParents().rbegin(); i != end;i++) {
      auto arc = *i;
      arc->moveTo(a,trail,mem);
   }
   if (firstMerge)
      nl.push_back(a);
   a->setState(ns);
   b->setState(ns);
}

void MDDRelax::trimLayer(int layer) 
{
   MDD::trimLayer(layer);
   if (_lowest.fresh())
      _lowest = layer;
   else _lowest = std::min(_lowest.value(),layer);
}

bool MDDRelax::refreshNode(MDDNode* n,int l)
{
   MDDState ms;
   bool first = true;
   for(auto& a : n->getParents()) { // a is the arc p --(v)--> n
      auto p = a->getParent();      // p is the parent
      auto v = a->getValue();
      MDDState cs;
      bool ok;
      std::tie(cs,ok) = _mddspec.createState(mem,p->getState(),x[l-1],v);
      if (first)
         ms = std::move(cs);
      else 
         ms = _mddspec.relaxation(mem,ms,cs);			
      first = false;
   }
   bool changed = n->getState() != ms;
   if (changed) {
      for(auto i = n->getChildren().rbegin();i != n->getChildren().rend();i++) {
         n->unhook(*i);
         delSupport(l,(*i)->getValue());
      }
      n->setState(ms);
   }
   return changed;
}

MDDNode* MDDRelax::findSimilar(TVec<MDDNode*>& layer,const MDDState& s)
{
   double best = DBL_MAX;
   int bj = -1;
   for(int j = 0;j < layer.size(); j++) {
      MDDNode* a = layer[j];
      double simAB = _mddspec.similarity(s,a->getState());
      bj   = simAB < best ? j : bj;
      best = simAB < best ? simAB : best;
   }
   return layer[bj];
}

std::set<MDDNode*> MDDRelax::split(TVec<MDDNode*>& layer,int l)
{
   std::set<MDDNode*> delta;
   bool xb = x[l-1]->isBound();
   for(auto i = layer.rbegin();i != layer.rend() && (xb || layer.size() < _width);i++) {
      auto n = *i;
      if (xb) {
         if (refreshNode(n,l))
            delta.insert(n);
      } else {
         if (!n->getState().isRelaxed()) continue;
         std::vector<MDDNode*> nn;
         nn.push_back(n);
         for(auto& a : n->getParents()) { // a is the arc p --(v)--> n
            auto p = a->getParent();      // p is the parent
            auto v = a->getValue();
            MDDState ms;
            bool ok;
            std::tie(ms,ok) = _mddspec.createState(mem,p->getState(),x[l-1],v);
            assert(ok);
            if (layer.size() < _width && ms != n->getState()) {
               MDDNode* nc = new (mem) MDDNode(mem,trail,ms,x[l-1]->size(),l,(int)layer.size());
               layer.push_back(nc,mem);
               a->moveTo(nc,trail,mem);
               delta.insert(nc);
               nn.push_back(nc);
            } else {
               double best =  DBL_MAX;
               MDDNode* bj = nullptr;
               for(auto sn : nn) {
                  double simAB = _mddspec.similarity(ms,sn->getState());
                  bj = simAB < best ? sn : bj;
                  best = simAB < best ? simAB : best;
               }
               if (best == 0 && bj != n) {
                  a->moveTo(bj,trail,mem);
                  delta.insert(bj);
               }
            }
         }
         if (n->getNumParents()==0) 
            removeNode(n);
         else {
            if (refreshNode(n,l))
               delta.insert(n);
         }
         if (layer.size() == _width) 
            break;
      }
   }
   return delta;
}

struct MDDStateHash {
   std::size_t operator()(MDDState* s)  const noexcept { return s->getHash();}
};

struct MDDStateEqual {
   bool operator()(const MDDState* s1,const MDDState* s2) const { return *s1 == *s2;}
};

MDDNode* MDDRelax::resetState(MDDNode* from,MDDNode* to,MDDState& s,int v,int l)
{
   to->setState(s);
   addSupport(l-1,v);
   from->addArc(mem,to,v);
   for(auto i = to->getChildren().rbegin();i != to->getChildren().rend();i++) {
      auto arc = *i;
      to->unhook(arc);
      delSupport(l,arc->getValue());
   }
   return to;
}

void MDDRelax::spawn(std::set<MDDNode*>& delta,TVec<MDDNode*>& layer,int l)
{
   if (delta.size() == 0) return;
   std::set<MDDNode*> out;
   std::unordered_map<MDDState*,MDDNode*,MDDStateHash,MDDStateEqual> umap(2999);
   for(auto& n : layer)
      umap.insert({n->key(),n});
   for(auto n : delta) {
      for(int v = x[l-1]->min(); v <= x[l-1]->max();v++) {
         if (!x[l-1]->contains(v)) continue;
         if (!_mddspec.exist(n->getState(),x[l-1],v)) continue;
         if (l == numVariables) {
            addSupport(l-1,v);
            n->addArc(mem,sink,v);
            continue;
         }
         MDDState psi;
         bool ok;
         std::tie(psi,ok) = _mddspec.createState(mem,n->getState(),x[l-1],v);
         assert(ok);
         auto found = umap.find(&psi);
         MDDNode* child = nullptr;
         if(found != umap.end()) { 
            child = found->second;
            addSupport(l-1,v);
            n->addArc(mem,child,v);
         } else { // Never seen psi before.
            if (layer.size() < _width) { // there is room in this layer.
               child = new (mem) MDDNode(mem,trail,psi,x[l-1]->size(),l,(int)layer.size());
               layer.push_back(child,mem);
               addSupport(l-1,v);
               n->addArc(mem,child,v);
               out.insert(child);
            } else {
               MDDNode* psiSim = findSimilar(layer,psi);
               if (psiSim->getNumParents() == 0) {
                  out.insert(resetState(n,psiSim,psi,v,l));
               } else {
                  MDDState ns = _mddspec.relaxation(mem,psiSim->getState(),psi);
                  if (ns != psiSim->getState()) {
                     out.insert(resetState(n,psiSim,ns,v,l));
                  } else {
                     addSupport(l-1,v);
                     n->addArc(mem,psiSim,v);
                  }
               }
            }
         }
      }
      if (n->getNumChildren() == 0)
         removeNode(n);
   }
   for(int v = x[l-1]->min(); v <= x[l-1]->max();v++) {
      if (x[l-1]->contains(v) && getSupport(l-1,v)==0)
         x[l-1]->remove(v);            
   }
   delta = out;
}

void MDDRelax::rebuild()
{
   //std::cout << "MDDRelax::rebuild(lowest=" << _lowest << ")" << std::endl;
   std::set<MDDNode*> delta;
   for(int l = _lowest + 1; l < numVariables;l++) {
      std::set<MDDNode*> splitNodes = split(layers[l],l);
      for(auto n : splitNodes)
         delta.insert(n);
      spawn(delta,layers[l+1],l+1);
   }
}

void MDDRelax::propagate() 
{
   MDD::propagate();
   rebuild();
   trimDomains();
   _lowest = (int)numVariables - 1;
}

