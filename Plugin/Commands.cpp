#include "aced.h"
#include "acedads.h"
#include "dbsymtb.h"
#include "dbapserv.h"
#include "acutads.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>

// Custom entities
#include "WSProNodeEntity.h"
#include "WSProPipeEntity.h"

#ifndef RTNORM
#define RTNORM 5100
#endif

/* ============================================================
   Utility
   ============================================================ */
static std::string trim(const std::string& s)
{
    size_t b = s.find_first_not_of(" \t\r\n\"");
    size_t e = s.find_last_not_of(" \t\r\n\"");
    if (b == std::string::npos || e == std::string::npos)
        return "";
    return s.substr(b, e - b + 1);
}

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
   WSPROIMPORTNODES
   CSV (header REQUIRED, order flexible):
   NodeID,X,Y,Z,(optional NodeType)
   ============================================================ */
void importWSProNodes()
{
    resbuf rb;
    if (acedGetFileD(
        L"Select Node CSV",
        nullptr,
        L"csv",
        0,
        &rb) != RTNORM)
    {
        acutPrintf(L"\nCommand cancelled.");
        return;
    }

    std::ifstream file(rb.resval.rstring);
    if (!file.is_open())
    {
        acutPrintf(L"\nUnable to open node CSV.");
        return;
    }

    AcDbBlockTable* bt = nullptr;
    acdbHostApplicationServices()
        ->workingDatabase()
        ->getBlockTable(bt, AcDb::kForRead);

    AcDbBlockTableRecord* ms = nullptr;
    bt->getAt(ACDB_MODEL_SPACE, ms, AcDb::kForWrite);

    std::string line;
    std::getline(file, line); // header

    int created = 0;
    int skipped = 0;

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::vector<std::string> c;
        std::string col;

        while (std::getline(ss, col, ','))
            c.push_back(trim(col));

        if (c.size() < 4)
        {
            skipped++;
            continue;
        }

        try
        {
            WSProNodeEntity* node = new WSProNodeEntity();
            node->setNodeId(AcString(c[0].c_str()));
            node->setPosition(
                AcGePoint3d(
                    std::stod(c[1]),
                    std::stod(c[2]),
                    std::stod(c[3])));

            // NodeType is OPTIONAL
            if (c.size() > 4 && !c[4].empty())
                node->setNodeType(AcString(c[4].c_str()));

            ms->appendAcDbEntity(node);
            node->close();
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

    acutPrintf(
        L"\nNode import completed. Created: %d, Skipped: %d",
        created, skipped);
}

/* ============================================================
   WSPROIMPORTPIPES
   CSV (header REQUIRED):
   PipeID,Diameter,US_Node,DS_Node
   ============================================================ */


void importWSProPipes()
{
    // ----------------------------------------------------
    // 1. Collect WSPro nodes from drawing
    // ----------------------------------------------------
    std::map<AcString, AcGePoint3d> nodeMap;

    AcDbBlockTable* bt = nullptr;
    acdbHostApplicationServices()->workingDatabase()
        ->getBlockTable(bt, AcDb::kForRead);

    AcDbBlockTableRecord* ms = nullptr;
    bt->getAt(ACDB_MODEL_SPACE, ms, AcDb::kForRead);

    AcDbBlockTableRecordIterator* it = nullptr;
    ms->newIterator(it);

    for (; !it->done(); it->step())
    {
        AcDbEntity* ent = nullptr;
        it->getEntity(ent, AcDb::kForRead);

        WSProNodeEntity* node = WSProNodeEntity::cast(ent);
        if (node)
        {
            nodeMap[node->nodeId()] = node->position();
        }

        ent->close();
    }

    delete it;
    ms->close();
    bt->close();

    if (nodeMap.empty())
    {
        acutPrintf(L"\nNo WSPro nodes found.");
        return;
    }

    // ----------------------------------------------------
    // 2. Select pipe CSV
    // ----------------------------------------------------
    resbuf rb;
    if (acedGetFileD(L"Select WS Pro Pipe CSV", nullptr, L"csv", 0, &rb) != RTNORM)
        return;

    std::ifstream file(rb.resval.rstring);
    if (!file.is_open())
    {
        acutPrintf(L"\nFailed to open pipe CSV.");
        return;
    }

    // ----------------------------------------------------
    // 3. Skip header line (CONTENT IGNORED)
    // ----------------------------------------------------
    std::string line;
    std::getline(file, line); // header ignored completely

    // ----------------------------------------------------
    // 4. Create pipes
    // Column mapping based on YOUR CSV:
    // 0 = ASSET_ID
    // 1 = DIAM_MM
    // 3 = US_NODE
    // 4 = DS_NODE
    // ----------------------------------------------------
    acdbHostApplicationServices()->workingDatabase()
        ->getBlockTable(bt, AcDb::kForRead);

    bt->getAt(ACDB_MODEL_SPACE, ms, AcDb::kForWrite);

    int created = 0;
    int skipped = 0;

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::vector<std::string> c;
        std::string col;

        while (std::getline(ss, col, ','))
            c.push_back(col);

        if (c.size() < 5)
        {
            skipped++;
            continue;
        }

        AcString pipeId(c[0].c_str());   // ASSET_ID

        double diameter = 0.0;
        try { diameter = std::stod(c[1]); }
        catch (...) {}

        AcString us(c[3].c_str());       // US_NODE
        AcString ds(c[4].c_str());       // DS_NODE

        if (!nodeMap.count(us) || !nodeMap.count(ds))
        {
            acutPrintf(
                L"\nSkipping pipe %ls (US=%ls DS=%ls)",
                pipeId.constPtr(),
                us.constPtr(),
                ds.constPtr());

            skipped++;
            continue;
        }

        WSProPipeEntity* pipe = new WSProPipeEntity();
        pipe->setPipeId(pipeId);
        pipe->setStartPoint(nodeMap[us]);
        pipe->setEndPoint(nodeMap[ds]);
        pipe->setDiameter(diameter);

        ms->appendAcDbEntity(pipe);
        pipe->close();

        created++;
    }

    file.close();
    ms->close();
    bt->close();

    acutPrintf(
        L"\nPipe import completed. Created: %d, Skipped: %d",
        created, skipped);
}

