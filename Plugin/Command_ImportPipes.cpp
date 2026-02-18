#include <Windows.h>
#include <commdlg.h>
#include <vector>
#include <string>
#include <cmath>

#include "Domain_WSPipe.h"
#include "Import_CSVParser.h"
#include "Global_Data.h"
#include "dbsymtb.h"
#include "dbents.h"
#include "dbapserv.h"
#include "gemat3d.h"
#include "gevec3d.h"
#include "gepnt3d.h"
#include "aced.h"

// ── Constants ──────────────────────────────────────────────────────────────
static const double PI = 3.14159265358979323846;
static const int    CIRCLE_SEGS = 16;    // smoothness of pipe cross-section
static const double DEFAULT_DIAM = 0.3;   // metres, fallback if CSV has no diameter

// ── Helper: build one circular ring of 3D points ──────────────────────────
//   centre  = centre of the ring
//   normal  = pipe axis direction the ring faces
//   radius  = pipe radius
//   pts     = output — CIRCLE_SEGS points around the ring
static void buildRing(
    const AcGePoint3d& centre,
    const AcGeVector3d& normal,
    double              radius,
    AcGePoint3d         pts[])
{
    AcGeVector3d n = normal;
    n.normalize();

    // Two vectors perpendicular to pipe axis
    AcGeVector3d arbitrary(0.0, 0.0, 1.0);
    if (fabs(n.dotProduct(arbitrary)) > 0.99)
        arbitrary = AcGeVector3d(0.0, 1.0, 0.0);

    AcGeVector3d u = n.crossProduct(arbitrary);  u.normalize();
    AcGeVector3d v = n.crossProduct(u);           v.normalize();

    for (int k = 0; k < CIRCLE_SEGS; ++k)
    {
        double angle = (2.0 * PI * k) / CIRCLE_SEGS;
        pts[k] = centre
            + u * (radius * std::cos(angle))
            + v * (radius * std::sin(angle));
    }
}

// ── Helper: draw one closed ring as AcDb3dPolyline ────────────────────────
static void appendRing(
    AcDbBlockTableRecord* pMs,
    AcGePoint3d           pts[],
    Adesk::UInt16         colorIdx)
{
    AcDb3dPolyline* pRing = new AcDb3dPolyline();

    for (int k = 0; k < CIRCLE_SEGS; ++k)
    {
        AcDb3dPolylineVertex* pVtx = new AcDb3dPolylineVertex(pts[k]);
        pRing->appendVertex(pVtx);
        pVtx->close();
    }

    // Close back to first point
    AcDb3dPolylineVertex* pClose = new AcDb3dPolylineVertex(pts[0]);
    pRing->appendVertex(pClose);
    pClose->close();

    pRing->setColorIndex(colorIdx);
    pMs->appendAcDbEntity(pRing);
    pRing->close();
}

// ── Helper: draw one longitudinal line along the pipe surface ─────────────
static void appendLongLine(
    AcDbBlockTableRecord* pMs,
    const AcGePoint3d& p1,
    const AcGePoint3d& p2,
    Adesk::UInt16         colorIdx)
{
    AcDbLine* pLine = new AcDbLine(p1, p2);
    pLine->setColorIndex(colorIdx);
    pMs->appendAcDbEntity(pLine);
    pLine->close();
}

