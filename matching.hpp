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

#ifndef __MATCHING_H
#define __MATCHING_H

#include <algorithm>
#include <stack>
#include <iomanip>
#include <stdint.h>
#include "intvar.hpp"

class Graph {
   int V;
   std::vector<std::vector<int>> adj;
   template <typename B>
   void SCCUtil(B body,int& time,int u, int disc[], int low[],std::stack<int>& st, bool inStack[]);
public:
   Graph(int nbV) : V(nbV),adj(nbV) {}
   Graph() : V(0) {}
   Graph(Graph&& g) : V(g.V),adj(std::move(g.adj)) {}
   Graph& operator=(Graph&& g) { V = std::move(g.V); adj = std::move(g.adj);return *this;}
   void clear()                { for(auto& al : adj) al.clear();}
   void addEdge(int v, int w)  { adj[v].push_back(w);}
   template <typename B> void SCC(B body); // apply body to each SCC
};

class MaximumMatching {
   Storage::Ptr _store;
   Factory::Veci& _x;
   int* _match,*_varSeen;
   int _min,_max;
   int _valSize;
   int*  _valMatch,*_valSeen;
   int _szMatching;
   int _magic;
   void findInitialMatching();
   int findMaximalMatching();
   bool findAlternatingPathFromVar(int i);
   bool findAlternatingPathFromVal(int v);
public:
   MaximumMatching(Factory::Veci& x,Storage::Ptr store)
      : _store(store),_x(x) {}
   ~MaximumMatching();
   void setup();
   int compute(int result[]);
};

template <typename B>
void Graph::SCCUtil(B body,int& time,int u,int disc[], int low[],std::stack<int>& st,bool inStack[])
{
   disc[u] = low[u] = ++time;
   st.push(u);
   inStack[u] = true;
   for(const auto v : adj[u]) { // v is current adjacent of 'u'
      if (disc[v] == -1)  {
         SCCUtil(body,time,v, disc, low, st, inStack);
         low[u] = std::min(low[u], low[v]);
      }
      else if (inStack[v])
          low[u] = std::min(low[u], disc[v]);
   }
   if (low[u] == disc[u])  {
      int* scc = (int*)alloca(sizeof(int)*V),k=0;
      while (st.top() != u)  {
         scc[k++] = st.top();
         inStack[scc[k-1]] = false;
         st.pop();
      }
      scc[k++] = st.top();
      inStack[scc[k-1]] = false;
      st.pop();
      body(k,scc);
   }
}

template <typename B> void Graph::SCC(B body) {
   int* disc = (int*)alloca(sizeof(int)*V);
   int* low  = (int*)alloca(sizeof(int)*V);
   bool* inStack = (bool*)alloca(sizeof(bool)*V);
   std::stack<int> st;
   int time = 0;
   for (int i = 0; i < V; i++)  {
      disc[i] = low[i] = -1;
      inStack[i] = false;
   }
   for (int i = 0; i < V; i++)
      if (disc[i] == -1)
         SCCUtil(body,time,i, disc, low, st,inStack);
}

#endif
