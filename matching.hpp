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
   struct AdjList {
      int* _t;
      int  _n;
      AdjList():_t(nullptr),_n(0) {}
      AdjList(int n)              { _t = new int[n];_n = 0;}
      ~AdjList()                  { delete []_t;}
      void clear() noexcept       { _n = 0;}
      void append(int v) noexcept { _t[_n++] = v;}
   };
   AdjList* adj;
   template <typename B>
   void SCCUtil(B body,int& time,int u, int disc[], int low[],
                int st[],int& top,bool inStack[]);
public:
   Graph(int nbV) : V(nbV) {
      adj = new AdjList[V];
      for(int i=0;i<V;i++) new (adj+i) AdjList(V);      
   }
   Graph() : V(0),adj(nullptr) {}
   ~Graph() { delete []adj;}
   Graph& operator=(Graph&& g)  {
      V = g.V;
      adj = std::move(g.adj);
      g.adj = nullptr;
      return *this;
   }
   void clear() noexcept               { for(int i=0;i<V;i++) adj[i].clear();}
   void addEdge(int v, int w) noexcept { adj[v].append(w);}
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
void Graph::SCCUtil(B body,int& time,int u,int disc[], int low[],
                    int st[],int& top,bool inStack[])
{
   disc[u] = low[u] = ++time;
   st[top++] = u;
   inStack[u] = true;
   for(int k=0;k < adj[u]._n;k++) {
      const auto v = adj[u]._t[k];  // v is current adjacent of 'u'
      if (disc[v] == -1)  {
         SCCUtil(body,time,v, disc, low, st,top,inStack);
         low[u] = std::min(low[u], low[v]);
      }
      else if (inStack[v])
          low[u] = std::min(low[u], disc[v]);
   }
   if (low[u] == disc[u])  {
      int* scc = (int*)alloca(sizeof(int)*V),k=0;
      while (st[top-1] != u)  {
         scc[k++] = st[--top];
         inStack[scc[k-1]] = false;
      }
      scc[k++] = st[--top];
      inStack[scc[k-1]] = false;
      body(k,scc);
   }
}

template <typename B> void Graph::SCC(B body) {
   int* disc = (int*)alloca(sizeof(int)*V);
   int* low  = (int*)alloca(sizeof(int)*V);
   int* st   = (int*)alloca(sizeof(int)*V);
   bool* inStack = (bool*)alloca(sizeof(bool)*V);
   int top = 0;
   int time = 0;
   for (int i = 0; i < V; i++)  {
      disc[i] = low[i] = -1;
      inStack[i] = false;
   }
   for (int i = 0; i < V; i++)
      if (disc[i] == -1)
         SCCUtil(body,time,i, disc, low, st,top,inStack);
}

#endif
