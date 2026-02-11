#include "aced.h"
#include "acedads.h"
#include "dbsymtb.h"
#include "dbapserv.h"
#include "acutads.h"
#include "dbents.h"
//#include "dbline.h"
//#include "dbregapp.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <map>

#ifndef RTNORM
#define RTNORM 5100
#endif

#ifndef RTNONE
#define RTNONE 5000
#endif

#define WSPRO_APP L"WSPRO"

/* ============================================================
   Forward declarations
   ============================================================ */
void importWSProPipes();
void registerAppName(const wchar_t* appName);

/* ============================================================
   Command registration
   ============================================================ */
void initCommands()
{
    acedRegCmds->addCommand(
        L"WSPRO_COMMANDS",
        L"WSPROIMPORTPIPES",
        L"WSPROIMPORTPIPES",
        ACRX_CMD_MODAL,
        importWSProPipes);
}

/* ============================================================
   Register RegApp (Required for XData)
   ============================================================ */
void registerAppName(const wchar_t* appName)
{
    AcDbDatabase* db = acdbHostApplicationServices()->workingDatabase();

    AcDbRegAppTable* regTable;
    db->getRegAppTable(regTable, AcDb::kForRead);

    if (!regTable->has(appName))
    {
        regTable->upgradeOpen();

        AcDbRegAppTableRecord* regRecord =
            new AcDbRegAppTableRecord;
        regRecord->setName(appName);

        regTable->add(regRecord);
        regRecord->close();
    }

    regTable->close();
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
   IMPORT PIPE COMMAND
   ============================================================ */
void importWSProPipes()
{
    // STEP 1: Register XData app
    registerAppName(WSPRO_APP);

    // STEP 2: Select NODE CSV
    resbuf rbNode;
    if (acedGetFileD(L"Select WS Pro NODE CSV", nullptr, L"csv", 0, &rbNode) != RTNORM)
        return;

    std::map<AcString, AcGePoint3d> nodeMap;
    if (!loadNodesFromCSV(rbNode.resval.rstring, nodeMap))
    {
        acutPrintf(L"\nFailed to load nodes.");
        return;
    }

    // STEP 3: Select PIPE CSV
    resbuf rbPipe;
    if (acedGetFileD(L"Select WS Pro PIPE CSV", nullptr, L"csv", 0, &rbPipe) != RTNORM)
        return;

    std::ifstream file(rbPipe.resval.rstring);
    if (!file.is_open())
        return;

    std::string line;
    std::getline(file, line); // header

    AcDbDatabase* db = acdbHostApplicationServices()->workingDatabase();
    AcDbBlockTable* bt;
    AcDbBlockTableRecord* ms;

    db->getBlockTable(bt, AcDb::kForRead);
    bt->getAt(ACDB_MODEL_SPACE, ms, AcDb::kForWrite);

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
            AcString pipeId(cols[0].c_str());
            double dia = std::stod(cols[1]);
            AcString us(cols[3].c_str());
            AcString ds(cols[4].c_str());

            if (!nodeMap.count(us) || !nodeMap.count(ds))
            {
                skipped++;
                continue;
            }

            // Create native AutoCAD Line
            AcDbLine* pipe = new AcDbLine(
                nodeMap[us],
                nodeMap[ds]);

            // Attach XData
            resbuf* rb = acutBuildList(
                AcDb::kDxfRegAppName, WSPRO_APP,
                AcDb::kDxfXdAsciiString, pipeId.kwszPtr(),
                AcDb::kDxfXdReal, dia,
                0);

            pipe->setXData(rb);
            acutRelRb(rb);

            ms->appendAcDbEntity(pipe);
            pipe->close();

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

    acutPrintf(L"\nPipe import complete. Created: %d, Skipped: %d",
        created, skipped);
}
