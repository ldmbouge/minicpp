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
    // _ig(this, _pool)
{}

void Explainer::injectListeners() 
{
    for (auto x : _es->_cps->intVars()) {
        auto l = new (_es->getStore()) ExpListener(this,x);
        _listeners.push_back(l);
    }
}

void Explainer::empty(var<int>::Ptr x, FailExpl e1, int v1, FailExpl e2, int v2)
{
    clearNoGood();
    _failDepth = _es->getDepth();
    switch (e1) {
        case EQL : _nogood.emplace_back( new Literal(x, EQ, v1, _es->getCurrConstraint(), _es->getDepth()) );
                  break;
        case RM : _nogood.emplace_back( new Literal(x, NEQ, v1, _es->getCurrConstraint(), _es->getDepth()) );
                  break;
        case LB : _nogood.emplace_back( new Literal(x, GEQ, v1, _es->getCurrConstraint(), _es->getDepth()) );
                  break;
        case UB : _nogood.emplace_back( new Literal(x, LEQ, v1, _es->getCurrConstraint(), _es->getDepth()) );
                  break;
    }
    switch (e2) {
        case EQL : _nogood.emplace_back( new Literal(x, EQ, v2, _es->getCurrConstraint(), _es->getDepth()) );
                  break;
        case RM : _nogood.emplace_back( new Literal(x, NEQ, v2, _es->getCurrConstraint(), _es->getDepth()) );
                  break;
        case LB : _nogood.emplace_back( new Literal(x, GEQ, v2, _es->getCurrConstraint(), _es->getDepth()) );
                  break;
        case UB : _nogood.emplace_back( new Literal(x, LEQ, v2, _es->getCurrConstraint(), _es->getDepth()) );
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
    for (auto lp : _nogood)
        delete lp;
    _nogood.clear();
}

// void Explainer::addNodeToImpGraph(Literal* l) 
// {
//     _ig.addNode(l);
// }

ExpSolver::ExpSolver() 
  : _cps(new CPSolver), _exp(new Explainer(this)) 
{
    _cps->_sm.dealloc();
    ExpTrailer::Ptr expT = new ExpTrailer(this);
    _exp->setTrailer(expT);
    _cps->_sm = expT;
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