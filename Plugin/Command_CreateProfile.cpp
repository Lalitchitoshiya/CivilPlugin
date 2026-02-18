#include "aced.h"
#include "dbsymtb.h"
#include "dbapserv.h"
#include "dbents.h"
//#include "dbline.h"
#include "Global_Data.h"
#include <cmath>

void cmdCreateProfile()
{
    if (g_PipeList.empty())
    {
        acutPrintf(L"\nNo pipes loaded. Run WSPROIMPORTPIPES first.");
        return;
    }

    // ── 1. Ask user where to place profile origin ──────────────────────
    AcGePoint3d origin(0, 0, 0);
    // Optional: use acedGetPoint to let user pick insertion point
    // For now place it at fixed offset from drawing origin
    // You can replace 0,0 with a picked point later
    double originX = 0.0;
    double originY = -50.0; // place profile 50 units below plan view

    // ── 2. Build profile geometry ──────────────────────────────────────
    AcDbDatabase* db = acdbHostApplicationServices()->workingDatabase();
    AcDbBlockTable* bt = nullptr;
    AcDbBlockTableRecord* ms = nullptr;
    db->getBlockTable(bt, AcDb::kForRead);
    bt->getAt(ACDB_MODEL_SPACE, ms, AcDb::kForWrite);
    bt->close();

    double chainage = 0.0;

    for (const auto& pipe : g_PipeList)
    {
        // Profile start point: (chainage, elevation) mapped to 2D in model space
        double x1 = originX + chainage;
        double y1 = originY + pipe.usInvert;

        double x2 = originX + chainage + pipe.length;
        double y2 = originY + pipe.dsInvert;

        // Draw the pipe invert as a 3D line (Z=0, lying flat as profile)
        AcDbLine* pLine = new AcDbLine(
            AcGePoint3d(x1, y1, 0),
            AcGePoint3d(x2, y2, 0)
        );
        pLine->setColorIndex(3); // green = invert profile
        ms->appendAcDbEntity(pLine);
        pLine->close();

        // Draw vertical markers at each node
        AcDbLine* pTick = new AcDbLine(
            AcGePoint3d(x1, y1 - 0.5, 0),
            AcGePoint3d(x1, y1 + 0.5, 0)
        );
        pTick->setColorIndex(1); // red = node ticks
        ms->appendAcDbEntity(pTick);
        pTick->close();

        chainage += pipe.length;
    }

    // Final closing tick at last node
    double lastX = originX + chainage;
    double lastY = originY + g_PipeList.back().dsInvert;
    AcDbLine* pLastTick = new AcDbLine(
        AcGePoint3d(lastX, lastY - 0.5, 0),
        AcGePoint3d(lastX, lastY + 0.5, 0)
    );
    pLastTick->setColorIndex(1);
    ms->appendAcDbEntity(pLastTick);
    pLastTick->close();

    ms->close();
    acutPrintf(L"\nProfile created. %d pipes drawn.", (int)g_PipeList.size());
}