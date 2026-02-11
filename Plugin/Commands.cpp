#include "aced.h"
#include "acedads.h"
#include "dbsymtb.h"
#include "dbapserv.h"
#include "dbents.h"
//#include "dbline.h"
#include "dbpl.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cmath>

#ifndef RTNORM
#define RTNORM 5100
#endif

/* ============================================================
   Forward declarations
   ============================================================ */
void importWSProNodes();
void importWSProPipes();

/* ============================================================
   Command registration
   ============================================================ */
void initCommands()
{
    acedRegCmds->addCommand(
        L"WSPRO_COMMANDS",
        L"WSPROIMPORTNODES",
        L"WSPROIMPORTNODES",
        ACRX_CMD_MODAL,
        importWSProNodes);

    acedRegCmds->addCommand(
        L"WSPRO_COMMANDS",
        L"WSPROIMPORTPIPES",
        L"WSPROIMPORTPIPES",
        ACRX_CMD_MODAL,
        importWSProPipes);
}

/* ============================================================
   Load Nodes from CSV
   ============================================================ */
static bool loadNodesFromCSV(
    const ACHAR* filePath,
    std::map<AcString, AcGePoint3d>& nodeMap)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        return false;

    std::string line;
    std::getline(file, line); // header

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::vector<std::string> cols;
        std::string col;

        while (std::getline(ss, col, ','))
            cols.push_back(col);

        if (cols.size() < 4)
            continue;

        try
        {
            AcString nodeId(cols[0].c_str());
            double x = std::stod(cols[1]);
            double y = std::stod(cols[2]);
            double z = std::stod(cols[3]);

            nodeMap[nodeId] = AcGePoint3d(x, y, z);
        }
        catch (...) {}
    }

    file.close();
    return !nodeMap.empty();
}

/* ============================================================
   WSPROIMPORTNODES
   Creates 0.1m diameter polyline circles
   ============================================================ */
void importWSProNodes()
{
    resbuf rbNode;
    if (acedGetFileD(L"Select WS Pro NODE CSV", nullptr, L"csv", 0, &rbNode) != RTNORM)
        return;

    std::map<AcString, AcGePoint3d> nodeMap;
    if (!loadNodesFromCSV(rbNode.resval.rstring, nodeMap))
    {
        acutPrintf(L"\nFailed to load nodes.");
        return;
    }

    AcDbDatabase* db = acdbHostApplicationServices()->workingDatabase();
    AcDbBlockTable* bt;
    AcDbBlockTableRecord* ms;

    db->getBlockTable(bt, AcDb::kForRead);
    bt->getAt(ACDB_MODEL_SPACE, ms, AcDb::kForWrite);

    const double radius = 0.05;      // 0.1m diameter
    const int segments = 16;         // smoothness

    int created = 0;

    for (const auto& kv : nodeMap)
    {
        AcDbPolyline* pl = new AcDbPolyline(segments);

        for (int i = 0; i < segments; ++i)
        {
            double angle = (2.0 * 3.141592653589793 * i) / segments;

            double x = kv.second.x + radius * cos(angle);
            double y = kv.second.y + radius * sin(angle);

            pl->addVertexAt(i, AcGePoint2d(x, y));
        }

        pl->setClosed(true);

        ms->appendAcDbEntity(pl);
        pl->close();

        created++;
    }

    ms->close();
    bt->close();

    acutPrintf(L"\nNodes imported as 0.1m diameter circles: %d", created);
}

/* ============================================================
   WSPROIMPORTPIPES
   Creates native lines between nodes
   ============================================================ */
void importWSProPipes()
{
    resbuf rbNode;
    if (acedGetFileD(L"Select WS Pro NODE CSV", nullptr, L"csv", 0, &rbNode) != RTNORM)
        return;

    std::map<AcString, AcGePoint3d> nodeMap;
    if (!loadNodesFromCSV(rbNode.resval.rstring, nodeMap))
    {
        acutPrintf(L"\nFailed to load nodes.");
        return;
    }

    AcDbDatabase* db = acdbHostApplicationServices()->workingDatabase();
    AcDbBlockTable* bt;
    AcDbBlockTableRecord* ms;

    db->getBlockTable(bt, AcDb::kForRead);
    bt->getAt(ACDB_MODEL_SPACE, ms, AcDb::kForWrite);

    /* ==============================
       STEP 1: Create Node Circles
       ============================== */
    const double radius = 0.05;  // 0.1m diameter
    const int segments = 16;

    for (const auto& kv : nodeMap)
    {
        AcDbPolyline* pl = new AcDbPolyline(segments);

        for (int i = 0; i < segments; ++i)
        {
            double angle = (2.0 * 3.141592653589793 * i) / segments;

            double x = kv.second.x + radius * cos(angle);
            double y = kv.second.y + radius * sin(angle);

            pl->addVertexAt(i, AcGePoint2d(x, y));
        }

        pl->setClosed(true);

        ms->appendAcDbEntity(pl);
        pl->close();
    }

    /* ==============================
       STEP 2: Select Pipe CSV
       ============================== */
    resbuf rbPipe;
    if (acedGetFileD(L"Select WS Pro PIPE CSV", nullptr, L"csv", 0, &rbPipe) != RTNORM)
    {
        ms->close();
        bt->close();
        return;
    }

    std::ifstream file(rbPipe.resval.rstring);
    if (!file.is_open())
    {
        ms->close();
        bt->close();
        return;
    }

    std::string line;
    std::getline(file, line); // header

    int created = 0, skipped = 0;

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::vector<std::string> cols;
        std::string col;

        while (std::getline(ss, col, ','))
            cols.push_back(col);

        if (cols.size() < 5)
        {
            skipped++;
            continue;
        }

        try
        {
            AcString us(cols[3].c_str());
            AcString ds(cols[4].c_str());

            if (!nodeMap.count(us) || !nodeMap.count(ds))
            {
                skipped++;
                continue;
            }

            AcDbLine* pipe = new AcDbLine(
                nodeMap[us],
                nodeMap[ds]);

            ms->appendAcDbEntity(pipe);
            pipe->close();

            created++;
        }
        catch (...)
        {
            skipped++;
        }
    }

    file.close();
    ms->close();
    bt->close();

    acutPrintf(L"\nNodes + Pipes imported. Pipes: %d, Skipped: %d",
        created, skipped);
}
