#include "constraint.hpp"

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
      _nUnBounds(s->getSolver()->getStateManager(),x.size()+1),
      _sumBounds(s->getSolver()->getStateManager(),0),
      _n(x.size() + 1),
      _unBounds(_n)
{
    for(int i=0;i < x.size();i++)
        _x[i] = x[i];
    _x[_n-1] = s;
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
   std::cout << "-->sum::: {" ;
   for(int i=0;i < _x.size();i++) {
      std::cout << "x[" << i << "]=" << _x[i] << ",";
   }
   std::cout << "}" << std::endl;
   
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
   
   std::cout << "<--sum::: {" ;
   for(int i=0;i < _x.size();i++) {
      std::cout << "x[" << i << "]=" << _x[i] << ",";
   }
   std::cout << "}" << std::endl;

}
