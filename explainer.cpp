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
    _listeners(0),
    _nogoodFound(false)
    // _pool(new Pool)
{}

int Explainer::getCurrDepth()
{
    return _es->getDepth();
}

void Explainer::injectListeners() 
{
    // inject var listeners
    for (auto x : _es->_cps->intVars()) {
        auto l = new (_es->getStore()) ExpListener(this,x);
        _listeners.push_back(l);
    }
    // inject constraint listeners
    auto cv = ExpVisitor();
    for (auto cPtr : _es->_cps->constraints()) {
        cPtr->visit(cv);
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
        case EQL :  lp = new Literal(x, EQ, v1, _es->getCurrConstraint()->getListener(), _es->getDepth());
                    _nogood.insert( {litKey(*lp), lp} );
                    break;
        case RM :   lp =  new Literal(x, NEQ, v1, _es->getCurrConstraint()->getListener(), _es->getDepth());
                    _nogood.insert( {litKey(*lp), lp} );
                    break;
        case LB :   lp = new Literal(x, GEQ, v1, _es->getCurrConstraint()->getListener(), _es->getDepth());
                    _nogood.insert( {litKey(*lp), lp} );
                    break;
        case UB :   lp = new Literal(x, LEQ, v1, _es->getCurrConstraint()->getListener(), _es->getDepth());
                    _nogood.insert( {litKey(*lp), lp} );
                    break;
    }
    switch (e2) {
        case EQL :  lp = new Literal(x, EQ, v2, _es->getCurrConstraint()->getListener(), _es->getDepth());
                    _nogood.insert( {litKey(*lp), lp} );
                    break;
        case RM :   lp = new Literal(x, NEQ, v2, _es->getCurrConstraint()->getListener(), _es->getDepth());
                    _nogood.insert( {litKey(*lp), lp} );
                    break;
        case LB :   lp = new Literal(x, GEQ, v2, _es->getCurrConstraint()->getListener(), _es->getDepth());
                    _nogood.insert( {litKey(*lp), lp} );
                    break;
        case UB :   lp = new Literal(x, LEQ, v2, _es->getCurrConstraint()->getListener(), _es->getDepth());
                    _nogood.insert( {litKey(*lp), lp} );
                    break;
    }
}

void Explainer::bind(var<int>::Ptr x, int a) 
{
    // if (x->getId() == 7 && a == 0)
    //     std::cout << "bound x7 to 0\n";
    Literal* lp = new Literal(x, EQ, a, _es->getCurrConstraint()->getListener(), _es->getDepth());
    _expT->storeLit(lp);
}

void Explainer::remove(var<int>::Ptr x, int a) 
{
    // if (x->getId() == 13 && a == 1)
    //     std::cout << "remove 1 from x13\n";
    Literal* lp = new Literal(x, NEQ, a, _es->getCurrConstraint()->getListener(), _es->getDepth());
    _expT->storeLit(lp);
}

void Explainer::changeMin(var<int>::Ptr x, int newMin) 
{
    Literal* lp = new Literal(x, GEQ, newMin, _es->getCurrConstraint()->getListener(), _es->getDepth());
    _expT->storeLit(lp);
}

void Explainer::changeMax(var<int>::Ptr x, int newMax) 
{
    Literal* lp = new Literal(x, LEQ, newMax, _es->getCurrConstraint()->getListener(), _es->getDepth());
    _expT->storeLit(lp);
}

void Explainer::setNoGood(std::vector<Literal*> vl)
{
    for (auto lp : vl)
        _nogood.insert( {litKey(*lp), lp} );
}

void Explainer::clearNoGood()
{
    Literal* lp;
    for (auto it : _nogood) {
        lp = it.second;
        if (!_expT->findLit(*lp))  // if lit pointer no longer in database, delete it -- could happen if lp explanation returned lp itself
            delete lp;
    }
    _nogood.clear();
}

void Explainer::printNoGood()
{
    for (auto it = _nogood.begin(); it != _nogood.end(); ++it)
        std::cout << *(it->second) << " /\\ ";
    std::cout << "\b\b \b\b \b\b \n";
}

void Explainer::addCut()
{
    std::vector<LitVar::Ptr> vl;
    int i;
    int d = -1;
    for (auto it = _nogood.begin(); it != _nogood.end(); ++it) {
        vl.emplace_back( Factory::makeLitVar(litNegation(*(it->second))) );  // heap allocated
        i = it->second->getDepth();
        if ( (i > d) && (i < _failDepth) )  // find largest depth that is less than the fail depth
            d = i;
    }
    Constraint::Ptr c = Factory::learnedClause(vl);  // heap allocated
    _nogoodFound = true;
    _backjumpTo = d;
    _cutQueue.addToQueue(c, d);
}

