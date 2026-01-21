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
   Helper: Load nodes from CSV (NO entities created here)
   CSV: NODE_ID,X,Y,Z,...
   ============================================================ */
static bool loadNodesFromCSV(
	const ACHAR* filePath,                              //file path from dialog
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
   WSPROIMPORTNODES (UNCHANGED)
   ============================================================ */
void importWSProNodes()
{
    resbuf rb;
	if (acedGetFileD(L"Select Node CSV", nullptr, L"csv", 0, &rb) != RTNORM)      // dialouge box to select file
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

    while (std::getline(file, line))            //processing each data row
    {
        std::stringstream ss(line);
        std::vector<std::string> cols;           //SPLIT CSV ROW INTO COLUMNS
        std::string col;

		while (std::getline(ss, col, ','))        
            cols.push_back(col);

        if (cols.size() < 4)                //VALIDATE REQUIRED DATA
        {
            skipped++;
            continue;
        }

        try
        {
            WSProNodeEntity* node = new WSProNodeEntity();     //CREATE NODE ENTITY (CORE LOGIC)
            node->setNodeId(AcString(cols[0].c_str()));
            node->setPosition(
                AcGePoint3d(
                    std::stod(cols[1]),
                    std::stod(cols[2]),
                    std::stod(cols[3])));

            ms->appendAcDbEntity(node);                     //APPEND NODE TO MODELSPACE
            node->close();
            created++;
		}                                                    //Error handling
        catch (...)
        {
            skipped++;
        }
    }

    ms->close();                                                //CLEANUP RESOURCES
    bt->close();
    file.close();

    acutPrintf(
        L"\nNode import completed. Created: %d, Skipped: %d",      //USER FEEDBACK
        created, skipped);
}

/* ============================================================
   WSPROIMPORTPIPES (INDEPENDENT + 3D SAFE)
   Uses:
   1) Node CSV → coordinates (WITH Z)
   2) Pipe CSV → connectivity

   Pipe CSV:
   ASSET_ID,DIAM_MM,MATERIAL,US_NODE,DS_NODE,UNIT_SYS
   ============================================================ */
void importWSProPipes()
{
    /* --------------------------------------------------------
       STEP 1: Select NODE CSV
       -------------------------------------------------------- */
    resbuf rbNode;                                                  //Select NODE CSV (Source of Geometry)
    if (acedGetFileD(
        L"Select WS Pro NODE CSV",
        nullptr,
        L"csv",
        0,
        &rbNode) != RTNORM)
        return;

	std::map<AcString, AcGePoint3d> nodeMap;                       
    if (!loadNodesFromCSV(rbNode.resval.rstring, nodeMap))              //call the load function to read nodes from CSV & Builds an in-memory lookup table:
    {
        acutPrintf(L"\nFailed to load nodes from CSV.");
        return;
    }

    acutPrintf(L"\nLoaded %d nodes from CSV.", nodeMap.size());

    /* --------------------------------------------------------
       STEP 2: Create NODE ENTITIES (visualization)
       -------------------------------------------------------- */
    AcDbBlockTable* bt;
    AcDbBlockTableRecord* ms;

    acdbHostApplicationServices()->workingDatabase()
        ->getBlockTable(bt, AcDb::kForRead);
    bt->getAt(ACDB_MODEL_SPACE, ms, AcDb::kForWrite);

    int nodesCreated = 0;

    for (const auto& kv : nodeMap)
    {
        WSProNodeEntity* node = new WSProNodeEntity();
        node->setNodeId(kv.first);
        node->setPosition(kv.second); // Z preserved

        ms->appendAcDbEntity(node);
        node->close();
        nodesCreated++;
    }

    ms->close();
    bt->close();

    acutPrintf(L"\nNodes visualized: %d", nodesCreated);

    /* --------------------------------------------------------
       STEP 3: Select PIPE CSV
       -------------------------------------------------------- */
    resbuf rbPipe;
	if (acedGetFileD(                              //dialouge box to select file
        L"Select WS Pro PIPE CSV",
        nullptr,
        L"csv",
        0,
        &rbPipe) != RTNORM)
        return;

    std::ifstream file(rbPipe.resval.rstring);          // file path stored here
    if (!file.is_open())
        return;

    std::string line;
    std::getline(file, line); // read csv file line by line

    /* --------------------------------------------------------
       STEP 4: Create pipes (TRUE 3D)
       -------------------------------------------------------- */
    acdbHostApplicationServices()->workingDatabase()     //Get the current drawing database
        ->getBlockTable(bt, AcDb::kForRead);                //Open the Block Table (read-only)
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

        AcString pipeId(cols[0].c_str());
        double dia = 0.0;
        try { dia = std::stod(cols[1]); }
        catch (...) {}

        AcString us(cols[3].c_str());
        AcString ds(cols[4].c_str());

        if (!nodeMap.count(us) || !nodeMap.count(ds))
        {
            skipped++;
            continue;
        }

        WSProPipeEntity* pipe = new WSProPipeEntity();
        pipe->setPipeId(pipeId);
        pipe->setStartPoint(nodeMap[us]); // Z preserved
        pipe->setEndPoint(nodeMap[ds]);   // Z preserved
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
