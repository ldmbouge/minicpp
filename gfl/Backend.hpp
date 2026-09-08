#pragma once

#ifdef __CUDACC__
#include <cuda/std/bit>
#include <cuda/std/cstddef>
#include <cuda/std/functional>
#include <cuda/std/initializer_list>
#include <cuda/std/limits>
#include <cuda/std/optional>
#include <cuda/std/span>
#include <cuda/std/ranges>
#include <cuda/std/tuple>
#include <cuda/std/utility>
#else
#include <bit>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <tuple>
#include <utility>
#endif

namespace gfl {
#ifdef __CUDACC__
namespace backend = cuda::std;
#else
namespace backend = std;
#endif

// Optional
using backend::nullopt;
using backend::optional;

// Tuple
using backend::get;
using backend::ignore;
using backend::tie;
using backend::tuple;

// Functional
using backend::greater;
using backend::less;

// Span
using backend::dynamic_extent;
using backend::span;

// Bits
using backend::bit_cast;
using backend::bit_width;
using backend::countl_zero;
using backend::countr_zero;
using backend::popcount;

// Limits
using backend::numeric_limits;

// Utility
using backend::exchange;
using backend::forward;
using backend::move;

// Standard definitions
using backend::nullptr_t;

// Initializer list
using backend::initializer_list;

// Ranges
namespace ranges = backend::ranges;
}