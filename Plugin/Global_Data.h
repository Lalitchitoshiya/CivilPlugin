#pragma once
#include <map>
#include <vector>
#include "gepnt3d.h"
#include "Domain_WSPipe.h"

extern std::map<int, AcGePoint3d> g_NodeMap;
extern std::vector<WSPipe>        g_PipeList;  // ADD THIS