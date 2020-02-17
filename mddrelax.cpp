#include "mddrelax.hpp"
#include "mddnode.hpp"
#include <float.h>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <random>
#include "RuntimeMonitor.hpp"

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

   auto start = RuntimeMonitor::now();

   for(int i = 0; i < numVariables; i++) {
      buildNextLayer(i);
      relaxLayer(i+1);
   }
   trimDomains();
   auto dur = RuntimeMonitor::elapsedSince(start);
   std::cout << "build/Relax:" << dur << std::endl;

   propagate();
   hookupPropagators();
}

void MDDRelax::relaxLayer(int i)
{
   if (layers[i].size() <= _width)
      return;   
   const int iSize = layers[i].size();

   std::vector<std::tuple<float,MDDNode*>> cl(iSize);
   static std::mt19937 rNG(42);
   std::uniform_int_distribution<int> sampler(0,iSize-1);
   int dirIdx = sampler(rNG);
   const MDDState& refDir = layers[i][dirIdx]->getState();
   int k = 0;
   for(auto& n : layers[i])
      cl[k++] = std::make_tuple(n->getState().inner(refDir),n);
   std::sort(cl.begin(),cl.end(),[](const auto& p1,const auto& p2) {
                                    return std::get<0>(p1) < std::get<0>(p2);
                                 });
   const int bucketSize = iSize / _width;
   int   rem = iSize % _width;
   int   lim = bucketSize + ((rem > 0) ? 1 : 0);
   rem = rem > 0 ? rem - 1 : 0;
   int   from = 0;
   char* buf = new char[_mddspec.layoutSize()]; 
   std::vector<MDDNode*> nl(_width,nullptr);
   for(k=0;k < _width;k++) { // k is the bucket id
      MDDState acc(&_mddspec,buf);
      acc.initState(layers[i][from]->getState());
      MDDNode* target = layers[i][from];
      for(from++; from < lim;from++) {
         MDDNode* strip = layers[i][from];
         acc = _mddspec.relaxation(mem,acc,strip->getState());
         for(auto i = strip->getParents().rbegin();i != strip->getParents().rend();i++) {
            auto arc = *i;
            arc->moveTo(target,trail,mem);
         }
      }
      target->setState(acc,mem);
      nl[k] = target;
      lim += bucketSize + ((rem > 0) ? 1 : 0);
      rem = rem > 0 ? rem - 1 : 0;
   }
   layers[i].clear();
   k = 0;
   for(MDDNode* n : nl) {
      layers[i].push_back(n,mem);
      n->setPosition(k++,mem);
   }
   std::cout << "UMAP-RELAX[" << i << "] :" << layers[i].size() << '/' << iSize << std::endl;
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
   assert(n->getNumParents() > 0);
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
      n->setState(ms,mem);
   }
   return changed;
}

MDDNode* MDDRelax::findSimilar(const std::map<float,MDDNode*>& layer,const MDDState& s,const MDDState& refDir)
{
   auto nlt = layer.lower_bound(s.inner(refDir));
   if (nlt == layer.end()) {
      nlt--;
   }
   return nlt->second;
}

std::tuple<MDDNode*,double> MDDRelax::findSimilar(std::vector<MDDNode*>& list,const MDDState& s)
{
   double best = DBL_MAX;
   int bj = -1;
   for(int j = 0;j < list.size(); j++) {
      MDDNode* a = list[j];
      double simAB = _mddspec.similarity(s,a->getState());
      bj   = simAB < best ? j : bj;
      best = simAB < best ? simAB : best;
   }
   return std::make_tuple(list[bj],best);
}


