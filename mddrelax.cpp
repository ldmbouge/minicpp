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
{}


const MDDState& MDDRelax::pickReference(int layer,int layerSize)
{
   // using namespace std;
   // int k = 0;
   // cout << "LAYER:" << layer << endl;
   // for(auto& n : layers[layer]) {
   //    cout << '\t' << k++ << ':' << n->getState() << endl;      
   // }
   double v = _sampler(_rnG);
   double w = 1.0 / (double)layerSize;
   int c = (int)std::floor(v / w);
   int dirIdx = c;
   //std::cout << "DBG:PICKREF(" << layer << ',' << layerSize << ") :" << dirIdx << '\n';
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
      relaxLayer(i+1);
   }
   trimDomains();
   auto dur = RuntimeMonitor::elapsedSince(start);
   std::cout << "build/Relax:" << dur << '\n';
   propagate();

   // for(auto i = 0u;i < numVariables;i++) {
   //    std::cout << "layer[" << i << "] = " << layers[i].size() << '\n';
   //    for(auto j=0u;j < layers[i].size();j++) {
   //       std::cout << '\t' << layers[i][j]->getState() << '\n';
   //    }
   // }
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

void MDDRelax::relaxLayer(int i)
{
   _refs.emplace_back(pickReference(i,(int)layers[i].size()).clone(mem));   
   if (layers[i].size() <= _width)
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

   const int bucketSize = iSize / _width;
   int   rem = iSize % _width;
   int   lim = bucketSize + ((rem > 0) ? 1 : 0);
   rem = rem > 0 ? rem - 1 : 0;
   int   from = 0;
   char* buf = (char*)alloca(sizeof(char)*_mddspec.layoutSize());
   
   std::multimap<float,MDDNode*,std::less<float> > cli;
   std::vector<MDDNode*> nl;
   for(k=0;k < _width;k++) { // k is the bucket id
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

   // int r=0;
   // for(auto n : layers[i])
   //    std::cout << "ENDRELAX " << r++ << " : " << n->getState() << '\n';
}


void MDDRelax::trimLayer(unsigned int layer)
{
   MDD::trimLayer(layer);
}

bool MDDRelax::refreshNode(MDDNode* n,int l)
{
   MDDState cs(&_mddspec,(char*)alloca(_mddspec.layoutSize()));
   MDDState ms(&_mddspec,(char*)alloca(_mddspec.layoutSize()));
   bool first = true;
   assert(n->getNumParents() > 0);
   for(auto& a : n->getParents()) { // a is the arc p --(v)--> n
      auto p = a->getParent();      // p is the parent
      auto v = a->getValue();
      cs.copyState(n->getState());
      _mddspec.createState(cs,p->getState(),l-1,x[l-1],v,true);
      if (first)
         ms.copyState(cs);
      else
         _mddspec.relaxation(ms,cs);
      first = false;
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
      // std::cout << "refresh[" << l << "]@" << n->getPosition() << " : "
      //           << n->getState() << " to " << ms << '\n';
   } else n->clearDirty();
   //return changed;
   return false;
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

MDDNodeSet MDDRelax::filter(TVec<MDDNode*>& layer,int l)
{
   // variable x[l-1] connects layer `l-1` to layer `l`
   MDDNodeSet pool(_width+1);
   for(auto i = layer.rbegin();i != layer.rend();i++) {
      auto n = *i; // This is a _destination_ node into layer `l`
      if (n->getNumParents()==0 && l > 0) {
         //assert(l != numVariables); // should never be recycling the sink
         if (l == numVariables)
            failNow();
         pool.insert(n);
         delState(n,l);
         continue;
      }
      if (n->isDirty())
         refreshNode(n,l);
      else {
         for(auto i = n->getChildren().rbegin(); i != n->getChildren().rend();i++) {
            auto arc = *i;
            MDDNode* child = arc->getChild();
            int v = arc->getValue();
            if (!_mddspec.exist(n->getState(),child->getState(),x[l],v,true)) {
               n->unhook(arc);
               child->markDirty();
               delSupport(l,v);
            }
         }         
      }      
   }
   return pool;
}

MDDNodeSet MDDRelax::split(MDDNodeSet& recycled,TVec<MDDNode*>& layer,int l) // this can use node from recycled or add node to recycle
{
   using namespace std;
   std::multimap<float,MDDNode*,std::less<float> > cl;
   const MDDState& refDir = _refs[l];
   MDDNodeSet delta(_width+1);
   if (l==0 || l==numVariables) return delta;
   bool xb = x[l-1]->isBound();
   MDDState ms(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSize()));
   for(auto i = layer.rbegin();i != layer.rend() && !xb && layer.size() < _width;i++) {
      if (cl.size()==0) {
         int nbR=0;
         for(auto& n : layer) nbR += n->getState().isRelaxed();
         if (nbR==0) break;
         for(auto& n : layer) cl.insert({n->getState().inner(refDir),n});        
      }     
      auto n = *i;
      assert(n->getNumParents() > 0);
      if (!n->getState().isRelaxed()) continue;
      //ms.copyState(n->getState());
      for(auto& a : n->getParents()) { // a is the arc p --(v)--> n
         auto p = a->getParent();      // p is the parent
         auto v = a->getValue();
         _mddspec.createState(ms,p->getState(),l-1,x[l-1],v,true);
         MDDNode* bj = findSimilar(cl,ms,refDir);
         if (bj->getState() == ms) { // there is a perfect match
            if (bj != n) {
               a->moveTo(bj,trail,mem);
               delta.insert(bj);
            }
            // If we matched to n nothing to do. We already point to n.
         } else { // There is an approximate match
            // So, if there is room create a new node
            if (layer.size()  < _width) {
               MDDNode* nc = new (mem) MDDNode(_lastNid++,mem,trail,ms.clone(mem),x[l-1]->size(),l,(int)layer.size());
               layer.push_back(nc,mem);
               a->moveTo(nc,trail,mem);
               delta.insert(nc);
               cl.insert({nc->getState().inner(refDir), nc});
            } 
         }
      }
      if (n->getNumParents()==0) {
         delState(n,l);
         removeMatch(cl,n->getState().inner(refDir),n);
         recycled.insert(n);
      } else {
         refreshNode(n,l);
      }      
   }
   return delta;
}

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
      MDDNode* child = arc->getChild();
      int v = arc->getValue();
      to->unhook(arc);
      child->markDirty();
      delSupport(l,v);
   }
   return to;
}

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

