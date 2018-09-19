#include "constraint.hpp"

void printCstr(Constraint::Ptr c) {
   c->print(std::cout);
   std::cout << std::endl;
}


void EQc::post()
{
   _x->assign(_c);
}

void NEQc::post()
{
   _x->remove(_c);
}

void EQBinBC::post()
{
   if (_x->isBound())
      _y->assign(_x->min() - _c);
   else if (_y->isBound())
      _x->assign(_y->min() + _c);
   else {
      _x->updateBounds(_y->min() + _c,_y->max() + _c);
      _y->updateBounds(_x->min() - _c,_x->max() - _c);
      _x->whenBoundsChange([this] {
                              _y->updateBounds(_x->min() - _c,_x->max() - _c);
                           });
      _y->whenBoundsChange([this] {
                              _x->updateBounds(_y->min() + _c,_y->max() + _c);
                           });
   }
}
void NEQBinBC::print(std::ostream& os) const
{
   os << _x << " != " << _y << " + " << _c << std::endl;
}

void NEQBinBC::post()
{
   if (_x->isBound())
      _y->remove(_x->min() - _c);
   else if (_y->isBound())
      _x->remove(_y->min() + _c);
   else {
      hdl[0] = _x->whenBind([this] {
            _y->remove(_x->min() - _c);
            hdl[0]->detach();
            hdl[1]->detach();
         });
      hdl[1] = _y->whenBind([this] {
            _x->remove(_y->min() + _c);
            hdl[0]->detach();
            hdl[1]->detach();
         });
   }
}

void EQBinDC::post()
{
   if (_x->isBound())
      _y->assign(_x->min() - _c);
   else if (_y->isBound())
      _x->assign(_y->min() + _c);
   else {
      _x->updateBounds(_y->min() + _c,_y->max() + _c);
      _y->updateBounds(_x->min() - _c,_x->max() - _c);
      int lx = _x->min(), ux = _x->max();
      for(int i = lx ; i <= ux; i++)
         if (!_x->contains(i))
            _y->remove(i - _c);
      int ly = _y->min(),uy = _y->max();
      for(int i= ly;i <= uy; i++)
         if (!_y->contains(i))
            _x->remove(i + _c);
      _x->whenBind([this] { _y->assign(_x->min() - _c);});
      _y->whenBind([this] { _x->assign(_y->min() + _c);});
   }
}

Minimize::Minimize(var<int>::Ptr& x)
    : _obj(x),_primal(0x7FFFFFFF)
{
    auto todo = std::function<void(void)>([this]() {
                                             _obj->removeAbove(_primal);
                                         });
    _obj->getSolver()->onFixpoint(todo);
}

void Minimize::print(std::ostream& os) const
{
   os << "minimize(" << *_obj << ", primal = " << _primal << ")";
}

void Minimize::tighten()
{
   assert(_obj->isBound());
   _primal = _obj->max() - 1;
   _obj->getSolver()->fail();
}

void IsEqual::post() 
{
    propagate();
    if (isActive()) {
        _x->propagateOnDomainChange(this);
        _b->propagateOnBind(this);
    }
}

void IsEqual::propagate() 
{
    if (_b->isTrue()) {
        _x->assign(_c);
        setActive(false);
    } else if (_b->isFalse()) {
        _x->remove(_c);
        setActive(false);
    } else if (!_x->contains(_c)) {
        _b->assign(false);
        setActive(false);
    } else if (_x->isBound()) {
        _b->assign(true);
        setActive(false);
    }
}

Sum::Sum(const Factory::Vecv& x,var<int>::Ptr s)
    : Constraint(s->getSolver()),
      _x(x.size() + 1),
      _nUnBounds(s->getSolver()->getStateManager(),(int)x.size()+1),
      _sumBounds(s->getSolver()->getStateManager(),0),
      _n((int)x.size() + 1),
      _unBounds(_n)
{
    for(int i=0;i < x.size();i++)
        _x[i] = x[i];
    _x[_n-1] = Factory::minus(s);
    for(int i=0; i < _n;i++)
        _unBounds[i] = i;
}        

