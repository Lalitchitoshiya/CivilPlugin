#include "Import_CSVParser.h"
#include "Utils_StringUtils.h"
#include <fstream>

std::vector<std::vector<std::string>> CSVParser::Parse(const std::string& filepath)
{
    std::vector<std::vector<std::string>> rows;
    std::ifstream file(filepath);

    if (!file.is_open())
        return rows;

    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty()) continue;
        rows.push_back(StringUtils::Split(line, ','));
    }

    file.close();
    return rows;
}