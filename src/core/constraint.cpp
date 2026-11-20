/*
 * mini-cp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License  v3
 * as published by the Free Software Foundation.
 *
 * mini-cp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.
 * See the GNU Lesser General Public License  for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with mini-cp. If not, see http://www.gnu.org/licenses/lgpl-3.0.en.html
 *
 * Copyright (c)  2018. by Laurent Michel, Pierre Schaus, Pascal Van Hentenryck
 */

#include "constraint.hpp"
#include <string.h>
#include <limits>

void printCstr(Constraint::Ptr c) {
   c->print(std::cout);
   std::cout << std::endl;
}

void printCstrDesc(ConstraintDesc::Ptr c) {
   c->print(std::cout);
   std::cout << std::endl;
}


void EQc::post()
{
   _x->assign(_c);  
}

EQc* EQcDesc::create()
{
   return new (_x->getSolver()) EQc(_x,_c);
}
EQcDesc* EQcDesc::clone()
{
   return new EQcDesc(_x,_c);
}
void EQcDesc::print(std::ostream& os) const
{
   os << _x << " == " << _c;
}

void NEQc::post()
{
   _x->remove(_c);
}

NEQc* NEQcDesc::create()
{
   return new (_x->getSolver()) NEQc(_x,_c);
}
NEQcDesc* NEQcDesc::clone()
{
   return new NEQcDesc(_x,_c);
}
void NEQcDesc::print(std::ostream& os) const
{
   os << _x << " != " << _c;
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

void EQTernBC::post()
{
   // x == y + z
   if (_x->isBound() && _y->isBound())
      _z->assign(_x->min() - _y->min());
   else if (_x->isBound() && _z->isBound())
      _y->assign(_x->min() - _z->min());
   else if (_y->isBound() && _z->isBound())
      _x->assign(_y->min() + _z->min());
   else {
      _x->updateBounds(_y->min() + _z->min(),_y->max() + _z->max());
      _y->updateBounds(_x->min() - _z->max(),_x->max() - _z->min());
      _z->updateBounds(_x->min() - _y->max(),_x->max() - _y->min());
      _x->whenBoundsChange([this] {
         _y->updateBounds(_x->min() - _z->max(),_x->max() - _z->min());
         _z->updateBounds(_x->min() - _y->max(),_x->max() - _y->min());
      });
      _y->whenBoundsChange([this] {
         _x->updateBounds(_y->min() + _z->min(),_y->max() + _z->max());
         _z->updateBounds(_x->min() - _y->max(),_x->max() - _y->min());
      });
      _z->whenBoundsChange([this] {
         _x->updateBounds(_y->min() + _z->min(),_y->max() + _z->max());
         _y->updateBounds(_x->min() - _z->max(),_x->max() - _z->min());
      });
   }
}

void EQTernBCbool::post()
{
   // x == y + b
   if (_x->isBound() && _y->isBound()) {
      if (_x->min() - _y->min() == 1) {
         _b->assign(true);
      }
      else if (_x->min() - _y->min() == 0) {
         _b->assign(false);
      } else {
         failNow();
      }
   }
   else if (_x->isBound() && _b->isTrue())
      _y->assign(_x->min() - 1);
   else if (_x->isBound() && _b->isFalse())
      _y->assign(_x->min());
   else if (_y->isBound() && _b->isTrue())
      _x->assign(_y->min() + 1);
   else if (_y->isBound() && _b->isFalse())
      _x->assign(_y->min());
   else {
      _x->updateBounds(_y->min() + _b->min(),_y->max() + _b->max());
      _y->updateBounds(_x->min() - _b->max(),_x->max() - _b->min());
      _b->updateBounds(_x->min() - _y->max(),_x->max() - _y->min());
      
      _x->whenBoundsChange([this] {
         _y->updateBounds(_x->min() - _b->max(),_x->max() - _b->min());
         _b->updateBounds(_x->min() - _y->max(),_x->max() - _y->min());
      });
      _y->whenBoundsChange([this] {
         _x->updateBounds(_y->min() + _b->min(),_y->max() + _b->max());
         _b->updateBounds(_x->min() - _y->max(),_x->max() - _y->min());
      });
      _b->whenBoundsChange([this] {
         _x->updateBounds(_y->min() + _b->min(),_y->max() + _b->max());
         _y->updateBounds(_x->min() - _b->max(),_x->max() - _b->min());
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

void NEQBinBCLight::print(std::ostream& os) const
{
   os << _x << " !=L " << _y << " + " << _c << std::endl;
}
void NEQBinBCLight::post()
{
   if (_y->isBound())
      _x->remove(_y->min() + _c);
   else if (_x->isBound())
      _y->remove(_x->min() - _c);
   else {
      _x->propagateOnBind(this);
      _y->propagateOnBind(this);
   }
}
void NEQBinBCLight::propagate()
{
   if (_y->isBound())
      _x->remove(_y->min() + _c);
   else
      _y->remove(_x->min() - _c);
   setActive(false);
}

void EQBinDC::post()
{
   _x->propagateOnDomainChange(this);
   _y->propagateOnDomainChange(this);
}

void EQBinDC::propagate()
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
   }
}

void Conjunction::post() // z == x && y
{
   propagate();
   if (!_x->isBound()) _x->propagateOnBind(this);
   if (!_y->isBound()) _y->propagateOnBind(this);
   if (!_z->isBound()) _z->propagateOnBind(this);
}

void Conjunction::propagate()
{
   if (_z->isBound()) {
      if (_z->isTrue()) {
         setActive(false);
         _x->assign(true);
         _y->assign(true);
      } else {
         if (_x->isTrue()) {
            setActive(false);
            _y->assign(false);
         } else if (_y->isTrue()) {
            setActive(false);
            _x->assign(false);
         }
      }
   } else {
      if (_x->isBound() && _y->isBound()) {
         setActive(false);
         _z->assign(_x->min() && _y->min());
      } else if (_x->isFalse() || _y->isFalse()) {
         setActive(false);
         _z->assign(false);
      }        
   }
}

void LessOrEqual::post()
{
   _x->propagateOnBoundChange(this);
   _y->propagateOnBoundChange(this);
   propagate();
}
    
void LessOrEqual::propagate()
{
   _x->removeAbove(_y->max());
   _y->removeBelow(_x->min());
   setActive(_x->max() >= _y->min());
}

Minimize::Minimize(var<int>::Ptr& x)
   : _obj(x),_primal(0x7FFFFFFF)
{
   auto todo = std::function<void(void)>([this]() {
      _obj->removeAbove(_primal - 1);
   });
   _obj->getSolver()->onFixpoint(todo);
}

void Minimize::print(std::ostream& os) const
{
   os << "minimize(" << *_obj << ", primal = " << _primal << ")";
}

void Minimize::tighten()
{
    //assert(_obj->isBound());
   _primal = _obj->max();
   failNow();
}

void Minimize::foundPrimal(int primal)
{
   if (primal < _primal) {
std::cout << "Primal: " << primal << "\n";
      _primal = primal;
      _obj->removeAbove(_primal - 1);
   }
} 

Maximize::Maximize(var<int>::Ptr& x)
   : _obj(x),_primal(0x80000001)
{
   auto todo = std::function<void(void)>([this]() {
      TRYFAIL
         //std::cout << "1=====> objective primal:" << _primal << "  z = " << _obj << std::endl;
         _obj->removeBelow(_primal + 1);
         //std::cout << "1-----> after:" << _obj << std::endl;
      ONFAIL
         failNow();
      ENDFAIL
   });
   _obj->getSolver()->onFixpoint(todo);
}

void Maximize::print(std::ostream& os) const
{
   os << "maximize(" << *_obj << ", primal = " << _primal << ")";
}

void Maximize::tighten()
{
   assert(_obj->isBound());
   _primal = _obj->min();
   failNow();
}

void Maximize::foundPrimal(int primal)
{
   if (primal > _primal) {
      _primal = primal;
      _obj->removeBelow(_primal + 1);
   }
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

void XOR::post()
{
   propagate();
   if (isActive()) {
      _b->propagateOnBind(this);
      _x->propagateOnBind(this);
      _y->propagateOnBind(this);
   }
}

void XOR::propagate()
{
   if (_b->isTrue()) {
      if (_x->isTrue()) {
         _y->assign(false);
         setActive(false);
      } else if (_x->isFalse()) {
         _y->assign(true);
         setActive(false);
      } else if (_y->isTrue()) {
         _x->assign(false);
         setActive(false);
      } else if (_y->isFalse()) {
         _x->assign(true);
         setActive(false);
      }
   } else if (_b->isFalse()) {
      if (_x->isTrue()) {
         _y->assign(true);
         setActive(false);
      } else if (_x->isFalse()) {
         _y->assign(false);
         setActive(false);
      } else if (_y->isTrue()) {
         _x->assign(true);
         setActive(false);
      } else if (_y->isFalse()) {
         _x->assign(false);
         setActive(false);
      }
   } else if (_x->isBound() && _y->isBound()) {
      if (_x->isTrue() == _y->isTrue()) {
         _b->assign(false);
         setActive(false);
      } else {
         _b->assign(true);
         setActive(false);
      }
   }
}

void IsMember::post() 
{
   propagate();
   if (isActive()) {
      _x->propagateOnDomainChange(this);
      _b->propagateOnBind(this);
   }
}

void IsMember::propagate() 
{  
   if (_b->isTrue()) {
      int xMin = _x->min(), xMax = _x->max();
      for (int v=xMin; v<=xMax; v++) {
         // if v is not in S: remove from domain of x
         if (_x->contains(v) && (_S.find(v) == _S.end()))
            _x->remove(v);
      }
      setActive(false);
   } else if (_b->isFalse()) {
      // remove all elements in S from domain of x
      for (std::set<int>::iterator it=_S.begin(); it!=_S.end(); ++it) {
         _x->remove(*it);
      }
      setActive(false);
   } else if (_x->isBound()) {
      int v = _x->min();
      if (_S.find(v)!=_S.end())
         _b->assign(true);
      else
         _b->assign(false);
      setActive(false);
   } else {
      // both b and x are not bound: check if x still has value in S and a value not in S
      bool hasMemberInS = false;
      bool hasMemberOutS = false;

      int xMin = _x->min(), xMax = _x->max();
      for (int v=xMin; v<=xMax; v++) {
         if (_x->contains(v)) {
            if (_S.find(v) == _S.end()) {
               hasMemberOutS = true;
            }
            else {
               hasMemberInS = true;
            }
         }
         if ((hasMemberInS == true) && (hasMemberOutS == true))
            break;
      }
      if (hasMemberInS==false) {
         _b->assign(false);
         setActive(false);
      }
      else if (hasMemberOutS==false) {
         _b->assign(true);
         setActive(false);
      }
   }
}


void IsLessOrEqual::post()
{
   if (_b->isTrue())
      _x->removeAbove(_c);
   else if (_b->isFalse())
      _x->removeBelow(_c + 1);
   else if (_x->max() <= _c)
      _b->assign(1);
   else if (_x->min() > _c)
      _b->assign(0);
   else {
      _b->whenBind([b=_b,x=_x,c=_c] {
         if (b->isTrue())
            x->removeAbove(c);
         else x->removeBelow(c + 1);
      });
      _x->whenBoundsChange([b=_b,x=_x,c=_c] {
         if (x->max() <= c)
            b->assign(1);
         else if (x->min() > c)
            b->assign(0);
      });
   }
}

void IsLessOrEqualVar::post()
{
   _x->propagateOnBoundChange(this);
   _y->propagateOnBoundChange(this);
   _b->propagateOnBind(this);
   propagate();
}

void IsLessOrEqualVar::propagate()
{
   if (_b->isTrue()) {            // x <= y
      _x->removeAbove(_y->max());
      _y->removeBelow(_x->min());
      setActive(_x->max() > _y->min());
   } else if (_b->isFalse()) {    // x >= y + 1
      _x->removeBelow(_y->min() + 1);
      _y->removeAbove(_x->max() - 1);
      setActive(_x->min() <= _y->max());
   } else if (_x->max() <= _y->min()) {
      _b->assign(true);
      setActive(false);
   } else if (_x->min() > _y->max()) {
      _b->assign(false);
      setActive(false);
   }
}

/*** Domain (bounds) propagation for AbsDiff constraint ***/
bool propagateExpression(const interval* _x, const interval* _y,
                         const interval* _z, interval &v1, interval &v3)
{

  /*** 
   * Apply interval propagation to expression [z] = [Abs[ [[x] - [y]] ]] :
   * First propagate x, y, z bounds 'up' to intersect [z] == [v3]
   * Then propagate the intervals 'down' back to x, y, z: this done in 
   * constraint, based on v1 and v3.
   ***/
  
  // Up-propagate:
  v1.min = _x->min - _y->max;
  v1.max = _x->max - _y->min;
  interval v2(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
  if (!((v1.max >= 0) && (v1.min<=0)))
    v2.min = std::min(std::abs(v1.min), std::abs(v1.max));
  v2.max = std::max(std::abs(v1.min), std::abs(v1.max));
  v3.min = std::max(_z->min, v2.min);
  v3.max = std::min(_z->max, v2.max);
  
  // Down-propagate for v1, v2, v3 (their bounds will be used for x, y, z):
  v2.min = std::max(v2.min, v3.min);
  v2.max = std::min(v2.max, v3.max);
  v1.min = std::max(v1.min, -(v2.max));
  v1.max = std::min(v1.max, v2.max);
  
  if ((v1.max < v1.min) || (v2.max < v2.min) || (v3.max < v3.min))
    return false;
  return true;
}

void EQAbsDiffBC::post()
{
   // z == |x - y|
   if (_x->isBound() && _y->isBound()) {
     _z->assign(std::abs(_x->min()-_y->min()));
   }
   else {
      interval v1(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
      interval v3(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
      interval X(_x->min(), _x->max());
      interval Y(_y->min(), _y->max());
      interval Z(_z->min(), _z->max());
      bool check = propagateExpression(&X, &Y, &Z, v1, v3);
      if (!check) { failNow(); }
      
      _z->updateBounds(std::max(_z->min(),v3.min), std::min(_z->max(),v3.max));
      _x->updateBounds(std::max(_x->min(),_y->min()+v1.min), std::min(_x->max(),_y->max()+v1.max));
      _y->updateBounds(std::max(_y->min(),_x->min()-v1.max), std::min(_y->max(),_x->max()-v1.min));
      
      _x->whenBoundsChange([this] {
         interval v1(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
         interval v3(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
         interval X(_x->min(), _x->max());
         interval Y(_y->min(), _y->max());
         interval Z(_z->min(), _z->max());
         bool check = propagateExpression(&X, &Y, &Z, v1, v3);
         if (!check)  failNow();
         _z->updateBounds(std::max(_z->min(),v3.min), std::min(_z->max(),v3.max));
         _y->updateBounds(std::max(_y->min(),_x->min()-v1.max), std::min(_y->max(),_x->max()-v1.min));
      });
      
      _y->whenBoundsChange([this] {
         interval v1(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
         interval v3(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
         interval X(_x->min(), _x->max());
         interval Y(_y->min(), _y->max());
         interval Z(_z->min(), _z->max());
         bool check = propagateExpression(&X, &Y, &Z, v1, v3);
         if (!check) { failNow(); }
         _z->updateBounds(std::max(_z->min(),v3.min), std::min(_z->max(),v3.max));
         _x->updateBounds(std::max(_x->min(),_y->min()+v1.min), std::min(_x->max(),_y->max()+v1.max));
      });
     
     // Applying domain consistency to x and y when Dom(z) changes seems to mimick the Puget&Regin approach.
     _z->whenDomainChange([this] {
       for (int x=_x->min(); x<=_x->max(); x++) {
         if (_x->contains(x)) {
           bool support = false;
           for (int y=_y->min(); y<=_y->max() && !support; y++) {
             if (_y->contains(y)) {
               for (int z=_z->min(); z<=_z->max() && !support; z++) {
                 if (_z->contains(z) && (z==std::abs(x-y))){
                   support = true;
                   break;
                 }
               }
             }
           }
           if (!support) { _x->remove(x); }
         }
       }
       for (int y=_y->min(); y<=_y->max(); y++) {
         if (_y->contains(y)) {
           bool support = false;
           for (int x=_x->min(); x<=_x->max() && !support; x++) {
             if (_x->contains(x)) {
               for (int z=_z->min(); z<=_z->max() && !support; z++) {
                 if (_z->contains(z) && (z==std::abs(x-y))){
                   support = true;
                   break;
                 }
               }
             }
           }
           if (!support) { _y->remove(y); }
         }
       }
     });
   }
}
/***/

void Absolute::post()
{
   _y->removeBelow(0);
   _x->propagateOnDomainChange(this);   // holes in x can raise y.min
   _y->propagateOnBoundChange(this);
   propagate();
}

void Absolute::propagate()
{
   if (_x->isBound()) {
      _y->assign(std::abs(_x->min()));
      setActive(false);
   } else if (_y->isBound()) {  // x in {-y, y}
      const int v = _y->min();
      if (!_x->contains(-v))
         _x->assign(v);
      else if (!_x->contains(v))
         _x->assign(-v);
      else {
         for (int i = _x->min(); i <= _x->max(); i++)
            if (i != v && i != -v)
               _x->remove(i);
      }
      setActive(false);
   } else if (_x->min() >= 0) {
      _y->removeBelow(_x->min());
      _y->removeAbove(_x->max());
      _x->removeBelow(_y->min());
      _x->removeAbove(_y->max());
   } else if (_x->max() <= 0) {
      _y->removeBelow(-_x->max());
      _y->removeAbove(-_x->min());
      _x->removeBelow(-_y->max());
      _x->removeAbove(-_y->min());
   } else {
      _y->removeAbove(std::max(_x->max(),-_x->min()));
      _x->removeAbove(_y->max());
      _x->removeBelow(-_y->max());
      while (!_x->contains(_y->min()) && !_x->contains(-_y->min()))
         _y->remove(_y->min());
   }
}
