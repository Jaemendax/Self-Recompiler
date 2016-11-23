#pragma once

#include <string>
#include <vector>

struct FuncInfo
{
    std::string funcname;
    std::vector<std::string> argtypes;
    unsigned numargs;
};
