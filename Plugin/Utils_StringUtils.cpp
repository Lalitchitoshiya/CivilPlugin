#include "Utils_StringUtils.h"
#include <sstream>

std::vector<std::string> StringUtils::Split(const std::string& line, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string item;

    while (std::getline(ss, item, delimiter))
    {
        tokens.push_back(item);
    }
    return tokens;
}