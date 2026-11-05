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
 *
 * Contributions by Heytem Zitoun, Waldemar Cruz, Rebecca Gentzel, Willem Jan Van Hoeve
 */

#pragma once

#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <concepts>
#include <type_traits>
#include <numeric>
#include <iterator>
#include <utility>

//#define TRACE(...) __VA_ARGS__
#define TRACE(...)
#define TO_STRING(e) #e

inline double division(int numerator, int denominator)
{
    return static_cast<double>(numerator) / static_cast<double>(denominator);
}

inline int floorDivision (int numerator, int denominator)
{
    return static_cast<int>(std::floor(division(numerator, denominator)));
}

inline int ceilDivision (int numerator, int denominator)
{
    return static_cast<int>(std::ceil(division(numerator, denominator)));
}

inline double power(int base, double exponent)
{
    return std::pow(static_cast<double>(base), exponent);
}

inline int floorPower (int base, double exponent)
{
    return static_cast<int>(std::floor(power(base, exponent)));
}

inline int ceilPower (int base, double exponent)
{
    return static_cast<int>(std::ceil(power(base, exponent)));
}

inline double logarithm(int base, int number)
{
    return log2(static_cast<double>(number)) / log2(static_cast<double>(base));
}

inline int floorLogarithm (int base, int number)
{
    return static_cast<int>(std::floor(logarithm(base, number)));
}

inline int ceilLogarithm(int base, int number)
{
    return static_cast<int>((std::ceil(logarithm(base, number))));
}

/**
 * Sum of the (numeric) elements of a container, computed with std::accumulate.
 * Only enabled for arithmetic element types so that it never competes with the
 * variable-based Factory::sum overloads.
 * @param c any container with begin()/end()
 * @return the sum, of the container's element type (0 when empty)
 */
template <class C,
          class T = std::decay_t<decltype(*std::begin(std::declval<const C&>()))>,
          std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
T sum(const C& c)
{
    return std::accumulate(std::begin(c), std::end(c), T{});
}

/**
 * Sum of f(x) over the elements x of a container, computed with std::accumulate.
 * Only enabled when f returns an arithmetic type. Useful for nested containers:
 *    sum(matrix, [](const auto& row) { return sum(row); })
 * @param c any container with begin()/end()
 * @param f a function applied to each element
 * @return the sum of the f(x), of f's result type (0 when empty)
 */
template <class C, class F,
          class R = std::decay_t<decltype(std::declval<F&>()(*std::begin(std::declval<const C&>())))>,
          std::enable_if_t<std::is_arithmetic_v<R>, int> = 0>
R sum(const C& c, F f)
{
    return std::accumulate(std::begin(c), std::end(c), R{},
                           [&f](R acc, const auto& x) { return acc + f(x); });
}

inline void printError(std::string const & error)
{
    std::cerr << "% [ERROR] " << error << std::endl;
}

template<typename C>
void printVector(std::ostream& os,const C& vector)
{
    if (vector.size() > 0) {
        os << '[' << vector[0];
        for (size_t i = 1; i < vector.size(); i += 1)
            os << "," << vector[i];
        os << ']';        
    } else os << "[]";
}

template<typename T>
std::ostream& operator<<(std::ostream& os,const std::vector<T>& vector)
{
    printVector(os,vector);
    return os;
}

