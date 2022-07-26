#ifndef __ALLINTERVAL_H
#define __ALLINTERVAL_H

#include <set>
#include <array>
#include <map>
#include <algorithm>
#include <iomanip>
#include <stdint.h>
#include "intvar.hpp"
#include "mddstate.hpp"

namespace Factory {
   MDDCstrDesc::Ptr absDiffMDD(MDD::Ptr m, const Factory::Veci& vars);
}

#endif