// ═══════════════════════════════════════════════════════════════════════════
//  cmdImportPipes
// ═══════════════════════════════════════════════════════════════════════════
void cmdImportPipes()
{
    // ── Guard ─────────────────────────────────────────────────────────
    if (g_NodeMap.empty())
    {
        acutPrintf(L"\nNo nodes found. Run WSPROIMPORTNODES first.");
        return;
    }

    // ── File picker ───────────────────────────────────────────────────
    wchar_t filePath[MAX_PATH] = { 0 };
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Select Pipe CSV File";

    if (!GetOpenFileNameW(&ofn))
    {
        acutPrintf(L"\nFile selection cancelled.");
        return;
    }

    std::wstring ws(filePath);
    std::string  path(ws.begin(), ws.end());

    // ── Parse CSV ─────────────────────────────────────────────────────
    auto rows = CSVParser::Parse(path);
    if (rows.size() <= 1)
    {
        acutPrintf(L"\nNo data found in file.");
        return;
    }

    // ── Open model space ──────────────────────────────────────────────
    AcDbDatabase* pDb = acdbHostApplicationServices()->workingDatabase();
    AcDbBlockTable* pBlockTable = nullptr;
    AcDbBlockTableRecord* pModelSpace = nullptr;

    pDb->getBlockTable(pBlockTable, AcDb::kForRead);
    pBlockTable->getAt(ACDB_MODEL_SPACE, pModelSpace, AcDb::kForWrite);
    pBlockTable->close();

    // ── Clear previous pipe list ──────────────────────────────────────
    g_PipeList.clear();

    int created = 0;
    int skipped = 0;

    // ── CSV column layout ─────────────────────────────────────────────
    //   [0] Pipe ID
    //   [1] Pipe Name      (informational)
    //   [2] Diameter (m)
    //   [3] US Node ID     (upstream  / start)
    //   [4] DS Node ID     (downstream / end)
    //
    // Adjust indices if your CSV layout differs.

    for (size_t i = 1; i < rows.size(); ++i)
    {
        if (rows[i].size() < 5) { skipped++; continue; }

        try
        {
            // ── Node lookup ───────────────────────────────────────────
            int usId = std::stoi(rows[i][3]);
            int dsId = std::stoi(rows[i][4]);

            if (!g_NodeMap.count(usId) || !g_NodeMap.count(dsId))
            {
                skipped++;
                continue;
            }

            AcGePoint3d pt1 = g_NodeMap[usId];   // start  (X, Y, Z=invert)
            AcGePoint3d pt2 = g_NodeMap[dsId];   // end    (X, Y, Z=invert)

            // ── Diameter ──────────────────────────────────────────────
            double diameter = DEFAULT_DIAM;
            if (rows[i].size() > 2 && !rows[i][2].empty())
            {
                try { diameter = std::stod(rows[i][2]); }
                catch (...) { diameter = DEFAULT_DIAM; }
            }
            double radius = diameter / 2.0;

            // ── Pipe axis ─────────────────────────────────────────────
            AcGeVector3d axis = pt2 - pt1;
            double       pipeLen = axis.length();
            if (pipeLen < 1e-6) { skipped++; continue; }

            // ── Horizontal length (for profile data) ──────────────────
            double dx = pt2.x - pt1.x;
            double dy = pt2.y - pt1.y;
            double hLen = std::sqrt(dx * dx + dy * dy);

            // ── Store in global pipe list ─────────────────────────────
            WSPipe pipe;
            pipe.id = rows[i][0];
            pipe.startNodeId = rows[i][3];
            pipe.endNodeId = rows[i][4];
            pipe.usInvert = pt1.z;
            pipe.dsInvert = pt2.z;
            pipe.length = hLen;
            g_PipeList.push_back(pipe);

            // ── Colour: cycle indices 1-6 ─────────────────────────────
            Adesk::UInt16 col = (Adesk::UInt16)(1 + (created % 6));

            // ══════════════════════════════════════════════════════════
            //  WIREFRAME CYLINDER
            //
            //  2 end-cap rings  +  CIRCLE_SEGS longitudinal lines
            //
            //      start ring            end ring
            //        ___                   ___
            //       /   \_________________/   \
            //      |     _________________     |
            //       \___/                 \___/
            //
            // ══════════════════════════════════════════════════════════

            AcGePoint3d startRing[CIRCLE_SEGS];
            AcGePoint3d endRing[CIRCLE_SEGS];

            buildRing(pt1, axis, radius, startRing);
            buildRing(pt2, axis, radius, endRing);

            // Start cap
            appendRing(pModelSpace, startRing, col);

            // End cap
            appendRing(pModelSpace, endRing, col);

            // Longitudinal lines
            for (int k = 0; k < CIRCLE_SEGS; ++k)
                appendLongLine(pModelSpace, startRing[k], endRing[k], col);

            created++;
        }
        catch (...)
        {
            skipped++;
        }
    }

    pModelSpace->close();

    acutPrintf(L"\n3D wireframe pipes created : %d", created);
    acutPrintf(L"\nSkipped                    : %d", skipped);
    acutPrintf(L"\nTip: Type  3DORBIT  to inspect the pipes in 3D.");
}