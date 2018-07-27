#include "constraint.hpp"

void EQc::post()
{
   _x->bind(_c);
}

void NEQc::post()
{
   _x->remove(_c);
}

void EQBinBC::post()
{
   if (_x->isBound())
      _y->bind(_x->getMin() - _c);
   else if (_y->isBound())
      _x->bind(_y->getMin() + _c);
   else {
      _x->updateBounds(_y->getMin() + _c,_y->getMax() + _c);
      _y->updateBounds(_x->getMin() - _c,_x->getMax() - _c);
      _x->whenChangeBounds([this] {
            _y->updateBounds(_x->getMin() - _c,_x->getMax() - _c);
         });
      _y->whenChangeBounds([this] {
            _x->updateBounds(_y->getMin() + _c,_y->getMax() + _c);
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
      _y->remove(_x->getMin() - _c);
   else if (_y->isBound())
      _x->remove(_y->getMin() + _c);
   else {
      hdl[0] = _x->whenBind([this] {
            _y->remove(_x->getMin() - _c);
            hdl[0]->detach();
            hdl[1]->detach();
         });
      hdl[1] = _y->whenBind([this] {
            _x->remove(_y->getMin() + _c);
            hdl[0]->detach();
            hdl[1]->detach();
         });
   }
}

void EQBinDC::post()
{
   if (_x->isBound())
      _y->bind(_x->getMin() - _c);
   else if (_y->isBound())
      _x->bind(_y->getMin() + _c);
   else {
      _x->updateBounds(_y->getMin() + _c,_y->getMax() + _c);
      _y->updateBounds(_x->getMin() - _c,_x->getMax() - _c);
      int lx = _x->getMin(), ux = _x->getMax();
      for(int i = lx ; i <= ux; i++)
         if (!_x->contains(i))
            _y->remove(i - _c);
      int ly = _y->getMin(),uy = _y->getMax();
      for(int i= ly;i <= uy; i++)
         if (!_y->contains(i))
            _x->remove(i + _c);
      _x->whenBind([this] { _y->bind(_x->getMin() - _c);});
      _y->whenBind([this] { _x->bind(_y->getMin() + _c);});
   }
}

void Minimize::print(std::ostream& os) const
{
   os << "minimize(" << *_obj << ", primal = " << _primal << ")";
}

void Minimize::tighten()
{
   assert(_obj->isBound());
   _primal = _obj->getMax() - 1;
   _obj->getSolver()->schedule(_todo);
}

void Minimize::post() 
{
   _todo = std::bind(&Minimize::propagate,this);
   _obj->getSolver()->onFixpoint(_todo);
   _obj->whenChangeBounds(std::bind(&Minimize::propagate,this));
}

void Minimize::propagate()
{
   _obj->updateMax(_primal);
}
