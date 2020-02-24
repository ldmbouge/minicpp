#include "mddrelax.hpp"
#include <float.h>
#include <unordered_map>
#include <map>
#include <algorithm>
#include "RuntimeMonitor.hpp"

void MDDRelax::buildDiagram()
{
   std::cout << "MDDRelax::buildDiagram" << std::endl;
   _mddspec.layout();
   std::cout << _mddspec << std::endl;
   auto rootState = _mddspec.rootState(mem);
   sink = new (mem) MDDNode(_lastNid++,mem, trail, (int) numVariables, 0);
   root = new (mem) MDDNode(_lastNid++,mem, trail, rootState, x[0]->size(),0, 0);
   layers[0].push_back(root,mem);
   layers[numVariables].push_back(sink,mem);

   auto start = RuntimeMonitor::now();
   _refs.emplace_back(rootState);
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
   _refs.emplace_back(pickReference(i,(int)layers[i].size()).clone(mem));   
   if (layers[i].size() <= _width)
      return;   
   const int iSize = (int)layers[i].size();  
   const MDDState& refDir = _refs[i];

   std::vector<std::tuple<float,MDDNode*>> cl(iSize);
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
   char* buf = (char*)alloca(sizeof(char)*_mddspec.layoutSize()); 
   std::vector<MDDNode*> nl(_width,nullptr);
   for(k=0;k < _width;k++) { // k is the bucket id
      MDDState acc(&_mddspec,buf);
      acc.initState(layers[i][from]->getState());
      MDDNode* target = layers[i][from];
      for(from++; from < lim;from++) {
         MDDNode* strip = layers[i][from];
         _mddspec.relaxation(acc,strip->getState());
         for(auto i = strip->getParents().rbegin();i != strip->getParents().rend();i++) {
            auto arc = *i;
            arc->moveTo(target,trail,mem);
         }
      }
      acc.hash();
      acc.relax();
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
      // std::cout << "refresh[" << l << "]@" << n->getPosition() << " : "
      //           << n->getState() << " to " << ms << std::endl;
      n->setState(ms,mem);
   }
   return changed;
}

MDDNode* MDDRelax::findSimilar(const std::multimap<float,MDDNode*>& layer,
                               const MDDState& s,const MDDState& refDir)
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


