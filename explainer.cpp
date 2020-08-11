#include "explainer.hpp"
#include "constraint.hpp"
#include "search.hpp"
#include "literal.hpp"
#include <string>
#include <iostream>

int ExpSolver::getDepth() 
{ 
    return _ss->getDepth();
}

Explainer::Explainer(ExpSolver* es) 
  : _es(es), 
    _listeners(0)
    // _pool(new Pool)
{}

void Explainer::injectListeners() 
{
    for (auto x : _es->_cps->intVars()) {
        auto l = new (_es->getStore()) ExpListener(this,x);
        _listeners.push_back(l);
    }
}

void Explainer::setFailDepth(int d)
{
    _failDepth = d;
}

void Explainer::empty(var<int>::Ptr x, FailExpl e1, int v1, FailExpl e2, int v2)
{
    clearNoGood();
    _failDepth = _es->getDepth();
    Literal* lp;
    switch (e1) {
        case EQL :  lp = new Literal(x, EQ, v1, _es->getCurrConstraint(), _es->getDepth());
                    _nogood.insert( {litKey(*lp), lp} );
                    break;
        case RM :   lp =  new Literal(x, NEQ, v1, _es->getCurrConstraint(), _es->getDepth());
                    _nogood.insert( {litKey(*lp), lp} );
                    break;
        case LB :   lp = new Literal(x, GEQ, v1, _es->getCurrConstraint(), _es->getDepth());
                    _nogood.insert( {litKey(*lp), lp} );
                    break;
        case UB :   lp = new Literal(x, LEQ, v1, _es->getCurrConstraint(), _es->getDepth());
                    _nogood.insert( {litKey(*lp), lp} );
                    break;
    }
    switch (e2) {
        case EQL :  lp = new Literal(x, EQ, v2, _es->getCurrConstraint(), _es->getDepth());
                    _nogood.insert( {litKey(*lp), lp} );
                    break;
        case RM :   lp = new Literal(x, NEQ, v2, _es->getCurrConstraint(), _es->getDepth());
                    _nogood.insert( {litKey(*lp), lp} );
                    break;
        case LB :   lp = new Literal(x, GEQ, v2, _es->getCurrConstraint(), _es->getDepth());
                    _nogood.insert( {litKey(*lp), lp} );
                    break;
        case UB :   lp = new Literal(x, LEQ, v2, _es->getCurrConstraint(), _es->getDepth());
                    _nogood.insert( {litKey(*lp), lp} );
                    break;
    }
}

void Explainer::bind(var<int>::Ptr x, int a) 
{
    Literal* lp = new Literal(x, EQ, a, _es->getCurrConstraint(), _es->getDepth());
    _expT->storeLit(lp);
}

void Explainer::remove(var<int>::Ptr x, int a) 
{
    Literal* lp = new Literal(x, NEQ, a, _es->getCurrConstraint(), _es->getDepth());
    _expT->storeLit(lp);
}

void Explainer::changeMin(var<int>::Ptr x, int newMin) 
{
    Literal* lp = new Literal(x, GEQ, newMin, _es->getCurrConstraint(), _es->getDepth());
    _expT->storeLit(lp);
}

void Explainer::changeMax(var<int>::Ptr x, int newMax) 
{
    Literal* lp = new Literal(x, LEQ, newMax, _es->getCurrConstraint(), _es->getDepth());
    _expT->storeLit(lp);
}

void Explainer::clearNoGood()
{
    for (auto it : _nogood)
        delete it.second;  // release literal from memory
    _nogood.clear();
}

void Explainer::printNoGood()
{
    // std::cout << "found nogood:\n\t";
    for (auto it = _nogood.begin(); it != _nogood.end(); ++it)
        std::cout << *(it->second) << " /\\ ";
    std::cout << "\b\b \b\b \b\b \n";
}

