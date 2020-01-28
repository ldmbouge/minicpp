#include "mddrelax.hpp"
#include "mddnode.hpp"
#include <float.h>

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
	std::vector<std::tuple<int,int,double>> sims;

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
		if (a->isMerged() || b->isMerged()) 
			continue;
		merge(nl,a,b);
		nbNodes++;
	}
	for(int j = 0;j < layers[i].size(); j++) {
		MDDNode* b = layers[i][j];
		if (b->isMerged()) 
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
            merge(nl,sa,b);
        }
	}
	layers[i].clear();
	int k = 0;
	for(MDDNode* n : nl) {
		layers[i].push_back(n,mem);
		n->setPosition(k++,mem);
	}
}

void MDDRelax::merge(std::vector<MDDNode*>& nl,MDDNode* a,MDDNode* b)
{
	MDDState ns = _mddspec.relaxation(mem,a->getState(),b->getState());
	// Let's reuse 'a', move everyone pointing to 'b' to now point to 'a'
	// Let's make the state of 'a' and b'  be the new state 'ns'
	// And rebuild a new vector with only the merged nodes. 
	for(MDDEdge::Ptr arc : b->getParents()) 
		arc->moveTo(a,mem);
	a->setState(ns);
	b->setState(ns);
	a->merge();
	b->merge();
	nl.push_back(a);
}
