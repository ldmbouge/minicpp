#ifndef __EXPTRAIL_H
#define __EXPTRAIL_H

#include "trail.hpp"
#include "hashtable.hpp"
#include "literal.hpp"
#include <unordered_map>


// class Literal;
class ExpSolver;
class Explainer;
// typedef TrailHashtable<unsigned int, Literal*, std::hash<unsigned int>, std::equal_to<unsigned int>> LitTrailHashTable;

class LitEntry {
    size_t _tsz;
    Literal* _lp;
public:
    LitEntry(size_t tsz, Literal* lp) : _tsz(tsz), _lp(lp) {}
    size_t trailSize() { return _tsz;}
    Literal* getLit() { return _lp;}
};

class ExpTrailer : public Trailer {
    std::vector<LitEntry>    _lits;
    std::unordered_map<unsigned long, Literal*> _litDatabase;
    ExpSolver* _es;
    handle_ptr<Explainer> _exp;
    void popWithExp();
    void popNoExp();
    void untrailToSize(int);
    void examineNextLit();
public:
    typedef handle_ptr<ExpTrailer> Ptr;
    ExpTrailer(ExpSolver*, handle_ptr<Explainer>);
    virtual ~ExpTrailer();
    virtual void restoreState() override;
    virtual void saveState() override;
    void storeLit(Literal*);
    void removeLit(Literal*);
    void clearLitsTo(size_t);
    Literal* findLit(Literal&);
    void printLitDatabase();
};


#endif