Sum::Sum(const std::vector<var<int>::Ptr>& x,var<int>::Ptr s)
   : Constraint(s->getSolver()),
     _x(x.size()+1),
      _nUnBounds(s->getSolver()->getStateManager(),(int)x.size()+1),
      _sumBounds(s->getSolver()->getStateManager(),0),
      _n((int)x.size() + 1),
      _unBounds(_n)
{
    for(int i=0;i < x.size();i++)
        _x[i] = x[i];
    _x[_n-1] = Factory::minus(s);
    for(int i=0; i < _n;i++)
        _unBounds[i] = i;
}        
     
void Sum::post()
{
   for(auto& var : _x)
      var->propagateOnBoundChange(this);
   propagate();
}

void Sum::propagate()
{  
   int nU = _nUnBounds;
   int sumMin = _sumBounds,sumMax = _sumBounds;
   for(int i = nU - 1; i >= 0;i--) {
      int idx = _unBounds[i];
      sumMin += _x[idx]->min();
      sumMax += _x[idx]->max();
      if (_x[idx]->isBound()) {
         _sumBounds = _sumBounds + _x[idx]->min();
         _unBounds[i] = _unBounds[nU - 1];
         _unBounds[nU - 1] = idx;
         nU--;
      }
   }
   _nUnBounds = nU;
   if (0 < sumMin ||  sumMax < 0)
      throw Failure;
   for(int i = nU - 1; i >= 0;i--) {
      int idx = _unBounds[i];
      _x[idx]->removeAbove(-(sumMin - _x[idx]->min()));
      _x[idx]->removeBelow(-(sumMax - _x[idx]->max()));
   }
}

AllDifferentBinary::AllDifferentBinary(const Factory::Vecv& x)
    : Constraint(x[0]->getSolver()),
      _x(x.size())
{
   for(int i=0;i < x.size();i++)
      _x[i] = x[i];
}
AllDifferentBinary::AllDifferentBinary(const std::vector<var<int>::Ptr>& x)
   : Constraint(x[0]->getSolver()),
     _x(x)
{}

void AllDifferentBinary::post()
{
    CPSolver::Ptr cp = _x[0]->getSolver();
    const long n = _x.size();
    for(int i=0;i < n;i++) 
        for(int j=i+1;j < n;j++)
            cp->post(Factory::notEqual(_x[i],_x[j]));    
}

Circuit::Circuit(const Factory::Vecv& x)
   : Constraint(x[0]->getSolver()),
     _x(x.size())
{
    auto cp = x[0]->getSolver();
    for(int i=0;i < x.size();i++)
       _x[i] = x[i];
    _dest = new (cp) trail<int>[_x.size()];
    _orig = new (cp) trail<int>[_x.size()];
    _lengthToDest = new (cp) trail<int>[_x.size()];
    for(int i=0;i<_x.size();i++) {
        new (_dest+i) trail<int>(cp->getStateManager(),i);
        new (_orig+i) trail<int>(cp->getStateManager(),i);
        new (_lengthToDest+i) trail<int>(cp->getStateManager(),0);
    }
}

Circuit::Circuit(const std::vector<var<int>::Ptr>& x)
   : Constraint(x[0]->getSolver()),
     _x(x)
{
    auto cp = x[0]->getSolver();
    //    for(int i=0;i < x.size();i++)
    //        _x[i] = x[i];
    _dest = new (cp) trail<int>[_x.size()];
    _orig = new (cp) trail<int>[_x.size()];
    _lengthToDest = new (cp) trail<int>[_x.size()];
    for(int i=0;i<_x.size();i++) {
        new (_dest+i) trail<int>(cp->getStateManager(),i);
        new (_orig+i) trail<int>(cp->getStateManager(),i);
        new (_lengthToDest+i) trail<int>(cp->getStateManager(),0);
    }
}