void MDDRelax::spawn(MDDNodeSet& delta,TVec<MDDNode*>& layer,unsigned int l)
{
   using namespace std;
   MDDNodeSet out(_width+1);
   if (l <= numVariables) {
      const MDDState& refDir = _refs[l];
      std::multimap<float,MDDNode*,std::less<float> > cl;
      MDDNodeSet recycled(_width+1); // we cannot filter here since the layer may be disconnected temporarily.
      for(auto n : layer)
         cl.insert({n->getState().inner(refDir),n});
         
      char* buf = (char*)alloca(sizeof(char)*_mddspec.layoutSize());
      for(auto n : delta) {
         if (!n->isActive()) continue;
         assert(n->isActive());
         const MDDState& state = n->getState();
         MDDState psi(&_mddspec,buf);
         for(int v = x[l-1]->min(); v <= x[l-1]->max();v++) {         
            if (!x[l-1]->contains(v)) continue;
            if (!_mddspec.exist(state,sink->getState(),x[l-1],v,false)) continue;
            _mddspec.createState(psi,state,l-1,x[l-1],v,false);
            if (l == numVariables) {
               addSupport(l-1,v);
               n->addArc(mem,sink,v);
               MDDState ssCopy(&_mddspec,(char*)alloca(sizeof(char)*_mddspec.layoutSize()));
               ssCopy.copyState(sink->getState());
               _mddspec.relaxation(ssCopy,psi);
               sink->setState(ssCopy,mem);
               continue;
            }
            MDDNode* child = findMatch(cl,psi,refDir);
            if (child) {
               addSupport(l-1,v);
               n->addArc(mem,child,v);
            } else { // Never seen psi before.
               if (layer.size() < _width) { // there is room in this layer.
                  if (recycled.size() > 0) {
                     child = recycled.pop(); 
                     child->setState(psi.clone(mem), mem);
                     addNodeToLayer(l,child,v);
                  } else {
                     child = new (mem) MDDNode(_lastNid++,mem,trail,psi.clone(mem),x[l-1]->size(),l,(int)layer.size());
                     layer.push_back(child,mem);
                     addSupport(l-1,v);
                  }
                  n->addArc(mem,child,v);
                  out.insert(child);
                  cl.insert({psi.inner(refDir),child});
               } else {
                  MDDNode* psiSim = findSimilar(cl,psi,refDir);
                  const MDDState& psiSimState = psiSim->getState();
                  if (psiSim->getNumParents() != 0)
                     _mddspec.relaxation(psi,psiSimState);
                  if (psi == psiSimState) {
                     addSupport(l-1,v);
                     n->addArc(mem,psiSim,v);
                  } else {
                     auto nh = cl.extract(psiSimState.inner(refDir));
                     out.insert(resetState(n,psiSim,psi,v,l));
                     if (!nh.empty()) {
                        nh.key() = psiSimState.inner(refDir);
                        cl.insert(std::move(nh));
                     } else 
                        cl.insert({psiSimState.inner(refDir),psiSim});
                  }
               }
            }
         }
         if (n->getNumChildren() == 0) 
            delState(n,l-1); // delete is in the layer above. cl contain nodes from layer *l*      
      }
   }
   delta = out;
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
   MDDNodeSet delta(2 * _width);
   bool changed = false;
   for(unsigned l = _ff; l <= numVariables;l++) {
      // First refresh the down information in the nodes of layer l based on whether those are dirty.
      MDDNodeSet recycled = filter(layers[l],l);
      MDDNodeSet splitNodes = split(recycled,layers[l],l);
      delta.unionWith(splitNodes);      
      spawn(delta,layers[l+1],l+1);    
      bool trim = (l>0) ? trimVariable(l-1) : false;
      changed |= trim;
   }
   return changed;
}

