#include "aced.h"
#include "acedads.h"
#include "dbsymtb.h"
#include "dbapserv.h"
#include "acutads.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <map>

// Custom entities
#include "WSProNodeEntity.h"
#include "WSProPipeEntity.h"

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
   Helper: Load nodes from CSV (NO entities created)
   Expected CSV:
   NODE_ID,X,Y,Z,...
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
        catch (...)
        {
            continue;
        }
    }

    file.close();
    return !nodeMap.empty();
}

/* ============================================================
   WSPROIMPORTNODES
   CSV: NODE_ID,X,Y,Z,...
   (UNCHANGED – creates node entities)
   ============================================================ */
void importWSProNodes()
{
    resbuf rb;
    if (acedGetFileD(L"Select Node CSV", nullptr, L"csv", 0, &rb) != RTNORM)
        return;

    std::ifstream file(rb.resval.rstring);
    if (!file.is_open())
        return;

    std::string line;
    std::getline(file, line); // header

    AcDbBlockTable* bt;
    AcDbBlockTableRecord* ms;

    acdbHostApplicationServices()->workingDatabase()
        ->getBlockTable(bt, AcDb::kForRead);
    bt->getAt(ACDB_MODEL_SPACE, ms, AcDb::kForWrite);

    int created = 0, skipped = 0;

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::vector<std::string> cols;
        std::string col;

        while (std::getline(ss, col, ','))
            cols.push_back(col);

        if (cols.size() < 4)
        {
            skipped++;
            continue;
        }

        try
        {
            WSProNodeEntity* node = new WSProNodeEntity();
            node->setNodeId(AcString(cols[0].c_str()));
            node->setPosition(
                AcGePoint3d(
                    std::stod(cols[1]),
                    std::stod(cols[2]),
                    std::stod(cols[3])));

            ms->appendAcDbEntity(node);
            node->close();
            created++;
        }
        catch (...)
        {
            skipped++;
        }
    }

    ms->close();
    bt->close();
    file.close();

    acutPrintf(
        L"\nNode import completed. Created: %d, Skipped: %d",
        created, skipped);
}

/* ============================================================
   WSPROIMPORTPIPES (INDEPENDENT)
   Uses TWO CSV files:
   1) Node CSV → coordinates
   2) Pipe CSV → connectivity

   Pipe CSV (WS Pro):
   ASSET_ID,DIAM_MM,MATERIAL,US_NODE,DS_NODE,UNIT_SYS
   ============================================================ */
void importWSProPipes()
{
    /* --------------------------------------------------------
       STEP 1: Select NODE CSV (for coordinates)
       -------------------------------------------------------- */
    resbuf rbNode;
    if (acedGetFileD(
        L"Select WS Pro NODE CSV",
        nullptr,
        L"csv",
        0,
        &rbNode) != RTNORM)
    {
        acutPrintf(L"\nNode CSV selection cancelled.");
        return;
    }

    std::map<AcString, AcGePoint3d> nodeMap;
    if (!loadNodesFromCSV(rbNode.resval.rstring, nodeMap))
    {
        acutPrintf(L"\nFailed to load nodes from CSV.");
        return;
    }

    acutPrintf(L"\nLoaded %d nodes from CSV.", nodeMap.size());

    /* --------------------------------------------------------
       STEP 2: Select PIPE CSV
       -------------------------------------------------------- */
    resbuf rbPipe;
    if (acedGetFileD(
        L"Select WS Pro PIPE CSV",
        nullptr,
        L"csv",
        0,
        &rbPipe) != RTNORM)
    {
        acutPrintf(L"\nPipe CSV selection cancelled.");
        return;
    }

    std::ifstream file(rbPipe.resval.rstring);
    if (!file.is_open())
    {
        acutPrintf(L"\nFailed to open pipe CSV.");
        return;
    }

    std::string line;
    std::getline(file, line); // header

    /* --------------------------------------------------------
       STEP 3: Open ModelSpace for pipe creation
       -------------------------------------------------------- */
    AcDbBlockTable* bt;
    AcDbBlockTableRecord* ms;

    acdbHostApplicationServices()->workingDatabase()
        ->getBlockTable(bt, AcDb::kForRead);
    bt->getAt(ACDB_MODEL_SPACE, ms, AcDb::kForWrite);

    int created = 0, skipped = 0;

    /* --------------------------------------------------------
       STEP 4: Create pipes
       Column mapping (fixed for WS Pro):
       0 = ASSET_ID
       1 = DIAM_MM
       3 = US_NODE
       4 = DS_NODE
       -------------------------------------------------------- */
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

        AcString pipeId(cols[0].c_str());

        double dia = 0.0;
        try { dia = std::stod(cols[1]); }
        catch (...) {}

        AcString us(cols[3].c_str());
        AcString ds(cols[4].c_str());

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

        // Flatten Z (current working behavior)
        AcGePoint3d p1 = nodeMap[us];
        AcGePoint3d p2 = nodeMap[ds];
        p1.z = 0.0;
        p2.z = 0.0;

        WSProPipeEntity* pipe = new WSProPipeEntity();
        pipe->setPipeId(pipeId);
        pipe->setStartPoint(p1);
        pipe->setEndPoint(p2);
        pipe->setDiameter(dia);

        ms->appendAcDbEntity(pipe);
        pipe->close();
        created++;
    }

    ms->close();
    bt->close();
    file.close();

    acutPrintf(
        L"\nPipe import completed. Created: %d, Skipped: %d",
        created, skipped);
}
