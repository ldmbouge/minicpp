#pragma once

#include <cassert>
#include <tuple>

#include "Alias.hpp"
#include "BitSet.hpp"
#include "DebugUtils.hpp"
#include "FunQual.hpp"

namespace gfl {
template<typename Range, typename Pred> requires ranges::range<Range>
GFL_HOST_DEVICE bool any(Range const& range, Pred const& pred) {
    for (auto const& value : range)
        if (pred(value)) return true;
    return false;
}
template<typename Range, typename Pred> requires ranges::range<Range>
GFL_HOST_DEVICE bool all(Range const& range, Pred const& pred) {
    for (auto const& value : range)
        if (!pred(value)) return false;
    return true;
}
template<typename Range, typename Pred> requires ranges::range<Range>
inline GFL_HOST_DEVICE bool none(Range const& range, Pred const& pred) {
    return !any(range, pred);
}
template<typename Range, typename Pred> requires ranges::range<Range>
GFL_HOST_DEVICE i32 count(Range const& range, Pred const& pred) {
    i32 result = 0;
    for (auto const& value : range)
        if (pred(value)) ++result;
    return result;
}
template<typename Range, typename Pred> requires ranges::range<Range>
GFL_HOST_DEVICE auto min(Range const& range, Pred const& pred) {
    assert(range.begin() != range.end());
    using ResultType = decltype(pred(*range.begin()));
    ResultType result = highest<ResultType>();
    for (auto const& value : range) {
        auto const current = pred(value);
        if (current < result) result = current;
    }
    return result;
}
template<typename Range, typename Filter, typename Pred> requires ranges::range<Range>
GFL_HOST_DEVICE auto min(Range const& range, Filter const& filter, Pred const& pred) {
    assert(range.begin() != range.end());
    using ResultType = decltype(pred(*range.begin()));
    ResultType result = highest<ResultType>();
    for (auto const& value : range) {
        if (!filter(value)) continue;
        auto const current = pred(value);
        if (current < result) result = current;
    }
    return result;
}
template<typename Range, typename Pred> requires ranges::range<Range>
GFL_HOST_DEVICE auto max(Range const& range, Pred const& pred) {
    assert(range.begin() != range.end());
    using ResultType = decltype(pred(*range.begin()));
    ResultType result = lowest<ResultType>();
    for (auto const& value : range) {
        auto const current = pred(value);
        if (current > result) result = current;
    }
    return result;
}
template<typename Range, typename Filter, typename Pred> requires ranges::range<Range>
GFL_HOST_DEVICE auto max(Range const& range, Filter const& filter, Pred const& pred) {
    assert(range.begin() != range.end());
    using ResultType = decltype(pred(*range.begin()));
    ResultType result = lowest<ResultType>();
    for (auto const& value : range) {
        if (!filter(value)) continue;
        auto const current = pred(value);
        if (current > result) result = current;
    }
    return result;
}
template<typename Range, typename Pred> requires ranges::range<Range>
GFL_HOST_DEVICE auto argmin(Range const& range, Pred const& pred) {
    assert(range.begin() != range.end());
    auto it = range.begin();
    auto bestValue = *it;
    auto bestEval = pred(bestValue);
    for (++it; it != range.end(); ++it) {
        auto const current = pred(*it);
        if (current < bestEval) {
            bestValue = *it;
            bestEval = current;
        }
    }
    return tuple{bestValue, bestEval};
}
template<typename Range, typename Pred> requires ranges::range<Range>
GFL_HOST_DEVICE auto argmax(Range const& range, Pred const& pred) {
    assert(range.begin() != range.end());
    auto it = range.begin();
    auto bestValue = *it;
    auto bestEval = pred(bestValue);
    for (++it; it != range.end(); ++it) {
        auto const current = pred(*it);
        if (current > bestEval) {
            bestValue = *it;
            bestEval = current;
        }
    }
    return tuple{bestValue, bestEval};
}
template<typename Range, typename Acc, typename BinOp> requires ranges::range<Range>
GFL_HOST_DEVICE auto fold(Range const& range, Acc init, BinOp const& op) {
    for (auto const& value : range) init = op(init, value);
    return init;
}
template<i32 Capacity, typename Pred> GFL_HOST_DEVICE BitSet<Capacity> filter(BitSet<Capacity> const& set, Pred const& pred) {
    BitSet<Capacity> result;
    for (i32 const value : set)
        if (pred(value)) result.insert(value);
    return result;
}

namespace details {
template<typename Tuple> void checkSize(Tuple const& containers, i64 size) {
    auto const check = [size](auto const&... container) {
        if (!((container.size() == size) && ...)) abortWithMsg("sortByKey: All containers must have the same size.");
    };
    std::apply(check, containers);
}
// Swaps position i and j in every container at once, so a single call keeps
// every container — and, below, the keys driving the comparisons — aligned
// to the same permutation. No separate index array is ever built or applied.
template<typename Tuple> void swapAt(Tuple& containers, i64 i, i64 j) {
    auto const swap = [i, j](auto&... container) { (std::swap(container[i], container[j]), ...); };
    std::apply(swap, containers);
}
// Lomuto-partition quicksort over `keys`: every time it swaps two keys, it
// swaps the same two positions in every container via swapAt, so containers
// end up co-sorted with keys directly — no permutation to compute or apply
// as a separate pass afterward.
template<typename Tuple, typename Key> void quickSortByKey(Tuple& containers, std::vector<Key>& keys, i64 lo, i64 hi) {
    if (lo >= hi) return;
    Key const pivot = keys[asSize(hi)];
    i64 i = lo - 1;
    for (i64 j = lo; j < hi; ++j) {
        if (keys[asSize(j)] < pivot) {
            ++i;
            std::swap(keys[asSize(i)], keys[asSize(j)]);
            swapAt(containers, i, j);
        }
    }
    ++i;
    std::swap(keys[asSize(i)], keys[asSize(hi)]);
    swapAt(containers, i, hi);
    quickSortByKey(containers, keys, lo, i - 1);
    quickSortByKey(containers, keys, i + 1, hi);
}
// The actual implementation, over a tuple of containers. Not named
// sortByKey and not public: every real call site goes through the 1-4
// container overloads below, so this never needs to coexist in overload
// resolution with sortByKey(C1&, KeyFn) — no tuple-vs-container ambiguity
// to disambiguate.
template<typename Tuple, typename KeyFn> void sortByKeyImpl(Tuple containers, KeyFn keyFn) {
    i64 const size = std::get<0>(containers).size();
    checkSize(containers, size);
    using Key = std::decay_t<decltype(keyFn(i64{}))>;
    std::vector<Key> keys(asSize(size));
    for (i64 i = 0; i < size; ++i) keys[asSize(i)] = keyFn(i);
    if (size > 1) quickSortByKey(containers, keys, 0, size - 1);
}
}

template<typename C1, typename KeyFn> void sortByKey(C1& c1, KeyFn keyFn) { details::sortByKeyImpl(std::tie(c1), keyFn); }
template<typename C1, typename C2, typename KeyFn> void sortByKey(C1& c1, C2& c2, KeyFn keyFn) { details::sortByKeyImpl(std::tie(c1, c2), keyFn); }
template<typename C1, typename C2, typename C3, typename KeyFn> void sortByKey(C1& c1, C2& c2, C3& c3, KeyFn keyFn) { details::sortByKeyImpl(std::tie(c1, c2, c3), keyFn); }
template<typename C1, typename C2, typename C3, typename C4, typename KeyFn> void sortByKey(C1& c1, C2& c2, C3& c3, C4& c4, KeyFn keyFn) { details::sortByKeyImpl(std::tie(c1, c2, c3, c4), keyFn); }
}