std::set<MDDNode*,MDDNodePtrOrder> MDDRelax::split(TVec<MDDNode*>& layer,int l)
{
   using namespace std;
   std::multimap<float,MDDNode*,std::less<float> > cl;
   const MDDState& refDir = _refs[l];
   for(auto& n : layer) 
      cl.insert({n->getState().inner(refDir),n});
   
   std::set<MDDNode*,MDDNodePtrOrder> delta;
   bool xb = x[l-1]->isBound();
   // cout << "startSplit(" << l << ")" << (xb ? "T" : "F") << " " << layer.size() << endl;
   for(auto i = layer.rbegin();i != layer.rend() && (xb || layer.size() < _width);i++) {
      auto n = *i;
      // cout << "split[" << l << "]@" << n->getPosition() << " : " << n->getState() << endl;
         
      if (n->getNumParents() == 0) {
        delState(n,l);
        removeMatch(cl,n->getState().inner(refDir),n);
        continue;
      }
      if (xb) {
         if (refreshNode(n,l))
            delta.insert(n);
      } else {
         if (!n->getState().isRelaxed()) continue;
         for(auto& a : n->getParents()) { // a is the arc p --(v)--> n
            auto p = a->getParent();      // p is the parent
            auto v = a->getValue();
            MDDState ms;
            bool ok;
            std::tie(ms,ok) = _mddspec.createState(mem,p->getState(),x[l-1],v);
            assert(ok);
            MDDNode* bj = findSimilar(cl,ms,refDir);
            // cout << "\tsimto:" << ms << " = " << bj->getState() << endl;
            if (bj->getState() == ms) { // there is a perfect match
               if (bj != n) {
                  a->moveTo(bj,trail,mem);
                  delta.insert(bj);
               }
               // If we matched to n nothing to do. We already point to n.
            } else { // There is an approximate match
               // So, if there is room create a new node
               if (layer.size()  < _width) {
                  MDDNode* nc = new (mem) MDDNode(_lastNid++,mem,trail,ms,x[l-1]->size(),l,(int)layer.size());
                  layer.push_back(nc,mem);
                  a->moveTo(nc,trail,mem);
                  delta.insert(nc);
                  cl.insert({nc->getState().inner(refDir), nc});
               } else { // No room, and an approximate match.
                  // a->moveTo(bj,trail,mem);
                  // delta.insert(bj);
                  // cout << "\tNR:" << ms << " ~ " << bj->getState() << endl;
               }
            }
         }
         if (n->getNumParents()==0) {
            delState(n,l);
            removeMatch(cl,n->getState().inner(refDir),n);
         } else {
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
   assert(node->isActive());
   node->deactivate();
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
   assert(node->getNumParents()==0);
   assert(node->getNumChildren()==0);
}

void MDDRelax::spawn(std::set<MDDNode*,MDDNodePtrOrder>& delta,TVec<MDDNode*>& layer,int l)
{
   if (delta.size() == 0) return;
   const MDDState& refDir = _refs[l];
   std::set<MDDNode*,MDDNodePtrOrder> out;
   std::unordered_map<MDDState*,MDDNode*,MDDStateHash,MDDStateEqual> umap(2* _width);
   std::multimap<float,MDDNode*,std::less<float> > cl;

   for(auto& n : layer) {
      umap.insert({n->key(),n});
      float key = n->getState().inner(refDir);
      cl.insert({key,n});
   }
   char* buf = (char*)alloca(sizeof(char)*_mddspec.layoutSize());
   for(auto n : delta) {
      const MDDState& state = n->getState();
      MDDState psi(&_mddspec,buf);
      for(int v = x[l-1]->min(); v <= x[l-1]->max();v++) {
         if (!x[l-1]->contains(v)) continue;
         if (!_mddspec.exist(state,x[l-1],v)) continue;
         if (l == numVariables) {
            addSupport(l-1,v);
            n->addArc(mem,sink,v);
            continue;
         }
         _mddspec.createState(psi,state,x[l-1],v);
         auto found = umap.find(&psi);
         MDDNode* child = nullptr;
         if(found != umap.end()) {
            child = found->second;
            addSupport(l-1,v);
            n->addArc(mem,child,v);
         } else { // Never seen psi before.
            if (layer.size() < _width) { // there is room in this layer.
               child = new (mem) MDDNode(_lastNid++,mem,trail,psi.clone(mem),x[l-1]->size(),l,(int)layer.size());
               layer.push_back(child,mem);
               addSupport(l-1,v);
               n->addArc(mem,child,v);
               auto ir = out.insert(child);
               umap.insert({child->key(),child});
               cl.insert({psi.inner(refDir),child});
            } else {
               MDDNode* psiSim = findSimilar(cl,psi,refDir);
               const MDDState& psiSimState = psiSim->getState();
               MDDState ns;
               if (psiSim->getNumParents() == 0)
                  ns = std::move(psi);
               else
                  ns = _mddspec.relaxation(mem,psiSimState,psi);
               if (ns == psiSimState) {
                  addSupport(l-1,v);
                  n->addArc(mem,psiSim,v);
               } else {
                  auto nh = cl.extract(psiSimState.inner(refDir));
                  auto ir = out.insert(resetState(n,psiSim,ns,v,l));
                  if (!nh.empty()) {
                     nh.key() = psiSimState.inner(refDir);
                     cl.insert(std::move(nh));
                  } else 
                     cl.insert({psiSimState.inner(refDir),psiSim});
               }
            }
         }
      }
      if (n->getNumChildren() == 0) {         
         delState(n,l-1); // delete is in the layer above. cl contain nodes from layer *l*
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
   std::cout << "MDDRelax::rebuild(lowest="
             << _lowest+1 <<  " -> " << numVariables << ")" << std::endl;
   std::set<MDDNode*,MDDNodePtrOrder> delta;
   for(int l = _lowest + 1; l < numVariables;l++) {
      std::set<MDDNode*,MDDNodePtrOrder> splitNodes = split(layers[l],l);
      for(auto n : splitNodes)
         delta.insert(n);
      spawn(delta,layers[l+1],l+1);
   }
}

void MDDRelax::trimDomains()
{
   for(auto i = _lowest + 1; i < numVariables;i++) {
      auto& layer = layers[i];
      for(int j = (int)layer.size() - 1;j >= 0;j--) {
         if(layer[j]->disconnected())
            removeNode(layer[j]);
      }
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

void MDDRelax::debugGraph()
{
   using namespace std;
   for(int l=0;l < numVariables;l++) {
      cout << "L[" << l <<"] = ";
      for(int i=0;i < layers[l].size();i++) {
         cout << "\t" << layers[l][i]->getState() << " : "
              << layers[l][i]->getState().inner(_refs[l]) << endl;
      }
   }
}