void Explainer::checkLit(Literal* currLit) 
{
    // create list, N, of lits in nogood that are no longer valid and also remove them from nogood
    // std::vector<Literal*> N;
    bool litReplacement = false;
    // std::cout << "initial nogood: \n\t";
    // printNoGood();
    for (auto it = _nogood.begin(); it != _nogood.end(); ++it) {
        if (!it->second->isValid()) {
            litReplacement = true;
            // N.push_back(it->second);
            delete it->second;  // release lit from memory
            _nogood.erase(it);
        }
    }
    // if N is not empty: release lits in N from memory; add lp->reason() to nogood
    if (litReplacement) {
        // for (auto lp : N)
        //     delete lp;
        std::vector<Literal*> r = currLit->explain();
        for (auto litPtr : r)
            _nogood.insert( {litKey(*litPtr), litPtr} );
    } else delete currLit;
    // simplify nogood
    // check if at 1 UIP
    int nbLitsAtFailDepth = 0;
    for (auto it = _nogood.begin(); it != _nogood.end(); ++it) {
        if (it->second->getDepth() == _failDepth)
            ++nbLitsAtFailDepth;
    }
    if (nbLitsAtFailDepth == 1) {
        // std::cout << "found nogood:\n\t";
        // printNoGood();
        // post nogood
    }
}

ExpSolver::ExpSolver() 
{
   _exp = new Explainer(this);
   ExpTrailer::Ptr trailer = new ExpTrailer(this, _exp);
   _exp->setTrailer(trailer);
   _cps = new CPSolver(trailer);
   _cps->_es = this;
}

ExpSolver::~ExpSolver()
{
    _cps.dealloc();
    _exp.dealloc();
}

ExpListener::ExpListener(Explainer* exp, var<int>::Ptr x)
  : _exp(exp),
    _x(x), 
    _notif(x->getListener())
{
    _x->setListener(this);
}

void ExpListener::empty()
{
    _notif->empty();
}

void ExpListener::empty(FailExpl e1, int v1, FailExpl e2, int v2)
{
    _exp->empty(_x, e1, v2, e2, v2);
    _notif->empty();
}

void ExpListener::change()
{
    _notif->change();
}

void ExpListener::bind(int v)
{
    _exp->bind(_x, v);
    _notif->bind(v);
}

void ExpListener::changeMin(int newMin)
{
    _exp->changeMin(_x, newMin);
    _notif->changeMin(newMin);
}

void ExpListener::changeMax(int newMax)
{
    _exp->changeMax(_x, newMax);
    _notif->changeMax(newMax);
}

void ExpListener::remove(int v)
{
    _exp->remove(_x, v);
}

AllDiffExplainer::AllDiffExplainer(ExpSolver* es, AllDifferentAC* c)
  : _es(es), _c(c) 
{
    if (es)
        _exp = es->getExplainer();
    else
        _exp = nullptr;
}

void AllDiffExplainer::explain(var<int>::Ptr x, int val) {
    std::cout << "explaining " << x << " != " << val << std::endl;
    // MaximumMatching& mm = _c->_mm;
    Graph& rg = _c->_rg;
    std::string s = "";
    _c->updateRange();
    _c->updateGraph();
    int nc = 0;
    int scc[_c->_nNodes];
    rg.SCC([&scc,&nc](int n,int nd[]) {
               for(int i=0;i < n;++i)
                   scc[nd[i]] = nc;
               ++nc;
           });
    int sccOfVal = scc[_c->valNode(val)];
    for (int i = 0; i < _c->_nVar; ++i) {
        if ((scc[i] == sccOfVal) || (val == _c->_match[i])) {
            for (int v = x->min(); v < x->max() + 1; ++v) {
                if (x->contains(v)) {
                    if (v != val)
                        s += "| var_" + std::to_string(i) + " != " + std::to_string(v) + " | ";
                }
            }
        }
    }
    std::cout << s << std::endl;
}

std::vector<Literal*> EQcExplainer::explain(Literal* lp)
{
    // come up with the reason literal [rl] for the literal in the argument
    // create a literal object for rl
    // find the "real" reason literal using findLit(rl)
    // return the "real" literal pointer as the reason
    return std::vector<Literal*>(0);
}
