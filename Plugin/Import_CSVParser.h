#pragma once
#include <vector>
#include <string>

class CSVParser
{
public:
    static std::vector<std::vector<std::string>> Parse(const std::string& filepath);
};