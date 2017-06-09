#ifndef __CONSTRAINT_H
#define __CONSTRAINT_H

#include "intvar.hpp"
#include "acstr.hpp"

class EQc : public Constraint { // x == c
   var<int>::Ptr _x;
   int           _c;
public:
   EQc(var<int>::Ptr& x,int c) : _x(x),_c(c) {}
   //~EQc() { std::cout << "destroying " << _x << " == " << _c << std::endl;}           
   void post() override;
};

class NEQc : public Constraint { // x != c
   var<int>::Ptr _x;
   int           _c;
public:
   NEQc(var<int>::Ptr& x,int c) : _x(x),_c(c) {}
   void post() override;
};

class EQBinBC : public Constraint { // x == y + c
   var<int>::Ptr _x,_y;
   int _c;
public:
   EQBinBC(var<int>::Ptr& x,var<int>::Ptr& y,int c) : _x(x),_y(y),_c(c) {}
   void post() override;
};

class NEQBinBC : public Constraint { // x != y + c
   var<int>::Ptr _x,_y;
   int _c;
   revList<std::function<void(void)>>::revNode* hdl[2];
   void print(std::ostream& os) const override;
public:
   NEQBinBC(var<int>::Ptr& x,var<int>::Ptr& y,int c) : _x(x),_y(y),_c(c) {}
   //~NEQBinBC() { std::cout << this << " : ~ != BIN" << std::endl;}
   void post() override;
};

class EQBinDC : public Constraint { // x == y + c
   var<int>::Ptr _x,_y;
   int _c;
public:
   EQBinDC(var<int>::Ptr& x,var<int>::Ptr& y,int c) : _x(x),_y(y),_c(c) {}
   void post() override;
};

namespace Factory {
   inline Constraint::Ptr makeEQBinBC(var<int>::Ptr x,var<int>::Ptr y,int c) {
      return make_handle<EQBinBC>(x,y,c);
   }
   inline Constraint::Ptr makeNEQBinBC(var<int>::Ptr x,var<int>::Ptr y,int c) {
      return make_handle<NEQBinBC>(x,y,c);
   }
   inline Constraint::Ptr operator==(var<int>::Ptr x,int c) {
      return make_handle<EQc>(x,c);
   }
   inline Constraint::Ptr operator!=(var<int>::Ptr x,int c) {
      return make_handle<NEQc>(x,c);
   }
};

inline Constraint::Ptr operator!=(var<int>::Ptr x,var<int>::Ptr y)
{
   return Factory::makeNEQBinBC(x,y,0);
}


#endif