void Circuit::post()
{
   auto cp = _x[0]->getSolver();
   cp->post(Factory::allDifferent(_x));
   if (_x.size() == 1) {
      _x[0]->assign(0);
      return ;      
   }
   for(int i=0;i < _x.size();i++)
      _x[i]->remove(i);
   for(int i=0;i < _x.size();i++) {
      if (_x[i]->isBound())
         bind(i);
      else 
         _x[i]->whenBind([i,this]() { bind(i);});      
   }
}

void Circuit::bind(int i)
{
   int j = _x[i]->min();
   int origi = _orig[i];
   int destj = _dest[j];
   _dest[origi] = destj;
   _orig[destj] = origi;
   int length = _lengthToDest[origi] + _lengthToDest[j] + 1;
   _lengthToDest[origi] = length;
   if (length < _x.size() - 1)
      _x[destj]->remove(origi);                     
}

Element2D::Element2D(const matrix<int,2>& mat,var<int>::Ptr x,var<int>::Ptr y,var<int>::Ptr z)
    : Constraint(x->getSolver()),
      _matrix(mat),
      _x(x),_y(y),_z(z),
      _n(mat.size(0)),
      _m(mat.size(1)),
      _low(x->getSolver()->getStateManager(),0),
      _up(x->getSolver()->getStateManager(),_n * _m - 1)
{
    //_xyz.resize(_up + 1);
    for(int i=0;i < _matrix.size(0);i++)
        for(int j=0;j < _matrix.size(1);j++)
            _xyz.push_back(Triplet(i,j,_matrix[i][j]));
    std::sort(_xyz.begin(),_xyz.end(),[](const Triplet& a,const Triplet& b) {
                                          return a.z < b.z;
                                      });
    _nColsSup = new (x->getSolver()) trail<int>[_n];
    _nRowsSup = new (x->getSolver()) trail<int>[_m];
    auto sm = x->getSolver()->getStateManager();
    for(int i=0;i<_n;i++)
        new (_nColsSup + i) trail<int>(sm,_m);
    for(int j=0;j <_m;j++)
       new (_nRowsSup + j) trail<int>(sm,_n);
}

void Element2D::updateSupport(int lostPos)
{
   int nv1 = _nColsSup[_xyz[lostPos].x] = _nColsSup[_xyz[lostPos].x] - 1;
   if (nv1 == 0)
      _x->remove(_xyz[lostPos].x);  
   int nv2 = _nRowsSup[_xyz[lostPos].y] = _nRowsSup[_xyz[lostPos].y] - 1;
   if (nv2==0)
      _y->remove(_xyz[lostPos].y);
}


void Element2D::post() 
{
   _x->updateBounds(0,_n-1);
   _y->updateBounds(0,_m-1);
   _x->propagateOnDomainChange(this);
   _y->propagateOnDomainChange(this);
   _z->propagateOnBoundChange(this);
   propagate();
}

void Element2D::propagate() 
{
   int l =  _low,u = _up;
   int zMin = _z->min(),zMax = _z->max();
   while (_xyz[l].z < zMin || !_x->contains(_xyz[l].x) || !_y->contains(_xyz[l].y)) {
      updateSupport(l++);
      if (l > u) throw Failure;      
   }
   while (_xyz[u].z > zMax || !_x->contains(_xyz[u].x) || !_y->contains(_xyz[u].y)) {
      updateSupport(u--);
      if (l > u) throw Failure;
   }
   _z->updateBounds(_xyz[l].z,_xyz[u].z);
   _low = l;
   _up  = u;
}

void Element2D::print(std::ostream& os) const
{
   os << "element2D(" << _x << ',' << _y << ',' << _z << ')' << std::endl;
}

Element1D::Element1D(const std::vector<int>& array,var<int>::Ptr y,var<int>::Ptr z)
   : Constraint(y->getSolver()),_t(array),_y(y),_z(z)
{}

void Element1D::post()
{
   matrix<int,2> t2({1,(int)_t.size()});
   for(int j=0;j< _t.size();j++)
      t2[0][j] = _t[j];
   auto x = Factory::makeIntVar(_y->getSolver(),0,0);
   auto c = new (_y->getSolver()) Element2D(t2,x,_y,_z);
   _y->getSolver()->post(c,false);
}

