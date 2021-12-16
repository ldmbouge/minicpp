#pragma once

#include <cmath>
#include <iostream>
#include <string>

//#define TRACE(...) __VA_ARGS__
#define TRACE(...)
#define ENUM2STR(e) #e

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
