//#include "aced.h"
//#include "dbsymtb.h"
//#include "WSProNodeEntity.h"
//#include "acdb.h" 
//#include "dbapserv.h"
//
//// Add this line to define RTNORM if not already defined
//#ifndef RTNORM
//#define RTNORM 5100
//#endif
//
//void createWSProNode()
//{
//    WSProNodeEntity* pNode = new WSProNodeEntity();
//    pNode->setNodeId(L"WSNODE_101");
//    pNode->setNodeType(L"Valve");
//    pNode->setDiameter(300.0);
//    pNode->setPosition(AcGePoint3d(100, 100, 0));
//
//    AcDbBlockTable* pBT;
//    acdbHostApplicationServices()->workingDatabase()
//        ->getBlockTable(pBT, AcDb::kForRead);
//
//    AcDbBlockTableRecord* pMS;
//    pBT->getAt(ACDB_MODEL_SPACE, pMS, AcDb::kForWrite);
//
//    AcDbObjectId id;
//    pMS->appendAcDbEntity(id, pNode);
//
//    pNode->close();
//    pMS->close();
//    pBT->close();
//}
//
//void initCommands()
//{
//    acedRegCmds->addCommand(
//        L"WS_PRO_CMDS",
//        L"WSPRONODE",
//        L"WSPRONODE",
//        ACRX_CMD_MODAL,
//        createWSProNode
//    );
//}


#ifndef RTNORM
#define RTNORM 5100
#endif

#include "aced.h"
#include "dbsymtb.h"
#include "dbapserv.h"
#include "acdb.h"
#include "WSProNodeEntity.h"

void createWSProNode()
{
    // -------------------------------
    // Step A1 — Ask user for location
    // -------------------------------
    ads_point pt;
    if (acedGetPoint(nullptr, L"\nSpecify WS Pro node location: ", pt) != RTNORM)
        return;

    // -------------------------------
    // Step A2 — Ask user for node type
    // -------------------------------
    ACHAR type[32];
    if (acedGetString(Adesk::kFalse,
        L"\nEnter node type [Valve/Junction]: ",
        type) != RTNORM)
        return;

    // -------------------------------
    // Step A3 — Ask user for diameter
    // -------------------------------
    double dia = 0.0;
    if (acedGetReal(L"\nEnter diameter: ", &dia) != RTNORM || dia <= 0.0)
        return;

    // -------------------------------
    // Create WS Pro Node Entity
    // -------------------------------
    WSProNodeEntity* pNode = new WSProNodeEntity();

    pNode->setNodeId(L"USER_NODE");   // placeholder ID
    pNode->setNodeType(type);
    pNode->setDiameter(dia);
    pNode->setPosition(AcGePoint3d(pt[0], pt[1], pt[2]));

    // -------------------------------
    // Append to ModelSpace
    // -------------------------------
    AcDbBlockTable* pBT = nullptr;
    acdbHostApplicationServices()
        ->workingDatabase()
        ->getBlockTable(pBT, AcDb::kForRead);

    AcDbBlockTableRecord* pMS = nullptr;
    pBT->getAt(ACDB_MODEL_SPACE, pMS, AcDb::kForWrite);

    AcDbObjectId id;
    pMS->appendAcDbEntity(id, pNode);

    pNode->close();
    pMS->close();
    pBT->close();
}

void initCommands()
{
    acedRegCmds->addCommand(
        L"WSPRO_COMMANDS",
        L"WSPRONODE",
        L"WSPRONODE",
        ACRX_CMD_MODAL,
        createWSProNode
    );
}