void MDDRelax::trimDomains()
{
   for(auto i = 1; i < numVariables;i++) {
      const auto& layer = layers[i];      
      for(int j = (int)layer.size() - 1;j >= 0;j--) {
         if(layer[j]->disconnected())
            removeNode(layer[j]);
      }
   }   
}

void MDDRelax::computeUp()
{
   if (_mddspec.usesUp()) {
      for(int i = _lf;i >= _ff;i--) {
         for(auto& n : layers[i]) {
            bool first = true;
            MDDState temp(n->getState());  // This is a direct reference to the internals of n->getState()
            for(auto& arcToKid : n->getChildren()) {
               MDDNode* kid = arcToKid->getChild();
               _mddspec.updateState(first,temp,kid->getState(),i,x[i],arcToKid->getValue());
               first = false;
            }         
         }
      }
   }
}


void MDDRelax::propagate()
{
   try {      
      bool change = false;
      do {
         MDD::propagate();
         computeUp();
         change = rebuild();
         trimDomains();
      } while (change);
      assert(layers[numVariables].size() == 1);
      _mddspec.reachedFixpoint(sink->getState());
      // adjust first and last free inward.
      while (_ff < numVariables && x[_ff]->isBound())
         _ff+=1;
      while (_lf >= 0 && x[_lf]->isBound())
         _lf-=1;
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
