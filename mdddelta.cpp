#include "mdddelta.hpp"

void MDDDelta::adaptDelta()
{
   int nsz = _nf->peakNodes();
   MDDStateDelta** nt = new MDDStateDelta*[nsz];
   for(int i=0;i < _csz;i++)
      nt[i] = _t[i];
   for(int i=_csz;i < nsz;i++)
      nt[i] = nullptr;
   delete[]_t;
   _t = nt;
   _csz = nsz;
}