void Explainer::checkCuts()
{
    std::vector<GlobalCut>& cuts = _cutQueue.getCuts();
    auto it = cuts.begin();
    int d = getCurrDepth();  
    while ( (it != cuts.end()) ) {
        if ( it->getDepth() > d ) {
            it->getCon().dealloc();  // also frees the LitVars in constraint
            it->setCon(nullptr);
            cuts.erase(it);
        } else
            ++it;
    }
    it = cuts.begin();
    while( it != cuts.end() ) {    
        if (it->getDepth() == d) {
            std::cout << "posting cut\n";
            _es->post( it->getCon() );
        }
        ++it;
    }
}

void Explainer::purgeCuts()
{
    std::vector<GlobalCut>& cuts = _cutQueue.getCuts();
    auto it = cuts.begin();
    int d = getCurrDepth();  
    while ( (it != cuts.end()) ) {
        if ( it->getDepth() > d ) {
            std::cout << "purging cut\n";
            it->getCon().dealloc();  // also frees the LitVars in constraint
            it->setCon(nullptr);
            cuts.erase(it);
        } else
            ++it;  // need else here b/c of erase usage above
    }
}

void Explainer::postCuts()
{
    _expT->incMagic();
    std::vector<GlobalCut>& cuts = _cutQueue.getCuts();
    auto it = cuts.begin();
    int d = getCurrDepth();  
    while( it != cuts.end() ) {    
        if (it->getDepth() == d) {
            std::cout << "posting cut\n";
            _es->post( it->getCon() );
        }
        ++it;
    }
}

void Explainer::checkLit(Literal* currLit) 
{
    bool litReplacement = false;
    std::cout << "\tcurrent nogood\n\t\t";
    printNoGood();
    for (auto it = _nogood.begin(); it != _nogood.end(); ++it) {
        if ((it->second) == currLit) {
            std::cout << "replacing " << *(it->second);
            litReplacement = true;
            _nogood.erase(it);
            break;
        }
    }
    bool delCurrLit = true;
    if (litReplacement) {
        std::vector<Literal*> r = currLit->explain();
        std::cout << " with ";
        for (auto litPtr : r) {
            std::cout << *litPtr << " /\\ ";
            _nogood.insert( {litKey(*litPtr), litPtr} );
            if (litPtr == currLit)  // this could happen if explanation is not implemented, returning an explanation that is just the literal itself
                delCurrLit = false;  
        }
        std::cout << "\b\b\b\b\n";
    }
    if (delCurrLit) 
        delete currLit;
    // check if at 1 UIP
    int nbLitsAtFailDepth = 0;
    for (auto it = _nogood.begin(); it != _nogood.end(); ++it) {
        if (it->second->getDepth() == _failDepth)
            ++nbLitsAtFailDepth;
    }
    if (nbLitsAtFailDepth == 1) {
        std::cout << "found nogood:\n\t";
        printNoGood();
        addCut();
        clearNoGood();  // TODO: add flag to stop checking lits at current node if we reach this point
    }
}

Literal* Explainer::findLit(Literal& l)
{
    // std::cout << "looking for " << l << '\n';
    // _expT->printLitDatabase();
    // std::cout << "******************************\n";
    return _expT->findLit(l);
}

bool Explainer::backjumping()
{
    bool rv = false;
    if (_nogoodFound) {
       rv = true;
       _nogoodFound = false; 
    }
    return rv;
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
    // std::cout << *_x << "bound to " << v << "\n";
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
    // std::cout << *_x << " loses value " << v << "\n";
    _exp->remove(_x, v);
}

// AllDiffExplainer::AllDiffExplainer(ExpSolver* es, AllDifferentAC* c)
//   : _es(es), _c(c) 
// {
//     if (es)
//         _exp = es->getExplainer();
//     else
//         _exp = nullptr;
// }

// void AllDiffExplainer::explain(var<int>::Ptr x, int val) {
//     std::cout << "explaining " << x << " != " << val << std::endl;
//     // MaximumMatching& mm = _c->_mm;
//     Graph& rg = _c->_rg;
//     std::string s = "";
//     _c->updateRange();
//     _c->updateGraph();
//     int nc = 0;
//     int scc[_c->_nNodes];
//     rg.SCC([&scc,&nc](int n,int nd[]) {
//                for(int i=0;i < n;++i)
//                    scc[nd[i]] = nc;
//                ++nc;
//            });
//     int sccOfVal = scc[_c->valNode(val)];
//     for (int i = 0; i < _c->_nVar; ++i) {
//         if ((scc[i] == sccOfVal) || (val == _c->_match[i])) {
//             for (int v = x->min(); v < x->max() + 1; ++v) {
//                 if (x->contains(v)) {
//                     if (v != val)
//                         s += "| var_" + std::to_string(i) + " != " + std::to_string(v) + " | ";
//                 }
//             }
//         }
//     }
//     std::cout << s << std::endl;
// }

// std::vector<Literal*> EQcExplainer::explain(Literal* lp)
// {
//     // come up with the reason literal [rl] for the literal in the argument
//     // create a literal object for rl
//     // find the "real" reason literal using findLit(rl)
//     // return the "real" literal pointer as the reason
//     return std::vector<Literal*>(0);
// }
