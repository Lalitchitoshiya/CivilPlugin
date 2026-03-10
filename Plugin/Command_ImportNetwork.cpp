// ===========================================================================
//  Command_ImportNetwork.cpp
//  Command: WSPROIMPORT
//
//  Reads PIPES_EXPORT.csv + PIPE_BURST_NODES.CSV (+ optional reservoirs)
//  Creates AcDbLine objects with XData for each pipe.
//  No Civil 3D SDK required. Pure ObjectARX.
//
//  CSV COLUMN LAYOUT (PIPES_EXPORT.csv):
//  [0]  US_IL        [1]  DS_IL        [2]  DIAMETER     [3]  MATERIAL
//  [4]  PRES_CLASS   [5]  MAX_PRES     [6]  MAX_VEL      [7]  SURGE_MAX
//  [8]  SURGE_MIN    [9]  US_ID        [10] DS_ID        [11] ELEVATION_US
//  [12] ELEVATION_DS [13] ROUGHNESS    [14] LENGTH       [15] US_X
//  [16] US_Y         [17] DS_X         [18] DS_Y         [19] VERTICES
//  [20] PIPE_ID      [21] SYSTEM_TYPE  [22] PIPE_STATUS  [23] LINING
//  [24] JOINT_TYPE   [25] INSTALL_YEAR [26] VELOCITY_FLAG[27] SURGE_FLAG
//  [28] CIVIL3D_PN_CLASS               [29] NOTES
//
//  NODE CSV (PIPE_BURST_NODES.CSV):
//  NODE_ID, Z_GROUND, X, Y
//
//  RESERVOIR CSV (PIPE_BURST_RESERVOIR.CSV):
//  RES_ID, TOP_WATER, MAX_HEAD, INFLOW, X, Y
// ===========================================================================

#include <Windows.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>

// ObjectARX
#include "aced.h"
#include "acedads.h"
#include "acutads.h"
#include "dbsymtb.h"
#include "dbents.h"
#include "dbapserv.h"
#include "gepnt3d.h"
#include "dbsymtb.h"
#include <adscodes.h>

// Our CSV parser
#include "Import_CSVParser.h"
#include "Utils_StringUtils.h"

// ===========================================================================
//  Constants
// ===========================================================================

static const wchar_t* WSPRO_APP = L"WSPRO";         // XData app name
static const wchar_t* LAYER_PIPES = L"WS_PIPES";      // layer for all pipes
static const wchar_t* LAYER_NODES = L"WS_NODES";      // layer for node markers

// Colour indices (AutoCAD colour numbers)
static const Adesk::UInt16 COL_PIPE_DEFAULT = 3;   // green
static const Adesk::UInt16 COL_PIPE_SURGE = 1;   // red   (SURGE_FLAG != OK)
static const Adesk::UInt16 COL_PIPE_VELOCITY = 2;   // yellow (VELOCITY_FLAG != OK)
static const Adesk::UInt16 COL_NODE = 4;   // cyan

// ===========================================================================
//  Internal data structures
// ===========================================================================

struct WSNode
{
    std::wstring id;
    double x = 0.0, y = 0.0, z = 0.0;   // z = ground level
    std::wstring type;                    // "JUNCTION", "RESERVOIR", "SOURCE"
};

struct WSPipeRow
{
    std::wstring pipeId;
    std::wstring usId, dsId;
    double usIL = 0.0, dsIL = 0.0;
    double diameter = 0.0;               // mm
    double length = 0.0;               // m
    std::wstring material;
    std::wstring presClass;
    std::wstring velocityFlag;
    std::wstring surgeFlag;
    std::wstring civil3dClass;
    std::wstring notes;
};

// ===========================================================================
//  Helpers
// ===========================================================================

// Safe string -> double, returns defaultVal if empty or invalid
static double toDouble(const std::string& s, double defaultVal = 0.0)
{
    if (s.empty()) return defaultVal;
    try { return std::stod(s); }
    catch (...) { return defaultVal; }
}

static std::wstring toWide(const std::string& s)
{
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (n <= 0) return L"";
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
    if (!w.empty() && w.back() == L'\0') w.pop_back();
    return w;
}

