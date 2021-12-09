#pragma once

#include <cmath>

inline int floorDivision (int a, int b)
{
    return static_cast<int>(std::floor(static_cast<double>(a) / static_cast<double>(b)));
}

inline int ceilDivision (int a, int b)
{
    return static_cast<int>(std::ceil(static_cast<double>(a) / static_cast<double>(b)));
}
