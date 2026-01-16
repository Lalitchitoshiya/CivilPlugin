
#include "aced.h"
#include "acedads.h"     // resbuf, ADS support
#include "dbsymtb.h"
#include "dbapserv.h"
#include "acutads.h"

// STL
#include <fstream>
#include <sstream>
#include <vector>

// Custom entities
#include "WSProNodeEntity.h"
#include "WSProPipeEntity.h"

#ifndef RTNORM
#define RTNORM 5100
#endif

/* ============================================================
   Forward declarations
   ============================================================ */
void createWSProNode();
void createWSProPipe();
void importPipesFromCSV();

/* ============================================================
   Command registration
   Called from EntryPoint.cpp
   ============================================================ */
void initCommands()
{
    acedRegCmds->addCommand(
        L"WSPRO_COMMANDS",
        L"WSPRONODE",
        L"WSPRONODE",
        ACRX_CMD_MODAL,
        createWSProNode);

    acedRegCmds->addCommand(
        L"WSPRO_COMMANDS",
        L"WSPROPIPE",
        L"WSPROPIPE",
        ACRX_CMD_MODAL,
        createWSProPipe);

    acedRegCmds->addCommand(
        L"WSPRO_COMMANDS",
        L"WSPROIMPORTPIPES",
        L"WSPROIMPORTPIPES",
        ACRX_CMD_MODAL,
        importPipesFromCSV);
}

/* ============================================================
   WSPRONODE
   ============================================================ */
void createWSProNode()
{
    ads_point pt;
    if (acedGetPoint(nullptr, L"\nSpecify WS Pro node location: ", pt) != RTNORM)
        return;

    ACHAR type[32];
    if (acedGetString(Adesk::kFalse,
        L"\nEnter node type [Valve/Junction]: ",
        type) != RTNORM)
        return;

    double dia = 0.0;
    if (acedGetReal(L"\nEnter diameter: ", &dia) != RTNORM || dia <= 0.0)
        return;

    WSProNodeEntity* node = new WSProNodeEntity();
    node->setNodeId(L"NODE");
    node->setNodeType(type);
    node->setDiameter(dia);
    node->setPosition(AcGePoint3d(pt[0], pt[1], pt[2]));

    AcDbBlockTable* bt = nullptr;
    acdbHostApplicationServices()
        ->workingDatabase()
        ->getBlockTable(bt, AcDb::kForRead);

    AcDbBlockTableRecord* ms = nullptr;
    bt->getAt(ACDB_MODEL_SPACE, ms, AcDb::kForWrite);

    ms->appendAcDbEntity(node);

    node->close();
    ms->close();
    bt->close();
}

/* ============================================================
   WSPROPIPE
   ============================================================ */
void createWSProPipe()
{
    ads_point p1, p2;

    if (acedGetPoint(nullptr, L"\nSpecify pipe start point: ", p1) != RTNORM)
        return;

    if (acedGetPoint(p1, L"\nSpecify pipe end point: ", p2) != RTNORM)
        return;

    double dia = 0.0;
    if (acedGetReal(L"\nEnter pipe diameter: ", &dia) != RTNORM || dia <= 0.0)
        return;

    WSProPipeEntity* pipe = new WSProPipeEntity();
    pipe->setPipeId(L"PIPE");
    pipe->setStartPoint(AcGePoint3d(p1[0], p1[1], p1[2]));
    pipe->setEndPoint(AcGePoint3d(p2[0], p2[1], p2[2]));
    pipe->setDiameter(dia);

    AcDbBlockTable* bt = nullptr;
    acdbHostApplicationServices()
        ->workingDatabase()
        ->getBlockTable(bt, AcDb::kForRead);

    AcDbBlockTableRecord* ms = nullptr;
    bt->getAt(ACDB_MODEL_SPACE, ms, AcDb::kForWrite);

    ms->appendAcDbEntity(pipe);

    pipe->close();
    ms->close();
    bt->close();
}

/* ============================================================
   WSPROIMPORTPIPES
   CSV format:
   PipeID,X1,Y1,Z1,X2,Y2,Z2,Diameter
   ============================================================ */
void importPipesFromCSV()
{
    resbuf rb;

    if (acedGetFileD(
        L"Select WS Pro Pipe CSV",
        nullptr,
        L"csv",
        0,
        &rb) != RTNORM || rb.resval.rstring == nullptr)
    {
        acutPrintf(L"\nCommand cancelled.");
        return;
    }

    const ACHAR* filePath = rb.resval.rstring;

    std::ifstream file(filePath);
    if (!file.is_open())
    {
        acutPrintf(L"\nUnable to open CSV file.");
        return;
    }

    AcDbBlockTable* bt = nullptr;
    acdbHostApplicationServices()
        ->workingDatabase()
        ->getBlockTable(bt, AcDb::kForRead);

    AcDbBlockTableRecord* ms = nullptr;
    bt->getAt(ACDB_MODEL_SPACE, ms, AcDb::kForWrite);

    std::string line;
    int count = 0;

    // Skip header
    std::getline(file, line);

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> cols;

        while (std::getline(ss, token, ','))
            cols.push_back(token);

        if (cols.size() < 8)
            continue;

        try
        {
            AcString pipeId(cols[0].c_str());

            double x1 = std::stod(cols[1]);
            double y1 = std::stod(cols[2]);
            double z1 = std::stod(cols[3]);
            double x2 = std::stod(cols[4]);
            double y2 = std::stod(cols[5]);
            double z2 = std::stod(cols[6]);
            double dia = std::stod(cols[7]);

            WSProPipeEntity* pipe = new WSProPipeEntity();
            pipe->setPipeId(pipeId);
            pipe->setStartPoint(AcGePoint3d(x1, y1, z1));
            pipe->setEndPoint(AcGePoint3d(x2, y2, z2));
            pipe->setDiameter(dia);

            ms->appendAcDbEntity(pipe);
            pipe->close();

            ++count;
        }
        catch (...)
        {
            // Skip invalid rows
        }
    }

    file.close();
    ms->close();
    bt->close();

    acutPrintf(L"\nImport completed: %d pipes created.", count);
}
