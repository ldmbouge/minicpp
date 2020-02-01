#ifndef __MDDRELAX_H
#define __MDDRELAX_H

#include "mdd.hpp"

class MDDRelax : public MDD {
	const int _width;
	void merge(std::vector<MDDNode*>& nl,MDDNode* a,MDDNode* b,bool firstMerge);
public:
	MDDRelax(CPSolver::Ptr cp,int width = 32) : MDD(cp),_width(width) {}
	void buildDiagram() override;
	void relaxLayer(int i);
	void propagate() override;
};

#endif