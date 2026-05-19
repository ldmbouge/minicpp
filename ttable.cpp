#include "ttable.hpp"
#include "fail.hpp"

void CumulativeTT::post() {
   using namespace Factory;
   for (auto &v : _start)
      v->propagateOnBoundChange(this);
   if (_postMirror) {
      auto cp = _start[0]->getSolver();
      auto startMirror = Factory::intVarArray(cp, _start.size());
      int i = 0;
      for (auto &e : _end)
         startMirror[i++] = -e;
      cp->post(new CumulativeTT(startMirror,_duration,_demand,_capa,false),false);
   }
   propagate();
}

Profile *CumulativeTT::buildProfile() {
    std::vector<Profile::Rectangle> inp;
    for (auto i = 0u; i < _start.size(); i++) {
        const auto maxS = _start[i]->max();
        const auto minE = _end[i]->min();
        if (minE > maxS) // mandatory part
            inp.emplace_back(Profile::Rectangle(maxS, minE, _demand[i]));
    }     
    return new Profile(inp);   
}  

void CumulativeTT::propagate()
{
   std::unique_ptr<Profile> profile(buildProfile());
   for (int i = 0u; i < profile->size(); i++)
      if (profile->get(i).getHeight() > _capa) {
         profile = nullptr;
         failNow();
      }
   for (auto i = 0u; i < _start.size(); i++) {
      if (!_start[i]->isBound()) {
         int j = profile->rectangleIndex(_start[i]->min());
         int t = _start[i]->min();
         while (j < profile->size() &&
                profile->get(j).getStart() < std::min(t + _duration[i], _start[i]->max())) {
            if (_capa - _demand[i] < profile->get(j).getHeight())
               t = std::min(profile->get(j).getEnd(), _start[i]->max());
            j++;
         }
         TRYFAIL
            _start[i]->removeBelow(t);
         ONFAIL {
            profile = nullptr; // deallocate and fail again
            failNow();
         } ENDFAIL;
      }
   }
}   

