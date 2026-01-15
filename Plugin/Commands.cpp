#include "aced.h"
#include "dbsymtb.h"
#include "dbapserv.h"

#include "WSProNodeEntity.h"
#include "WSProPipeEntity.h"

#ifndef RTNORM
#define RTNORM 5100
#endif

void createWSProNode();
void createWSProPipe();

void initCommands()
{
    acedRegCmds->addCommand(
        L"WSPRO_COMMANDS",
        L"WSPRONODE",
        L"WSPRONODE",
        ACRX_CMD_MODAL,
        createWSProNode
    );

    acedRegCmds->addCommand(
        L"WSPRO_COMMANDS",
        L"WSPROPIPE",
        L"WSPROPIPE",
        ACRX_CMD_MODAL,
        createWSProPipe
    );
}

void createWSProNode()
{
    ads_point pt;
    if (acedGetPoint(nullptr, L"\nSpecify WS Pro node location: ", pt) != RTNORM)
        return;

    ACHAR type[32];
    if (acedGetString(Adesk::kFalse, L"\nEnter node type [Valve/Junction]: ", type) != RTNORM)
        return;

    double dia;
    if (acedGetReal(L"\nEnter diameter: ", &dia) != RTNORM || dia <= 0)
        return;

    WSProNodeEntity* pNode = new WSProNodeEntity();
    pNode->setNodeId(L"NODE");
    pNode->setNodeType(type);
    pNode->setDiameter(dia);
    pNode->setPosition(AcGePoint3d(pt[0], pt[1], pt[2]));

    AcDbBlockTable* pBT;
    acdbHostApplicationServices()->workingDatabase()
        ->getBlockTable(pBT, AcDb::kForRead);

    AcDbBlockTableRecord* pMS;
    pBT->getAt(ACDB_MODEL_SPACE, pMS, AcDb::kForWrite);

    pMS->appendAcDbEntity(pNode);

    pNode->close();
    pMS->close();
    pBT->close();
}

void createWSProPipe()
{
    ads_point p1, p2;
    if (acedGetPoint(nullptr, L"\nPipe start point: ", p1) != RTNORM)
        return;

    if (acedGetPoint(p1, L"\nPipe end point: ", p2) != RTNORM)
        return;

    double dia;
    if (acedGetReal(L"\nEnter pipe diameter: ", &dia) != RTNORM || dia <= 0)
        return;

    WSProPipeEntity* pPipe = new WSProPipeEntity();
    pPipe->setPipeId(L"PIPE");
    pPipe->setStartPoint(AcGePoint3d(p1[0], p1[1], p1[2]));
    pPipe->setEndPoint(AcGePoint3d(p2[0], p2[1], p2[2]));
    pPipe->setDiameter(dia);

    AcDbBlockTable* pBT;
    acdbHostApplicationServices()->workingDatabase()
        ->getBlockTable(pBT, AcDb::kForRead);

    AcDbBlockTableRecord* pMS;
    pBT->getAt(ACDB_MODEL_SPACE, pMS, AcDb::kForWrite);

    pMS->appendAcDbEntity(pPipe);

    pPipe->close();
    pMS->close();
    pBT->close();
}
