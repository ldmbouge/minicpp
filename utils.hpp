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

inline void printError(std::string const & error)
{
    std::cerr << "% [ERROR] " << error << std::endl;
}

template<typename T>
void printVector(std::ostream& os, std::vector<T>& vector)
{
    if (not vector.empty())
    {
        os << vector[0];
        for (size_t i = 1; i < vector.size(); i += 1)
        {
            os << "," << vector[i];
        };
    }
}
