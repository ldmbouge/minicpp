#pragma once

#include <iostream>
#include <cassert>
#include <cstdio>
#include <functional>
#include <type_traits>

#include "FunQual.hpp"
#include "Types.hpp"

namespace gfl {
  template <typename T>
  class Scalar {
    T*  _valptr;
  public:
    GFL_HOST_DEVICE Scalar() : _valptr(nullptr) {}
    GFL_HOST_DEVICE Scalar(T* vp) : _valptr(vp) {}
    GFL_HOST_DEVICE Scalar(const Scalar<T>& o) : _valptr(o._valptr) {}
    GFL_HOST_DEVICE Scalar(Scalar<T>&& o) { _valptr = o._valptr;o._valptr = nullptr;}
    GFL_HOST_DEVICE Scalar<T>& operator=(const Scalar<T>& o) noexcept { _valptr = o._valptr;return *this;}
    GFL_HOST_DEVICE Scalar<T>& operator=(Scalar<T>&& o) noexcept { _valptr = o._valptr;o._valptr = nullptr;return *this;}
    GFL_HOST_DEVICE Scalar<T>& operator=(T v) noexcept { *_valptr = v;}
    GFL_HOST_DEVICE operator T() const noexcept  { return *_valptr;}
    GFL_HOST_DEVICE operator T*() const noexcept { return _valptr;}
  };
};
