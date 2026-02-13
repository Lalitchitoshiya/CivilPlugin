#pragma once
#include <string>
#include <vector>

class StringUtils
{
public:
    static std::vector<std::string> Split(const std::string& line, char delimiter);
};