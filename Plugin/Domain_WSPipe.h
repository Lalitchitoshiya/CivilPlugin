#pragma once
#include <string>

struct WSPipe
{
    std::string id;
    std::string startNodeId;
    std::string endNodeId;
    double usInvert = 0.0;   // ADD: upstream invert (Z of start node)
    double dsInvert = 0.0;   // ADD: downstream invert (Z of end node)
    double length = 0.0;   // ADD: horizontal pipe length
};