static std::string trim(const std::string& s)
{
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// Prompt user to pick a CSV file
static bool promptCSV(const wchar_t* title, std::wstring& outPath)
{
    wchar_t buf[MAX_PATH] = { 0 };
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return false;
    outPath = buf;
    return true;
}

// ===========================================================================
//  Ensure layer exists, create if missing
// ===========================================================================

static bool ensureLayer(AcDbDatabase* pDb, const wchar_t* layerName,
    Adesk::UInt16 colorIdx)
{
    AcDbLayerTable* pLT = nullptr;
    if (pDb->getLayerTable(pLT, AcDb::kForWrite) != Acad::eOk) return false;

    if (!pLT->has(layerName))
    {
        AcDbLayerTableRecord* pLR = new AcDbLayerTableRecord();
        pLR->setName(layerName);
        AcCmColor col;
        col.setColorIndex(colorIdx);
        pLR->setColor(col);
        pLT->add(pLR);
        pLR->close();
    }
    pLT->close();
    return true;
}

// ===========================================================================
//  Register XData application name
// ===========================================================================

static bool ensureRegApp(AcDbDatabase* pDb, const wchar_t* appName)
{
    AcDbRegAppTable* pRAT = nullptr;
    if (pDb->getRegAppTable(pRAT, AcDb::kForWrite) != Acad::eOk) return false;

    if (!pRAT->has(appName))
    {
        AcDbRegAppTableRecord* pRec = new AcDbRegAppTableRecord();
        pRec->setName(appName);
        pRAT->add(pRec);
        pRec->close();
    }
    pRAT->close();
    return true;
}

// ===========================================================================
//  Attach XData to an entity
//  Format: {WSPRO, pipe_id, us_id, ds_id, diameter_mm, material,
//           pres_class, vel_flag, surge_flag, civil3d_class, notes}
// ===========================================================================

static void attachPipeXData(AcDbEntity* pEnt, const WSPipeRow& p)
{
    resbuf* pHead = acutBuildList(
        AcDb::kDxfRegAppName, WSPRO_APP,
        AcDb::kDxfXdAsciiString, L"PIPE_ID:",
        AcDb::kDxfXdAsciiString, p.pipeId.c_str(),
        AcDb::kDxfXdAsciiString, L"US_ID:",
        AcDb::kDxfXdAsciiString, p.usId.c_str(),
        AcDb::kDxfXdAsciiString, L"DS_ID:",
        AcDb::kDxfXdAsciiString, p.dsId.c_str(),
        AcDb::kDxfXdAsciiString, L"DIAMETER_MM:",
        AcDb::kDxfXdReal, p.diameter,
        AcDb::kDxfXdAsciiString, L"MATERIAL:",
        AcDb::kDxfXdAsciiString, p.material.c_str(),
        AcDb::kDxfXdAsciiString, L"PRES_CLASS:",
        AcDb::kDxfXdAsciiString, p.presClass.c_str(),
        AcDb::kDxfXdAsciiString, L"VELOCITY_FLAG:",
        AcDb::kDxfXdAsciiString, p.velocityFlag.c_str(),
        AcDb::kDxfXdAsciiString, L"SURGE_FLAG:",
        AcDb::kDxfXdAsciiString, p.surgeFlag.c_str(),
        AcDb::kDxfXdAsciiString, L"CIVIL3D_CLASS:",
        AcDb::kDxfXdAsciiString, p.civil3dClass.c_str(),
        AcDb::kDxfXdAsciiString, L"NOTES:",
        AcDb::kDxfXdAsciiString, p.notes.c_str(),
        RTNONE);

    if (pHead)
    {
        pEnt->setXData(pHead);
        acutRelRb(pHead);
    }
}
//```
//
//Each value now has a label string before it.So XDLIST will show :
//```
//string : PIPE_ID :
//    string : 199_201
//    string : US_ID:
//string: 199
//string : DIAMETER_MM :
//    Real number : 609.600
//    string : MATERIAL :
//    string : DI

// ===========================================================================
//  Draw a small cross marker for a node
// ===========================================================================

static void addNodeMarker(AcDbBlockTableRecord* pMs,
    const WSNode& node,
    double size = 0.05)
{
    AcGePoint3d centre(node.x, node.y, node.z);

    // Horizontal arm
    AcDbLine* pH = new AcDbLine(
        AcGePoint3d(node.x - size, node.y, node.z),
        AcGePoint3d(node.x + size, node.y, node.z));
    pH->setLayer(LAYER_NODES);
    pH->setColorIndex(COL_NODE);
    pMs->appendAcDbEntity(pH);
    pH->close();

    // Vertical arm
    AcDbLine* pV = new AcDbLine(
        AcGePoint3d(node.x, node.y - size, node.z),
        AcGePoint3d(node.x, node.y + size, node.z));
    pV->setLayer(LAYER_NODES);
    pV->setColorIndex(COL_NODE);
    pMs->appendAcDbEntity(pV);
    pV->close();
}

// ===========================================================================
//  MAIN COMMAND: WSPROIMPORT
// ===========================================================================

void cmdWSProImport()
{
    acutPrintf(L"\n=== WS Pro Import to Civil 3D ===\n");

    // ── Step 1: Pick files ──────────────────────────────────────────────────

    std::wstring nodesPath, pipesPath, resPath;

    acutPrintf(L"Select NODES CSV (PIPE_BURST_NODES.CSV):\n");
    if (!promptCSV(L"Select Nodes CSV", nodesPath))
    {
        acutPrintf(L"Cancelled.\n"); return;
    }
    acutPrintf(L"Nodes : %s\n", nodesPath.c_str());

    acutPrintf(L"Select PIPES CSV (PIPES_EXPORT.csv):\n");
    if (!promptCSV(L"Select Pipes CSV", pipesPath))
    {
        acutPrintf(L"Cancelled.\n"); return;
    }
    acutPrintf(L"Pipes : %s\n", pipesPath.c_str());

    acutPrintf(L"Select RESERVOIRS CSV (PIPE_BURST_RESERVOIR.CSV) - press Cancel to skip:\n");
    promptCSV(L"Select Reservoirs CSV (optional - Cancel to skip)", resPath);
    if (!resPath.empty())
        acutPrintf(L"Reservoirs: %s\n", resPath.c_str());

    // ── Step 2: Convert wide paths to narrow for CSVParser ─────────────────

    auto toNarrow = [](const std::wstring& w) -> std::string {
        if (w.empty()) return "";
        int n = WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (n <= 0) return "";
        std::string s(n, '\0');
        WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, &s[0], n, nullptr, nullptr);
        if (!s.empty() && s.back() == '\0') s.pop_back();
        return s;
        };

    // ── Step 3: Parse nodes ─────────────────────────────────────────────────

    std::map<std::wstring, WSNode> nodeMap;

    auto nodeRows = CSVParser::Parse(toNarrow(nodesPath));
    if (nodeRows.size() < 2)
    {
        acutPrintf(L"[ERROR] Nodes CSV has no data rows.\n"); return;
    }

    // Find columns: NODE_ID, X, Y, Z_GROUND
    auto& nodeHdr = nodeRows[0];
    int ci_nid = -1, ci_nx = -1, ci_ny = -1, ci_nz = -1;
    for (int i = 0; i < (int)nodeHdr.size(); ++i)
    {
        std::string h = trim(nodeHdr[i]);
        if (h == "NODE_ID")   ci_nid = i;
        else if (h == "X")    ci_nx = i;
        else if (h == "Y")    ci_ny = i;
        else if (h == "Z_GROUND") ci_nz = i;
    }
    if (ci_nid < 0 || ci_nx < 0 || ci_ny < 0)
    {
        acutPrintf(L"[ERROR] Nodes CSV missing NODE_ID, X, or Y columns.\n"); return;
    }

    for (size_t r = 1; r < nodeRows.size(); ++r)
    {
        auto& row = nodeRows[r];
        if ((int)row.size() <= ci_nid) continue;
        std::wstring nid = toWide(trim(row[ci_nid]));
        if (nid.empty()) continue;

        WSNode n;
        n.id = nid;
        n.type = L"JUNCTION";
        if (ci_nx < (int)row.size()) n.x = toDouble(trim(row[ci_nx]));
        if (ci_ny < (int)row.size()) n.y = toDouble(trim(row[ci_ny]));
        if (ci_nz >= 0 && ci_nz < (int)row.size()) n.z = toDouble(trim(row[ci_nz]));
        nodeMap[nid] = n;
    }
    acutPrintf(L"Loaded %d nodes.\n", (int)nodeMap.size());

    // ── Step 4: Parse reservoirs (optional) ─────────────────────────────────

    if (!resPath.empty())
    {
        auto resRows = CSVParser::Parse(toNarrow(resPath));
        if (resRows.size() >= 2)
        {
            auto& rHdr = resRows[0];
            int ci_rid = -1, ci_rx = -1, ci_ry = -1, ci_rtw = -1;
            for (int i = 0; i < (int)rHdr.size(); ++i)
            {
                std::string h = trim(rHdr[i]);
                if (h == "RES_ID")     ci_rid = i;
                else if (h == "X")     ci_rx = i;
                else if (h == "Y")     ci_ry = i;
                else if (h == "TOP_WATER") ci_rtw = i;
            }
            for (size_t r = 1; r < resRows.size(); ++r)
            {
                auto& row = resRows[r];
                if (ci_rid < 0 || ci_rid >= (int)row.size()) continue;
                std::wstring rid = toWide(trim(row[ci_rid]));
                if (rid.empty()) continue;

                WSNode res;
                res.id = rid;
                res.type = L"RESERVOIR";
                if (ci_rx >= 0 && ci_rx < (int)row.size()) res.x = toDouble(trim(row[ci_rx]));
                if (ci_ry >= 0 && ci_ry < (int)row.size()) res.y = toDouble(trim(row[ci_ry]));
                if (ci_rtw >= 0 && ci_rtw < (int)row.size()) res.z = toDouble(trim(row[ci_rtw]));
                nodeMap[rid] = res;
            }
            acutPrintf(L"Loaded reservoirs. Total nodes: %d\n", (int)nodeMap.size());
        }
    }

    // ── Step 5: Parse pipes ─────────────────────────────────────────────────

    auto pipeRows = CSVParser::Parse(toNarrow(pipesPath));
    if (pipeRows.size() < 2)
    {
        acutPrintf(L"[ERROR] Pipes CSV has no data rows.\n"); return;
    }

    // Map column names
    auto& pipeHdr = pipeRows[0];
    int ci_usil = -1, ci_dsil = -1, ci_diam = -1, ci_mat = -1, ci_pcls = -1;
    int ci_usid = -1, ci_dsid = -1, ci_len = -1;
    int ci_pid = -1, ci_vflag = -1, ci_sflag = -1, ci_c3d = -1, ci_notes = -1;

    for (int i = 0; i < (int)pipeHdr.size(); ++i)
    {
        std::string h = trim(pipeHdr[i]);
        if (h == "US_IL")           ci_usil = i;
        else if (h == "DS_IL")           ci_dsil = i;
        else if (h == "DIAMETER")        ci_diam = i;
        else if (h == "MATERIAL")        ci_mat = i;
        else if (h == "PRES_CLASS")      ci_pcls = i;
        else if (h == "US_ID")           ci_usid = i;
        else if (h == "DS_ID")           ci_dsid = i;
        else if (h == "LENGTH")          ci_len = i;
        else if (h == "PIPE_ID")         ci_pid = i;
        else if (h == "VELOCITY_FLAG")   ci_vflag = i;
        else if (h == "SURGE_FLAG")      ci_sflag = i;
        else if (h == "CIVIL3D_PN_CLASS")ci_c3d = i;
        else if (h == "NOTES")           ci_notes = i;
    }

    if (ci_usid < 0 || ci_dsid < 0)
    {
        acutPrintf(L"[ERROR] Pipes CSV missing US_ID or DS_ID columns.\n"); return;
    }

    std::vector<WSPipeRow> pipes;
    int skipped = 0;

    for (size_t r = 1; r < pipeRows.size(); ++r)
    {
        auto& row = pipeRows[r];
        if ((int)row.size() <= ci_dsid) { skipped++; continue; }

        WSPipeRow p;
        p.usId = toWide(trim(row[ci_usid]));
        p.dsId = toWide(trim(row[ci_dsid]));

        if (p.usId.empty() || p.dsId.empty()) { skipped++; continue; }

        // Check both nodes exist
        if (nodeMap.find(p.usId) == nodeMap.end() ||
            nodeMap.find(p.dsId) == nodeMap.end())
        {
            acutPrintf(L"  [WARN] Pipe row %d: node %s or %s not found - skipping\n",
                (int)r, p.usId.c_str(), p.dsId.c_str());
            skipped++;
            continue;
        }

        if (ci_usil >= 0 && ci_usil < (int)row.size()) p.usIL = toDouble(trim(row[ci_usil]));
        if (ci_dsil >= 0 && ci_dsil < (int)row.size()) p.dsIL = toDouble(trim(row[ci_dsil]));
        if (ci_diam >= 0 && ci_diam < (int)row.size()) p.diameter = toDouble(trim(row[ci_diam]), 100.0);
        if (ci_len >= 0 && ci_len < (int)row.size()) p.length = toDouble(trim(row[ci_len]));
        if (ci_mat >= 0 && ci_mat < (int)row.size()) p.material = toWide(trim(row[ci_mat]));
        if (ci_pcls >= 0 && ci_pcls < (int)row.size()) p.presClass = toWide(trim(row[ci_pcls]));
        if (ci_pid >= 0 && ci_pid < (int)row.size()) p.pipeId = toWide(trim(row[ci_pid]));
        if (ci_vflag >= 0 && ci_vflag < (int)row.size()) p.velocityFlag = toWide(trim(row[ci_vflag]));
        if (ci_sflag >= 0 && ci_sflag < (int)row.size()) p.surgeFlag = toWide(trim(row[ci_sflag]));
        if (ci_c3d >= 0 && ci_c3d < (int)row.size()) p.civil3dClass = toWide(trim(row[ci_c3d]));
        if (ci_notes >= 0 && ci_notes < (int)row.size()) p.notes = toWide(trim(row[ci_notes]));

        if (p.pipeId.empty())
        {
            // Auto-generate pipe ID from node IDs
            p.pipeId = p.usId + L"_" + p.dsId;
        }

        pipes.push_back(p);
    }

    acutPrintf(L"Loaded %d pipes (%d skipped).\n", (int)pipes.size(), skipped);

    if (pipes.empty())
    {
        acutPrintf(L"[ERROR] No valid pipes to import.\n"); return;
    }

    // ── Step 6: Open drawing database ──────────────────────────────────────

    AcDbDatabase* pDb = acdbHostApplicationServices()->workingDatabase();
    if (!pDb) { acutPrintf(L"[ERROR] No working database.\n"); return; }

    // Create layers and register XData app
    ensureLayer(pDb, LAYER_PIPES, COL_PIPE_DEFAULT);
    ensureLayer(pDb, LAYER_NODES, COL_NODE);
    ensureRegApp(pDb, WSPRO_APP);

    // Open modelspace
    AcDbBlockTable* pBT = nullptr;
    pDb->getBlockTable(pBT, AcDb::kForRead);
    AcDbBlockTableRecord* pMs = nullptr;
    pBT->getAt(ACDB_MODEL_SPACE, pMs, AcDb::kForWrite);
    pBT->close();

    if (!pMs) { acutPrintf(L"[ERROR] Cannot open modelspace.\n"); return; }

    // ── Step 7: Draw pipes as AcDbLine with XData ───────────────────────────

    int created = 0, flagged = 0;

    for (const auto& p : pipes)
    {
        const WSNode& us = nodeMap[p.usId];
        const WSNode& ds = nodeMap[p.dsId];

        // Use invert levels for Z if available, otherwise ground level
        double zStart = (p.usIL != 0.0) ? p.usIL : us.z;
        double zEnd = (p.dsIL != 0.0) ? p.dsIL : ds.z;

        AcGePoint3d ptStart(us.x, us.y, zStart);
        AcGePoint3d ptEnd(ds.x, ds.y, zEnd);

        AcDbLine* pLine = new AcDbLine(ptStart, ptEnd);
        pLine->setLayer(LAYER_PIPES);

        // Colour by flag status
        bool surgeOk = (p.surgeFlag.empty() || p.surgeFlag == L"OK");
        bool velOk = (p.velocityFlag.empty() || p.velocityFlag == L"OK");

        if (!surgeOk)
        {
            pLine->setColorIndex(COL_PIPE_SURGE);    // red
            flagged++;
        }
        else if (!velOk)
        {
            pLine->setColorIndex(COL_PIPE_VELOCITY); // yellow
            flagged++;
        }
        else
        {
            pLine->setColorIndex(COL_PIPE_DEFAULT);  // green
        }

        // Attach all hydraulic data as XData
        attachPipeXData(pLine, p);

        pMs->appendAcDbEntity(pLine);
        pLine->close();
        created++;
    }

    // ── Step 8: Draw node markers ───────────────────────────────────────────

    int nodeCount = 0;
    for (const auto& kv : nodeMap)
    {
        addNodeMarker(pMs, kv.second, 0.05);
        nodeCount++;
    }

    pMs->close();

    // ── Step 9: Report ──────────────────────────────────────────────────────

    acutPrintf(L"\n[SUCCESS] Import complete:\n");
    acutPrintf(L"  Pipes created : %d\n", created);
    acutPrintf(L"  Flagged pipes : %d (red=surge, yellow=velocity)\n", flagged);
    acutPrintf(L"  Node markers  : %d\n", nodeCount);
    acutPrintf(L"  Layers        : %s  %s\n", LAYER_PIPES, LAYER_NODES);
    acutPrintf(L"\n  XData app '%s' attached to all pipes.\n", WSPRO_APP);
    acutPrintf(L"  Select any pipe and type XDLIST to inspect hydraulic data.\n");
    acutPrintf(L"\n  Type ZOOM E to fit view.\n");
}
