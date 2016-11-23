#pragma once
#include <string>
#include <sstream>
#include "userfuncs.hpp"

struct noreturn_t{};
extern const noreturn_t noreturn;

template <class T>
T strast(const std::string& str) {
    std::stringstream ss(str);
    T t{};
    ss >> t;
    return t;
}

template <class... Args>
decltype(auto) invoke(const std::string& fstr, Args... args) {
    std::string agg[] {args...};

    if (fstr == "example")
        return example();

    else if (fstr == "example2")
        return example2(strast<int>(agg[0]));

    else if (fstr == "example3")
        return example3();

    else if (fstr == "add")
        return add(strast<int>(agg[0]), strast<int>(agg[1]));

    else
        return noreturn;
}