std::set<MDDNode*> MDDRelax::split(TVec<MDDNode*>& layer,int l)
{
   std::set<MDDNode*> delta;
   bool xb = x[l-1]->isBound();
   for(auto i = layer.rbegin();i != layer.rend() && (xb || layer.size() < _width);i++) {
      auto n = *i;
      if (n->getNumParents() == 0) {
        delState(n,l);
        continue;
      }
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
            MDDNode* bj = nullptr;
            double best;
            std::tie(bj,best) = findSimilar(nn,ms);
            if (best == 0) { // there is a perfect match
               if (bj != n) {
                  a->moveTo(bj,trail,mem);
                  delta.insert(bj);
               }
               // If we matched to n nothing to do. We already point to n.
            } else { // There is an approximate match
               // So, if there is room create a new node
               if (layer.size()  < _width) {
                  MDDNode* nc = new (mem) MDDNode(mem,trail,ms,x[l-1]->size(),l,(int)layer.size());
                  layer.push_back(nc,mem);
                  a->moveTo(nc,trail,mem);
                  delta.insert(nc);
                  nn.push_back(nc);
               } else { // No room, and an approximate match.
                  // Leave the node where it is.
                  // The refresh(n) will compute a relaxation for it. 
               }
            }
         }
         if (n->getNumParents()==0)
            delState(n,l);
         else {
            if (refreshNode(n,l))
               delta.insert(n);
         }
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
   to->setState(s,mem);
   addSupport(l-1,v);
   from->addArc(mem,to,v);
   for(auto i = to->getChildren().rbegin();i != to->getChildren().rend();i++) {
      auto arc = *i;
      to->unhook(arc);
      delSupport(l,arc->getValue());
   }
   return to;
}

void MDDRelax::delState(MDDNode* node,int l)
{
   assert(node->isActive(this));
   assert(l == node->getLayer());
   const int at = node->getPosition();
   assert(node == layers[l][at]);
   layers[l].remove(at);
   node->setPosition((int)layers[l].size(),mem);
   layers[l][at]->setPosition(at,mem);
   if (node->getNumParents() > 0) {
      for(auto i = node->getParents().rbegin();i != node->getParents().rend();i++) {
         auto arc = *i;
         MDDNode* parent = arc->getParent();
         parent->unhook(arc);
         delSupport(l-1,arc->getValue());         
      }
   }
   if (node->getNumChildren() > 0) {
      for(auto i = node->getChildren().rbegin();i != node->getChildren().rend();i++) {
         auto arc = *i;
         node->unhook(arc);
         delSupport(l,arc->getValue());
      }
   }
}

void MDDRelax::spawn(std::set<MDDNode*>& delta,TVec<MDDNode*>& layer,int l)
{
   if (delta.size() == 0) return;
   std::set<MDDNode*> out;
   std::unordered_map<MDDState*,MDDNode*,MDDStateHash,MDDStateEqual> umap(2999);
   std::map<float,MDDNode*,std::less<float> > cl;

   static std::mt19937 rNG(42);
   std::uniform_int_distribution<int> sampler(0,layer.size()-1);
   int dirIdx = sampler(rNG);
   const MDDState& refDir = layer[dirIdx]->getState();
   for(auto& n : layer) {
      umap.insert({n->key(),n});
      float key = n->getState().inner(refDir);
      cl[key] = n;
   }
   
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
               umap.insert({child->key(),child});
               cl[psi.inner(refDir)] = child;
            } else {
               MDDNode* psiSim = findSimilar(cl,psi,refDir);
               if (psiSim->getNumParents() == 0) {
                  auto nh = cl.extract(psiSim->getState().inner(refDir));                  
                  out.insert(resetState(n,psiSim,psi,v,l));
                  if (!nh.empty()) {
                     nh.key() = psiSim->getState().inner(refDir);
                     cl.insert(std::move(nh));
                  } else {
                     cl[psiSim->getState().inner(refDir)] = psiSim;
                  }
               } else {
                  MDDState ns = _mddspec.relaxation(mem,psiSim->getState(),psi);
                  if (ns != psiSim->getState()) {
                     auto nh = cl.extract(psiSim->getState().inner(refDir));
                     out.insert(resetState(n,psiSim,ns,v,l));
                     if (!nh.empty()) {
                        nh.key() = psiSim->getState().inner(refDir);
                        cl.insert(std::move(nh));
                     } else {
                        cl[psiSim->getState().inner(refDir)] = psiSim;
                     }
                  } else {
                     addSupport(l-1,v);
                     n->addArc(mem,psiSim,v);
                  }
               }
            }
         }
      }
      if (n->getNumChildren() == 0) {
         cl.erase(n->getState().inner(refDir));
         delState(n,l-1);
      }
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
   try {
      MDD::propagate();
      rebuild();
      trimDomains();
      _lowest = (int)numVariables - 1;
   } catch(Status s) {
      queue.clear();
      throw s;
   }
}
