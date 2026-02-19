// ============================================================
//  PressureNetworkBridge.cpp
//
//  ROOT CAUSE of OFN_FILEMUSTEXIST / DWORD errors:
//  PressureNetworkBridge.h includes <adscodes.h> first, which
//  pulls in ARX's acad_windows.h → sets _WINDOWS_ guard →
//  subsequent #include <windows.h> becomes a no-op.
//
//  FIX: Declare every Windows type/function we need ourselves,
//  BEFORE including any ARX or project header.
// ============================================================

// ── Step 1: define Windows primitives we need ─────────────────
// These match the real Windows SDK values exactly.
// Safe to define even if windows.h loads later — guards prevent clash.
// Add this include at the top, after ARX headers:
//#include <aced.h> // for acedCommandS
//#ifndef DWORD
//typedef unsigned long DWORD;
//#endif
//
//#ifndef MAX_PATH
//#define MAX_PATH 260
//#endif
//
//#ifndef CP_ACP
//#define CP_ACP 0
//#endif
//
//// Declare the two Win32 functions we use, without pulling windows.h
//extern "C"
//{
//    // kernel32 — GetTempPathW
//    __declspec(dllimport) DWORD __stdcall
//        GetTempPathW(DWORD nBufferLength, wchar_t* lpBuffer);
//
//    // kernel32 — WideCharToMultiByte
//    __declspec(dllimport) int __stdcall
//        WideCharToMultiByte(unsigned int   CodePage,
//            DWORD          dwFlags,
//            const wchar_t* lpWideCharStr,
//            int            cchWideChar,
//            char* lpMultiByteStr,
//            int            cbMultiByte,
//            const char* lpDefaultChar,
//            int* lpUsedDefaultChar);
//}
//
//// ── Step 2: ARX headers (may now load acad_windows.h safely) ──
//#include "PressureNetworkBridge.h" // pulls adscodes.h → acad_windows.h
//#include <aced.h>                    // acedCommandS
//#include <acedads.h>                 // resbuf
//#include <acutads.h>                 // acutPrintf
//#include "aced.h"  
//#include <acedCmdNF.h>   // ← this was the missing include
//// ── Step 3: Standard library ──────────────────────────────────
//#include <map>
//#include <algorithm>
//#include <cassert>
//#include <sstream>
//#include <iomanip>
//#include <fstream>
//
//// ─────────────────────────────────────────────────────────────
////  File-scope helpers
//// ─────────────────────────────────────────────────────────────
//
//static std::wstring wsTrim(const std::wstring& s)
//{
//    size_t b = s.find_first_not_of(L" \t\r\n");
//    if (b == std::wstring::npos) return L"";
//    size_t e = s.find_last_not_of(L" \t\r\n");
//    return s.substr(b, e - b + 1);
//}
//
//static std::vector<std::wstring> wsSplitCSV(const std::wstring& line)
//{
//    std::vector<std::wstring> cols;
//    std::wstring cur;
//    bool inQuote = false;
//    for (wchar_t c : line)
//    {
//        if (c == L'"') { inQuote = !inQuote; }
//        else if (c == L',' && !inQuote) { cols.push_back(wsTrim(cur)); cur.clear(); }
//        else { cur += c; }
//    }
//    cols.push_back(wsTrim(cur));
//    return cols;
//}
//
//static void wsToUpper(std::wstring& s)
//{
//    std::transform(s.begin(), s.end(), s.begin(), ::towupper);
//}
//
//// ─────────────────────────────────────────────────────────────
//namespace PNBridge
//{
//    // ─────────────────────────────────────────────────────────────
//    //  CSV IMPORT
//    // ─────────────────────────────────────────────────────────────
//
//    BridgeError PressureNetworkBridge::loadFromCSV(
//        const std::wstring& nodesCsvPath,
//        const std::wstring& segsCsvPath,
//        NetworkDef& outNet)
//    {
//        // ── Nodes ─────────────────────────────────────────────────
//        {
//            std::wifstream f(nodesCsvPath);
//            if (!f.is_open())
//                return { BridgeResult::ERR_CSV_PARSE,
//                         L"Cannot open nodes CSV: " + nodesCsvPath };
//
//            std::wstring line;
//            bool headerDone = false;
//            std::map<std::wstring, int> col;
//
//            while (std::getline(f, line))
//            {
//                if (wsTrim(line).empty()) continue;
//                std::vector<std::wstring> cells = wsSplitCSV(line);
//
//                if (!headerDone)
//                {
//                    headerDone = true;
//                    for (int i = 0; i < static_cast<int>(cells.size()); ++i)
//                    {
//                        std::wstring h = cells[i];
//                        wsToUpper(h);
//                        col[h] = i;
//                    }
//                    const wchar_t* req[] = { L"ID", L"X", L"Y", L"Z", L"TYPE" };
//                    for (int r = 0; r < 5; ++r)
//                    {
//                        if (col.find(req[r]) == col.end())
//                            return { BridgeResult::ERR_CSV_PARSE,
//                                     std::wstring(L"Nodes CSV missing column: ") + req[r] };
//                    }
//                    continue;
//                }
//
//                if (cells.size() < 5) continue;
//
//                PipeNode n;
//                n.id = cells[col[L"ID"]];
//                n.x = std::stod(cells[col[L"X"]]);
//                n.y = std::stod(cells[col[L"Y"]]);
//                n.z = std::stod(cells[col[L"Z"]]);
//                n.type = cells[col[L"TYPE"]];
//                wsToUpper(n.type);
//
//                if (n.id.empty())
//                    return { BridgeResult::ERR_INVALID_DATA, L"Node row has empty ID" };
//
//                outNet.nodes.push_back(std::move(n));
//            }
//        }
//
//        if (outNet.nodes.empty())
//            return { BridgeResult::ERR_INVALID_DATA, L"No nodes loaded from CSV" };
//
//        // ── Segments ──────────────────────────────────────────────
//        {
//            std::wifstream f(segsCsvPath);
//            if (!f.is_open())
//                return { BridgeResult::ERR_CSV_PARSE,
//                         L"Cannot open segments CSV: " + segsCsvPath };
//
//            std::wstring line;
//            bool headerDone = false;
//            std::map<std::wstring, int> col;
//
//            while (std::getline(f, line))
//            {
//                if (wsTrim(line).empty()) continue;
//                std::vector<std::wstring> cells = wsSplitCSV(line);
//
//                if (!headerDone)
//                {
//                    headerDone = true;
//                    for (int i = 0; i < static_cast<int>(cells.size()); ++i)
//                    {
//                        std::wstring h = cells[i];
//                        wsToUpper(h);
//                        col[h] = i;
//                    }
//                    const wchar_t* req[] = {
//                        L"ID", L"START_NODE", L"END_NODE", L"DIAMETER_MM", L"MATERIAL"
//                    };
//                    for (int r = 0; r < 5; ++r)
//                    {
//                        if (col.find(req[r]) == col.end())
//                            return { BridgeResult::ERR_CSV_PARSE,
//                                     std::wstring(L"Segments CSV missing column: ") + req[r] };
//                    }
//                    continue;
//                }
//
//                if (cells.size() < 5) continue;
//
//                PipeSeg s;
//                s.id = cells[col[L"ID"]];
//                s.startNode = cells[col[L"START_NODE"]];
//                s.endNode = cells[col[L"END_NODE"]];
//                s.diameter = std::stod(cells[col[L"DIAMETER_MM"]]);
//                s.material = cells[col[L"MATERIAL"]];
//
//                if (s.id.empty() || s.startNode.empty() || s.endNode.empty())
//                    return { BridgeResult::ERR_INVALID_DATA,
//                             L"Segment row has empty ID/start/end" };
//
//                if (!findNode(outNet, s.startNode))
//                    return { BridgeResult::ERR_INVALID_DATA,
//                             L"Segment " + s.id + L" unknown start node: " + s.startNode };
//
//                if (!findNode(outNet, s.endNode))
//                    return { BridgeResult::ERR_INVALID_DATA,
//                             L"Segment " + s.id + L" unknown end node: " + s.endNode };
//
//                outNet.segments.push_back(std::move(s));
//            }
//        }
//
//        if (outNet.segments.empty())
//            return { BridgeResult::ERR_INVALID_DATA, L"No segments loaded from CSV" };
//
//        return {};
//    }
//
//    // ─────────────────────────────────────────────────────────────
//    //  SCRIPT WRITERS
//    // ─────────────────────────────────────────────────────────────
//
//    void PressureNetworkBridge::writeHeader(std::wofstream& f, const NetworkDef& net)
//    {
//        f << L"; Auto-generated by PressureNetworkBridge\n"
//            << L"; Network: " << net.name << L"\n\n"
//            << L"CMDECHO 0\n"
//            << L"FILEDIA 0\n\n";
//    }
//
//    void PressureNetworkBridge::writeSetCurrentLayer(std::wofstream& f,
//        const std::wstring& layer)
//    {
//        f << L"-LAYER\nM\n" << layer << L"\n\n";
//    }
//
//    void PressureNetworkBridge::writeCreateNetwork(std::wofstream& f,
//        const NetworkDef& net)
//    {
//        f << L"CREATEPRESSURENETWORK\n"
//            << escapeForScript(net.name) << L"\n"
//            << escapeForScript(net.description) << L"\n"
//            << escapeForScript(net.partsListName) << L"\n\n\n";
//    }
//
//    void PressureNetworkBridge::writeAddJunction(std::wofstream& f, const PipeNode& n)
//    {
//        f << L"ADDPRESSURENETWORKFITTING\n"
//            << fmt(n.x) << L"," << fmt(n.y) << L"," << fmt(n.z) << L"\n\n";
//    }
//
//    void PressureNetworkBridge::writeAddHydrant(std::wofstream& f, const PipeNode& n)
//    {
//        f << L"ADDPRESSURENETWORKAPPURTENANCE\n"
//            << fmt(n.x) << L"," << fmt(n.y) << L"," << fmt(n.z) << L"\n\n";
//    }
//
//    void PressureNetworkBridge::writeAddValve(std::wofstream& f, const PipeNode& n)
//    {
//        f << L"ADDPRESSURENETWORKAPPURTENANCE\n"
//            << fmt(n.x) << L"," << fmt(n.y) << L"," << fmt(n.z) << L"\n\n";
//    }
//
//    void PressureNetworkBridge::writeAddPipe(std::wofstream& f,
//        const PipeSeg& seg,
//        const NetworkDef& net)
//    {
//        const PipeNode* s = findNode(net, seg.startNode);
//        const PipeNode* e = findNode(net, seg.endNode);
//        assert(s && e);
//
//        f << L"ADDPRESSURENETWORKPIPE\n"
//            << fmt(s->x) << L"," << fmt(s->y) << L"," << fmt(s->z) << L"\n"
//            << fmt(e->x) << L"," << fmt(e->y) << L"," << fmt(e->z) << L"\n\n";
//    }
//
//    // ─────────────────────────────────────────────────────────────
//    //  MAIN: build .scr + run via acedCommandS
//    // ─────────────────────────────────────────────────────────────
//
//    BridgeError PressureNetworkBridge::createPressureNetwork(
//        const NetworkDef& net,
//        const std::wstring& scriptDir)
//    {
//        std::wstring dir = scriptDir.empty() ? tempDir() : scriptDir;
//        if (!dir.empty() && dir.back() != L'\\') dir += L'\\';
//        m_lastScriptPath = dir + L"pn_" + net.name + L".scr";
//
//        // Sanitise filename part
//        for (size_t i = dir.size(); i < m_lastScriptPath.size(); ++i)
//            if (m_lastScriptPath[i] == L' ') m_lastScriptPath[i] = L'_';
//
//        {
//            std::wofstream f(m_lastScriptPath, std::ios::out | std::ios::trunc);
//            if (!f.is_open())
//                return { BridgeResult::ERR_SCRIPT_WRITE,
//                         L"Cannot write script: " + m_lastScriptPath };
//
//            writeHeader(f, net);
//            writeSetCurrentLayer(f, net.layerName);
//            writeCreateNetwork(f, net);
//
//            for (const auto& node : net.nodes)
//            {
//                if (node.type == L"HYDRANT") writeAddHydrant(f, node);
//                else if (node.type == L"VALVE")   writeAddValve(f, node);
//                else                              writeAddJunction(f, node);
//            }
//            for (const auto& seg : net.segments)
//                writeAddPipe(f, seg, net);
//
//            f << L"\nCMDECHO 1\nFILEDIA 1\n";
//            f.flush();
//            if (!f.good())
//                return { BridgeResult::ERR_SCRIPT_WRITE, L"Error flushing script" };
//        }
//
//        // Wide → narrow path conversion using our forward-declared WideCharToMultiByte
//        int needed = WideCharToMultiByte(CP_ACP, 0,
//            m_lastScriptPath.c_str(), -1,
//            NULL, 0, NULL, NULL);
//        std::string pathA(static_cast<size_t>(needed), '\0');
//        WideCharToMultiByte(CP_ACP, 0,
//            m_lastScriptPath.c_str(), -1,
//            &pathA[0], needed, NULL, NULL);
//        if (!pathA.empty() && pathA.back() == '\0')
//            pathA.pop_back();
//
//        // acedCommandS — replacement for deprecated acedCommand (ARX 2021+)
//        int rc = acedCommandS(RTSTR, "SCRIPT",
//            RTSTR, pathA.c_str(),
//            RTNONE);
//
//        if (rc != RTNORM)
//            return { BridgeResult::ERR_COMMAND_EXEC,
//                     L"acedCommandS failed, code=" + std::to_wstring(rc)
//                     + L"  Script: " + m_lastScriptPath };
//
//        return {};
//    }
//
//    // ─────────────────────────────────────────────────────────────
//    //  CONVENIENCE
//    // ─────────────────────────────────────────────────────────────
//
//    BridgeError PressureNetworkBridge::run(
//        const std::wstring& nodesCsvPath,
//        const std::wstring& segsCsvPath,
//        const std::wstring& networkName,
//        const std::wstring& partsListName,
//        const std::wstring& layerName)
//    {
//        NetworkDef net;
//        net.name = networkName;
//        net.description = L"Imported from CSV by PressureNetworkBridge";
//        net.partsListName = partsListName;
//        net.layerName = layerName;
//
//        BridgeError err = loadFromCSV(nodesCsvPath, segsCsvPath, net);
//        if (!err.ok()) return err;
//        return createPressureNetwork(net);
//    }
//
//    // ─────────────────────────────────────────────────────────────
//    //  PRIVATE HELPERS
//    // ─────────────────────────────────────────────────────────────
//
//    const PipeNode* PressureNetworkBridge::findNode(
//        const NetworkDef& net, const std::wstring& id) const
//    {
//        for (const auto& n : net.nodes)
//            if (n.id == id) return &n;
//        return NULL;
//    }
//
//    std::wstring PressureNetworkBridge::tempDir() const
//    {
//        wchar_t buf[MAX_PATH];
//        buf[0] = L'\0';
//        DWORD len = GetTempPathW(MAX_PATH, buf);
//        if (len > 0 && len < MAX_PATH)
//            return std::wstring(buf);   // already ends with backslash
//        return L"C:\\Temp\\";
//    }
//
//    std::wstring PressureNetworkBridge::escapeForScript(const std::wstring& s) const
//    {
//        if (s.find(L' ') != std::wstring::npos)
//            return L"\"" + s + L"\"";
//        return s;
//    }
//
//    std::wstring PressureNetworkBridge::fmt(double v, int decimals) const
//    {
//        std::wostringstream ss;
//        ss << std::fixed << std::setprecision(decimals) << v;
//        return ss.str();
//    }
//
//} // namespace PNBridge




















// ============================================================
//  PressureNetworkBridge.cpp
//
//  Column mapping for your CSVs:
//  Nodes  (cssv4.CSV): ASSET_ID, X, Y, Z_ELEV, TYPE
//  Pipes  (csv3.CSV):  ASSET_ID, DIAM_MM, MATERIAL, US_NODE, DS_NODE
// ============================================================

//#ifndef DWORD
//typedef unsigned long DWORD;
//#endif
//#ifndef MAX_PATH
//#define MAX_PATH 260
//#endif
//#ifndef CP_ACP
//#define CP_ACP 0
//#endif
//
//extern "C"
//{
//    __declspec(dllimport) DWORD __stdcall
//        GetTempPathW(DWORD nBufferLength, wchar_t* lpBuffer);
//
//    __declspec(dllimport) int __stdcall
//        WideCharToMultiByte(unsigned int   CodePage,
//            DWORD          dwFlags,
//            const wchar_t* lpWideCharStr,
//            int            cchWideChar,
//            char* lpMultiByteStr,
//            int            cbMultiByte,
//            const char* lpDefaultChar,
//            int* lpUsedDefaultChar);
//}
//
//#include "PressureNetworkBridge.h"
//#include <aced.h>
//#include <acedads.h>
//#include <acutads.h>
//#include <acedCmdNF.h>
//
//#include <map>
//#include <algorithm>
//#include <cassert>
//#include <sstream>
//#include <iomanip>
//#include <fstream>
//
//// ─────────────────────────────────────────────────────────────
////  Helpers
//// ─────────────────────────────────────────────────────────────
//
//static std::wstring wsTrim(const std::wstring& s)
//{
//    size_t b = s.find_first_not_of(L" \t\r\n");
//    if (b == std::wstring::npos) return L"";
//    size_t e = s.find_last_not_of(L" \t\r\n");
//    return s.substr(b, e - b + 1);
//}
//
//static std::vector<std::wstring> wsSplitCSV(const std::wstring& line)
//{
//    std::vector<std::wstring> cols;
//    std::wstring cur;
//    bool inQuote = false;
//    for (wchar_t c : line)
//    {
//        if (c == L'"') { inQuote = !inQuote; }
//        else if (c == L',' && !inQuote) { cols.push_back(wsTrim(cur)); cur.clear(); }
//        else { cur += c; }
//    }
//    cols.push_back(wsTrim(cur));
//    return cols;
//}
//
//static void wsToUpper(std::wstring& s)
//{
//    std::transform(s.begin(), s.end(), s.begin(), ::towupper);
//}
//
//// Finds first matching alias in column map, returns index or -1
//static int resolveCol(const std::map<std::wstring, int>& col,
//    const wchar_t* aliases[], int count)
//{
//    for (int i = 0; i < count; ++i)
//    {
//        auto it = col.find(aliases[i]);
//        if (it != col.end()) return it->second;
//    }
//    return -1;
//}
//
//// ─────────────────────────────────────────────────────────────
//namespace PNBridge
//{
//
//    BridgeError PressureNetworkBridge::loadFromCSV(
//        const std::wstring& nodesCsvPath,
//        const std::wstring& segsCsvPath,
//        NetworkDef& outNet)
//    {
//        // ═══════════════════════════════════════════════════════
//        //  NODES
//        //  Your CSV: ASSET_ID, X, Y, Z_ELEV, TYPE
//        // ═══════════════════════════════════════════════════════
//        {
//            std::wifstream f(nodesCsvPath);
//            if (!f.is_open())
//                return { BridgeResult::ERR_CSV_PARSE,
//                         L"Cannot open nodes CSV: " + nodesCsvPath };
//
//            std::wstring line;
//            bool headerDone = false;
//            std::map<std::wstring, int> col;
//            int ci_id = -1, ci_x = -1, ci_y = -1, ci_z = -1, ci_type = -1;
//
//            while (std::getline(f, line))
//            {
//                if (wsTrim(line).empty()) continue;
//                std::vector<std::wstring> cells = wsSplitCSV(line);
//
//                if (!headerDone)
//                {
//                    headerDone = true;
//                    for (int i = 0; i < static_cast<int>(cells.size()); ++i)
//                    {
//                        std::wstring h = cells[i];
//                        // Strip BOM (Excel UTF-8 sometimes adds this)
//                        if (!h.empty() && (unsigned short)h[0] == 0xFEFF)
//                            h = h.substr(1);
//                        wsToUpper(h);
//                        col[h] = i;
//                    }
//
//                    // ── ID: ASSET_ID, ID, NODE_ID, NAME, LABEL, NO ──
//                    {
//                        const wchar_t* a[] = {
//                            L"ASSET_ID", L"ID", L"NODE_ID", L"NODEID",
//                            L"NAME", L"LABEL", L"NO", L"NUMBER", L"NODE"
//                        };
//                        ci_id = resolveCol(col, a, 9);
//                        if (ci_id < 0)
//                            return { BridgeResult::ERR_CSV_PARSE,
//                                     L"Nodes CSV: no ID column found. "
//                                     L"Tried: ASSET_ID, ID, NODE_ID, NAME" };
//                    }
//
//                    // ── X: X, EASTING, X_COORD ──────────────────────
//                    {
//                        const wchar_t* a[] = {
//                            L"X", L"EASTING", L"X_COORD", L"XCOORD", L"EAST"
//                        };
//                        ci_x = resolveCol(col, a, 5);
//                        if (ci_x < 0)
//                            return { BridgeResult::ERR_CSV_PARSE,
//                                     L"Nodes CSV: no X column found. "
//                                     L"Tried: X, EASTING, X_COORD" };
//                    }
//
//                    // ── Y: Y, NORTHING, Y_COORD ──────────────────────
//                    {
//                        const wchar_t* a[] = {
//                            L"Y", L"NORTHING", L"Y_COORD", L"YCOORD", L"NORTH"
//                        };
//                        ci_y = resolveCol(col, a, 5);
//                        if (ci_y < 0)
//                            return { BridgeResult::ERR_CSV_PARSE,
//                                     L"Nodes CSV: no Y column found. "
//                                     L"Tried: Y, NORTHING, Y_COORD" };
//                    }
//
//                    // ── Z: Z_ELEV, Z, ELEVATION, ELEV, HEIGHT ───────
//                    {
//                        const wchar_t* a[] = {
//                            L"Z_ELEV", L"Z", L"ELEVATION", L"ELEV",
//                            L"EL", L"HEIGHT", L"ALT", L"INVERT",
//                            L"Z_COORD", L"INVERT_ELEV"
//                        };
//                        ci_z = resolveCol(col, a, 10);
//                        if (ci_z < 0)
//                            return { BridgeResult::ERR_CSV_PARSE,
//                                     L"Nodes CSV: no Z/Elevation column found. "
//                                     L"Tried: Z_ELEV, Z, ELEVATION, ELEV" };
//                    }
//
//                    // ── TYPE: optional, defaults to JUNCTION ─────────
//                    {
//                        const wchar_t* a[] = {
//                            L"TYPE", L"NODE_TYPE", L"NODETYPE",
//                            L"KIND", L"CATEGORY", L"CLASS"
//                        };
//                        ci_type = resolveCol(col, a, 6);
//                        // Not an error if missing
//                    }
//
//                    acutPrintf(L"  Nodes columns: ID=%d X=%d Y=%d Z=%d TYPE=%d\n",
//                        ci_id, ci_x, ci_y, ci_z, ci_type);
//                    continue;
//                }
//
//                // ── Data row ─────────────────────────────────────────
//                if (static_cast<int>(cells.size()) <= ci_id) continue;
//
//                PipeNode n;
//                n.id = cells[ci_id];
//                if (n.id.empty()) continue;
//
//                try { n.x = std::stod(cells[ci_x]); }
//                catch (...) { continue; }
//                try { n.y = std::stod(cells[ci_y]); }
//                catch (...) { continue; }
//                try { n.z = std::stod(cells[ci_z]); }
//                catch (...) { n.z = 0.0; }
//
//                // TYPE: empty in your CSV → default JUNCTION
//                n.type = L"JUNCTION";
//                if (ci_type >= 0 && ci_type < static_cast<int>(cells.size())
//                    && !cells[ci_type].empty())
//                {
//                    n.type = cells[ci_type];
//                    wsToUpper(n.type);
//
//                    // Normalise aliases
//                    if (n.type == L"H" || n.type == L"FH" || n.type == L"FIRE_HYDRANT")
//                        n.type = L"HYDRANT";
//                    else if (n.type == L"V" || n.type == L"GV" || n.type == L"GATE_VALVE")
//                        n.type = L"VALVE";
//                    else if (n.type == L"J" || n.type == L"JN" || n.type == L"JCT" ||
//                        n.type == L"TEE" || n.type == L"CROSS")
//                        n.type = L"JUNCTION";
//                    else if (n.type == L"S" || n.type == L"SRC" || n.type == L"RESERVOIR" ||
//                        n.type == L"TANK" || n.type == L"INLET")
//                        n.type = L"SOURCE";
//                }
//
//                outNet.nodes.push_back(std::move(n));
//            }
//        }
//
//        if (outNet.nodes.empty())
//            return { BridgeResult::ERR_INVALID_DATA, L"No nodes loaded from CSV" };
//
//        acutPrintf(L"  Loaded %d nodes.\n",
//            static_cast<int>(outNet.nodes.size()));
//
//        // ═══════════════════════════════════════════════════════
//        //  SEGMENTS
//        //  Your CSV: ASSET_ID, DIAM_MM, MATERIAL, US_NODE, DS_NODE
//        //
//        //  NOTE: Pipes reference nodes 1, 2, 3, RIVER which are
//        //  NOT in your nodes CSV. These are source/reservoir nodes.
//        //  We auto-create them as dummy SOURCE nodes at origin (0,0,0)
//        //  so Civil 3D can still place the pipes — move them manually.
//        // ═══════════════════════════════════════════════════════
//        {
//            std::wifstream f(segsCsvPath);
//            if (!f.is_open())
//                return { BridgeResult::ERR_CSV_PARSE,
//                         L"Cannot open segments CSV: " + segsCsvPath };
//
//            std::wstring line;
//            bool headerDone = false;
//            std::map<std::wstring, int> col;
//            int ci_id = -1, ci_diam = -1, ci_mat = -1, ci_start = -1, ci_end = -1;
//
//            while (std::getline(f, line))
//            {
//                if (wsTrim(line).empty()) continue;
//                std::vector<std::wstring> cells = wsSplitCSV(line);
//
//                if (!headerDone)
//                {
//                    headerDone = true;
//                    for (int i = 0; i < static_cast<int>(cells.size()); ++i)
//                    {
//                        std::wstring h = cells[i];
//                        if (!h.empty() && (unsigned short)h[0] == 0xFEFF)
//                            h = h.substr(1);
//                        wsToUpper(h);
//                        col[h] = i;
//                    }
//
//                    // ── ID ───────────────────────────────────────────
//                    {
//                        const wchar_t* a[] = {
//                            L"ASSET_ID", L"ID", L"PIPE_ID", L"PIPEID",
//                            L"NAME", L"LABEL", L"NO", L"PIPE"
//                        };
//                        ci_id = resolveCol(col, a, 8);
//                        if (ci_id < 0)
//                            return { BridgeResult::ERR_CSV_PARSE,
//                                     L"Segments CSV: no ID column found." };
//                    }
//
//                    // ── DIAMETER ─────────────────────────────────────
//                    {
//                        const wchar_t* a[] = {
//                            L"DIAM_MM", L"DIAMETER_MM", L"DIAMETER",
//                            L"DIAM", L"DIA", L"DIA_MM", L"SIZE", L"SIZE_MM"
//                        };
//                        ci_diam = resolveCol(col, a, 8);
//                        if (ci_diam < 0)
//                            return { BridgeResult::ERR_CSV_PARSE,
//                                     L"Segments CSV: no DIAMETER column found. "
//                                     L"Tried: DIAM_MM, DIAMETER, DIAM, DIA" };
//                    }
//
//                    // ── MATERIAL: optional ───────────────────────────
//                    {
//                        const wchar_t* a[] = {
//                            L"MATERIAL", L"MAT", L"PIPE_MATERIAL",
//                            L"PIPE_TYPE", L"MATL"
//                        };
//                        ci_mat = resolveCol(col, a, 5);
//                    }
//
//                    // ── START NODE: US_NODE, FROM, START ────────────
//                    {
//                        const wchar_t* a[] = {
//                            L"US_NODE", L"START_NODE", L"START",
//                            L"FROM_NODE", L"FROM", L"UPSTREAM",
//                            L"NODE1", L"JUNC1", L"INLET_NODE"
//                        };
//                        ci_start = resolveCol(col, a, 9);
//                        if (ci_start < 0)
//                            return { BridgeResult::ERR_CSV_PARSE,
//                                     L"Segments CSV: no START node column found. "
//                                     L"Tried: US_NODE, START_NODE, FROM_NODE, FROM" };
//                    }
//
//                    // ── END NODE: DS_NODE, TO, END ───────────────────
//                    {
//                        const wchar_t* a[] = {
//                            L"DS_NODE", L"END_NODE", L"END",
//                            L"TO_NODE", L"TO", L"DOWNSTREAM",
//                            L"NODE2", L"JUNC2", L"OUTLET_NODE"
//                        };
//                        ci_end = resolveCol(col, a, 9);
//                        if (ci_end < 0)
//                            return { BridgeResult::ERR_CSV_PARSE,
//                                     L"Segments CSV: no END node column found. "
//                                     L"Tried: DS_NODE, END_NODE, TO_NODE, TO" };
//                    }
//
//                    acutPrintf(
//                        L"  Pipes columns: ID=%d DIAM=%d MAT=%d START=%d END=%d\n",
//                        ci_id, ci_diam, ci_mat, ci_start, ci_end);
//                    continue;
//                }
//
//                // ── Data row ─────────────────────────────────────────
//                if (static_cast<int>(cells.size()) <= ci_id) continue;
//
//                PipeSeg s;
//                s.id = cells[ci_id];
//                s.startNode = cells[ci_start];
//                s.endNode = cells[ci_end];
//
//                if (s.id.empty() || s.startNode.empty() || s.endNode.empty())
//                    continue;
//
//                try { s.diameter = std::stod(cells[ci_diam]); }
//                catch (...) { s.diameter = 100.0; }
//
//                s.material = (ci_mat >= 0
//                    && ci_mat < static_cast<int>(cells.size())
//                    && !cells[ci_mat].empty())
//                    ? cells[ci_mat] : L"PVC";
//
//                // ── Auto-create missing source nodes ─────────────────
//                // Nodes 1, 2, 3, RIVER exist in pipes but not nodes CSV.
//                // Create them as SOURCE at 0,0,0 — Civil 3D needs a
//                // coordinate for every referenced node.
//                auto ensureNode = [&](const std::wstring& nid)
//                    {
//                        if (!findNode(outNet, nid))
//                        {
//                            PipeNode src;
//                            src.id = nid;
//                            src.x = 0.0;
//                            src.y = 0.0;
//                            src.z = 0.0;
//                            src.type = L"SOURCE";
//                            outNet.nodes.push_back(src);
//                            acutPrintf(L"  [INFO] Auto-created SOURCE node '%s' "
//                                L"at (0,0,0) — move it manually in Civil 3D\n",
//                                nid.c_str());
//                        }
//                    };
//
//                ensureNode(s.startNode);
//                ensureNode(s.endNode);
//
//                outNet.segments.push_back(std::move(s));
//            }
//        }
//
//        if (outNet.segments.empty())
//            return { BridgeResult::ERR_INVALID_DATA, L"No segments loaded from CSV" };
//
//        acutPrintf(L"  Loaded %d segments.\n",
//            static_cast<int>(outNet.segments.size()));
//
//        return {};
//    }
//
//    // ─────────────────────────────────────────────────────────────
//    //  SCRIPT WRITERS
//    // ─────────────────────────────────────────────────────────────
//
//    void PressureNetworkBridge::writeHeader(std::wofstream& f, const NetworkDef& net)
//    {
//        f << L"; Auto-generated by PressureNetworkBridge\n"
//            << L"; Network: " << net.name << L"\n\n"
//            << L"CMDECHO 0\n"
//            << L"FILEDIA 0\n\n";
//    }
//
//    void PressureNetworkBridge::writeSetCurrentLayer(std::wofstream& f,
//        const std::wstring& layer)
//    {
//        f << L"-LAYER\nM\n" << layer << L"\n\n";
//    }
//
//    void PressureNetworkBridge::writeCreateNetwork(std::wofstream& f,
//        const NetworkDef& net)
//    {
//        f << L"CREATEPRESSURENETWORK\n"
//            << escapeForScript(net.name) << L"\n"
//            << escapeForScript(net.description) << L"\n"
//            << escapeForScript(net.partsListName) << L"\n\n\n";
//    }
//
//    void PressureNetworkBridge::writeAddJunction(std::wofstream& f, const PipeNode& n)
//    {
//        f << L"ADDPRESSURENETWORKFITTING\n"
//            << fmt(n.x) << L"," << fmt(n.y) << L"," << fmt(n.z) << L"\n\n";
//    }
//
//    void PressureNetworkBridge::writeAddHydrant(std::wofstream& f, const PipeNode& n)
//    {
//        f << L"ADDPRESSURENETWORKAPPURTENANCE\n"
//            << fmt(n.x) << L"," << fmt(n.y) << L"," << fmt(n.z) << L"\n\n";
//    }
//
//    void PressureNetworkBridge::writeAddValve(std::wofstream& f, const PipeNode& n)
//    {
//        f << L"ADDPRESSURENETWORKAPPURTENANCE\n"
//            << fmt(n.x) << L"," << fmt(n.y) << L"," << fmt(n.z) << L"\n\n";
//    }
//
//    void PressureNetworkBridge::writeAddPipe(std::wofstream& f,
//        const PipeSeg& seg,
//        const NetworkDef& net)
//    {
//        const PipeNode* s = findNode(net, seg.startNode);
//        const PipeNode* e = findNode(net, seg.endNode);
//        assert(s && e);
//
//        f << L"; Pipe " << seg.id
//            << L" D=" << fmt(seg.diameter, 1) << L"mm"
//            << L" " << seg.startNode << L"->" << seg.endNode << L"\n"
//            << L"ADDPRESSURENETWORKPIPE\n"
//            << fmt(s->x) << L"," << fmt(s->y) << L"," << fmt(s->z) << L"\n"
//            << fmt(e->x) << L"," << fmt(e->y) << L"," << fmt(e->z) << L"\n\n";
//    }
//
//    // ─────────────────────────────────────────────────────────────
//    //  MAIN: build .scr + execute
//    // ─────────────────────────────────────────────────────────────
//
//    BridgeError PressureNetworkBridge::createPressureNetwork(
//        const NetworkDef& net,
//        const std::wstring& scriptDir)
//    {
//        std::wstring dir = scriptDir.empty() ? tempDir() : scriptDir;
//        if (!dir.empty() && dir.back() != L'\\') dir += L'\\';
//        m_lastScriptPath = dir + L"pn_" + net.name + L".scr";
//
//        for (size_t i = dir.size(); i < m_lastScriptPath.size(); ++i)
//            if (m_lastScriptPath[i] == L' ') m_lastScriptPath[i] = L'_';
//
//        {
//            std::wofstream f(m_lastScriptPath, std::ios::out | std::ios::trunc);
//            if (!f.is_open())
//                return { BridgeResult::ERR_SCRIPT_WRITE,
//                         L"Cannot write script: " + m_lastScriptPath };
//
//            writeHeader(f, net);
//            writeSetCurrentLayer(f, net.layerName);
//            writeCreateNetwork(f, net);
//
//            for (const auto& node : net.nodes)
//            {
//                if (node.type == L"HYDRANT") writeAddHydrant(f, node);
//                else if (node.type == L"VALVE")   writeAddValve(f, node);
//                else                              writeAddJunction(f, node);
//            }
//            for (const auto& seg : net.segments)
//                writeAddPipe(f, seg, net);
//
//            f << L"\nCMDECHO 1\nFILEDIA 1\n";
//            f.flush();
//            if (!f.good())
//                return { BridgeResult::ERR_SCRIPT_WRITE, L"Error flushing script" };
//        }
//
//        acutPrintf(L"  Script: %s\n", m_lastScriptPath.c_str());
//
//        int needed = WideCharToMultiByte(CP_ACP, 0,
//            m_lastScriptPath.c_str(), -1,
//            NULL, 0, NULL, NULL);
//        std::string pathA(static_cast<size_t>(needed), '\0');
//        WideCharToMultiByte(CP_ACP, 0, m_lastScriptPath.c_str(), -1,
//            &pathA[0], needed, NULL, NULL);
//        if (!pathA.empty() && pathA.back() == '\0') pathA.pop_back();
//
//        int rc = acedCommandS(RTSTR, "SCRIPT", RTSTR, pathA.c_str(), RTNONE);
//
//        if (rc != RTNORM)
//            return { BridgeResult::ERR_COMMAND_EXEC,
//                     L"acedCommandS failed, code=" + std::to_wstring(rc)
//                     + L"  Script: " + m_lastScriptPath };
//
//        return {};
//    }
//
//    // ─────────────────────────────────────────────────────────────
//    //  CONVENIENCE
//    // ─────────────────────────────────────────────────────────────
//
//    BridgeError PressureNetworkBridge::run(
//        const std::wstring& nodesCsvPath,
//        const std::wstring& segsCsvPath,
//        const std::wstring& networkName,
//        const std::wstring& partsListName,
//        const std::wstring& layerName)
//    {
//        NetworkDef net;
//        net.name = networkName;
//        net.description = L"Imported from CSV by PressureNetworkBridge";
//        net.partsListName = partsListName;
//        net.layerName = layerName;
//
//        BridgeError err = loadFromCSV(nodesCsvPath, segsCsvPath, net);
//        if (!err.ok()) return err;
//        return createPressureNetwork(net);
//    }
//
//    // ─────────────────────────────────────────────────────────────
//    //  PRIVATE HELPERS
//    // ─────────────────────────────────────────────────────────────
//
//    const PipeNode* PressureNetworkBridge::findNode(
//        const NetworkDef& net, const std::wstring& id) const
//    {
//        for (const auto& n : net.nodes)
//            if (n.id == id) return &n;
//        return NULL;
//    }
//
//    std::wstring PressureNetworkBridge::tempDir() const
//    {
//        wchar_t buf[MAX_PATH];
//        buf[0] = L'\0';
//        DWORD len = GetTempPathW(MAX_PATH, buf);
//        if (len > 0 && len < MAX_PATH)
//            return std::wstring(buf);
//        return L"C:\\Temp\\";
//    }
//
//    std::wstring PressureNetworkBridge::escapeForScript(const std::wstring& s) const
//    {
//        if (s.find(L' ') != std::wstring::npos)
//            return L"\"" + s + L"\"";
//        return s;
//    }
//
//    std::wstring PressureNetworkBridge::fmt(double v, int decimals) const
//    {
//        std::wostringstream ss;
//        ss << std::fixed << std::setprecision(decimals) << v;
//        return ss.str();
//    }
//
//} // namespace PNBridge











// ============================================================
//  PressureNetworkBridge.cpp
//
//  Column mapping for your CSVs:
//  Nodes  (cssv4.CSV): ASSET_ID, X, Y, Z_ELEV, TYPE
//  Pipes  (csv3.CSV):  ASSET_ID, DIAM_MM, MATERIAL, US_NODE, DS_NODE
//
//  Fix: acedCommandS code=-5001 (RTERROR) when calling SCRIPT.
//  Root cause: cannot call SCRIPT from inside a command callback.
//  Solution: use acedSendStringToExecute() to post commands to
//  Civil 3D's command queue — runs AFTER our command returns.
// ============================================================

#ifndef DWORD
typedef unsigned long DWORD;
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#ifndef CP_ACP
#define CP_ACP 0
#endif

extern "C"
{
    __declspec(dllimport) DWORD __stdcall
        GetTempPathW(DWORD nBufferLength, wchar_t* lpBuffer);

    __declspec(dllimport) int __stdcall
        WideCharToMultiByte(unsigned int   CodePage,
            DWORD          dwFlags,
            const wchar_t* lpWideCharStr,
            int            cchWideChar,
            char* lpMultiByteStr,
            int            cbMultiByte,
            const char* lpDefaultChar,
            int* lpUsedDefaultChar);
}

#include "PressureNetworkBridge.h"
#include <aced.h>
#include <acedads.h>
#include <acutads.h>
#include <acedCmdNF.h>

#include <map>
#include <algorithm>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <fstream>

// ─────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────

static std::wstring wsTrim(const std::wstring& s)
{
    size_t b = s.find_first_not_of(L" \t\r\n");
    if (b == std::wstring::npos) return L"";
    size_t e = s.find_last_not_of(L" \t\r\n");
    return s.substr(b, e - b + 1);
}

static std::vector<std::wstring> wsSplitCSV(const std::wstring& line)
{
    std::vector<std::wstring> cols;
    std::wstring cur;
    bool inQuote = false;
    for (wchar_t c : line)
    {
        if (c == L'"') { inQuote = !inQuote; }
        else if (c == L',' && !inQuote) { cols.push_back(wsTrim(cur)); cur.clear(); }
        else { cur += c; }
    }
    cols.push_back(wsTrim(cur));
    return cols;
}

static void wsToUpper(std::wstring& s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::towupper);
}

static int resolveCol(const std::map<std::wstring, int>& col,
    const wchar_t* aliases[], int count)
{
    for (int i = 0; i < count; ++i)
    {
        auto it = col.find(aliases[i]);
        if (it != col.end()) return it->second;
    }
    return -1;
}

// ─────────────────────────────────────────────────────────────
namespace PNBridge
{

    // ─────────────────────────────────────────────────────────────
    //  CSV IMPORT
    // ─────────────────────────────────────────────────────────────

    BridgeError PressureNetworkBridge::loadFromCSV(
        const std::wstring& nodesCsvPath,
        const std::wstring& segsCsvPath,
        NetworkDef& outNet)
    {
        // ── Nodes ─────────────────────────────────────────────────
        {
            std::wifstream f(nodesCsvPath);
            if (!f.is_open())
                return { BridgeResult::ERR_CSV_PARSE,
                         L"Cannot open nodes CSV: " + nodesCsvPath };

            std::wstring line;
            bool headerDone = false;
            std::map<std::wstring, int> col;
            int ci_id = -1, ci_x = -1, ci_y = -1, ci_z = -1, ci_type = -1;

            while (std::getline(f, line))
            {
                if (wsTrim(line).empty()) continue;
                std::vector<std::wstring> cells = wsSplitCSV(line);

                if (!headerDone)
                {
                    headerDone = true;
                    for (int i = 0; i < static_cast<int>(cells.size()); ++i)
                    {
                        std::wstring h = cells[i];
                        if (!h.empty() && (unsigned short)h[0] == 0xFEFF)
                            h = h.substr(1);
                        wsToUpper(h);
                        col[h] = i;
                    }

                    {
                        const wchar_t* a[] = { L"ASSET_ID",L"ID",L"NODE_ID",L"NODEID",L"NAME",L"LABEL",L"NO",L"NUMBER",L"NODE" };
                        ci_id = resolveCol(col, a, 9);
                        if (ci_id < 0) return{ BridgeResult::ERR_CSV_PARSE,L"Nodes CSV: no ID column. Tried: ASSET_ID, ID, NODE_ID" };
                    }

                    {
                        const wchar_t* a[] = { L"X",L"EASTING",L"X_COORD",L"XCOORD",L"EAST" };
                        ci_x = resolveCol(col, a, 5);
                        if (ci_x < 0) return{ BridgeResult::ERR_CSV_PARSE,L"Nodes CSV: no X column. Tried: X, EASTING, X_COORD" };
                    }

                    {
                        const wchar_t* a[] = { L"Y",L"NORTHING",L"Y_COORD",L"YCOORD",L"NORTH" };
                        ci_y = resolveCol(col, a, 5);
                        if (ci_y < 0) return{ BridgeResult::ERR_CSV_PARSE,L"Nodes CSV: no Y column. Tried: Y, NORTHING, Y_COORD" };
                    }

                    {
                        const wchar_t* a[] = { L"Z_ELEV",L"Z",L"ELEVATION",L"ELEV",L"EL",L"HEIGHT",L"ALT",L"INVERT",L"Z_COORD",L"INVERT_ELEV" };
                        ci_z = resolveCol(col, a, 10);
                        if (ci_z < 0) return{ BridgeResult::ERR_CSV_PARSE,L"Nodes CSV: no Z column. Tried: Z_ELEV, Z, ELEVATION, ELEV" };
                    }

                    {
                        const wchar_t* a[] = { L"TYPE",L"NODE_TYPE",L"NODETYPE",L"KIND",L"CATEGORY",L"CLASS" };
                        ci_type = resolveCol(col, a, 6);
                    }

                    acutPrintf(L"  Nodes columns: ID=%d X=%d Y=%d Z=%d TYPE=%d\n",
                        ci_id, ci_x, ci_y, ci_z, ci_type);
                    continue;
                }

                if (static_cast<int>(cells.size()) <= ci_id) continue;

                PipeNode n;
                n.id = cells[ci_id];
                if (n.id.empty()) continue;

                try { n.x = std::stod(cells[ci_x]); }
                catch (...) { continue; }
                try { n.y = std::stod(cells[ci_y]); }
                catch (...) { continue; }
                try { n.z = std::stod(cells[ci_z]); }
                catch (...) { n.z = 0.0; }

                n.type = L"JUNCTION";
                if (ci_type >= 0 && ci_type < static_cast<int>(cells.size())
                    && !cells[ci_type].empty())
                {
                    n.type = cells[ci_type];
                    wsToUpper(n.type);
                    if (n.type == L"H" || n.type == L"FH" || n.type == L"FIRE_HYDRANT") n.type = L"HYDRANT";
                    else if (n.type == L"V" || n.type == L"GV" || n.type == L"GATE_VALVE") n.type = L"VALVE";
                    else if (n.type == L"J" || n.type == L"JN" || n.type == L"JCT" || n.type == L"TEE" || n.type == L"CROSS") n.type = L"JUNCTION";
                    else if (n.type == L"S" || n.type == L"SRC" || n.type == L"RESERVOIR" || n.type == L"TANK" || n.type == L"INLET") n.type = L"SOURCE";
                }

                outNet.nodes.push_back(std::move(n));
            }
        }

        if (outNet.nodes.empty())
            return { BridgeResult::ERR_INVALID_DATA, L"No nodes loaded from CSV" };

        acutPrintf(L"  Loaded %d nodes.\n", static_cast<int>(outNet.nodes.size()));

        // ── Segments ──────────────────────────────────────────────
        {
            std::wifstream f(segsCsvPath);
            if (!f.is_open())
                return { BridgeResult::ERR_CSV_PARSE,
                         L"Cannot open segments CSV: " + segsCsvPath };

            std::wstring line;
            bool headerDone = false;
            std::map<std::wstring, int> col;
            int ci_id = -1, ci_diam = -1, ci_mat = -1, ci_start = -1, ci_end = -1;

            while (std::getline(f, line))
            {
                if (wsTrim(line).empty()) continue;
                std::vector<std::wstring> cells = wsSplitCSV(line);

                if (!headerDone)
                {
                    headerDone = true;
                    for (int i = 0; i < static_cast<int>(cells.size()); ++i)
                    {
                        std::wstring h = cells[i];
                        if (!h.empty() && (unsigned short)h[0] == 0xFEFF) h = h.substr(1);
                        wsToUpper(h);
                        col[h] = i;
                    }

                    {
                        const wchar_t* a[] = { L"ASSET_ID",L"ID",L"PIPE_ID",L"PIPEID",L"NAME",L"LABEL",L"NO",L"PIPE" };
                        ci_id = resolveCol(col, a, 8);
                        if (ci_id < 0) return{ BridgeResult::ERR_CSV_PARSE,L"Segments CSV: no ID column." };
                    }

                    {
                        const wchar_t* a[] = { L"DIAM_MM",L"DIAMETER_MM",L"DIAMETER",L"DIAM",L"DIA",L"DIA_MM",L"SIZE",L"SIZE_MM" };
                        ci_diam = resolveCol(col, a, 8);
                        if (ci_diam < 0) return{ BridgeResult::ERR_CSV_PARSE,L"Segments CSV: no DIAMETER column. Tried: DIAM_MM, DIAMETER, DIAM" };
                    }

                    {
                        const wchar_t* a[] = { L"MATERIAL",L"MAT",L"PIPE_MATERIAL",L"PIPE_TYPE",L"MATL" };
                        ci_mat = resolveCol(col, a, 5);
                    }

                    {
                        const wchar_t* a[] = { L"US_NODE",L"START_NODE",L"START",L"FROM_NODE",L"FROM",L"UPSTREAM",L"NODE1",L"JUNC1",L"INLET_NODE" };
                        ci_start = resolveCol(col, a, 9);
                        if (ci_start < 0) return{ BridgeResult::ERR_CSV_PARSE,L"Segments CSV: no START node column. Tried: US_NODE, START_NODE, FROM" };
                    }

                    {
                        const wchar_t* a[] = { L"DS_NODE",L"END_NODE",L"END",L"TO_NODE",L"TO",L"DOWNSTREAM",L"NODE2",L"JUNC2",L"OUTLET_NODE" };
                        ci_end = resolveCol(col, a, 9);
                        if (ci_end < 0) return{ BridgeResult::ERR_CSV_PARSE,L"Segments CSV: no END node column. Tried: DS_NODE, END_NODE, TO" };
                    }

                    acutPrintf(L"  Pipes columns: ID=%d DIAM=%d MAT=%d START=%d END=%d\n",
                        ci_id, ci_diam, ci_mat, ci_start, ci_end);
                    continue;
                }

                if (static_cast<int>(cells.size()) <= ci_id) continue;

                PipeSeg s;
                s.id = cells[ci_id];
                s.startNode = cells[ci_start];
                s.endNode = cells[ci_end];
                if (s.id.empty() || s.startNode.empty() || s.endNode.empty()) continue;

                try { s.diameter = std::stod(cells[ci_diam]); }
                catch (...) { s.diameter = 100.0; }

                s.material = (ci_mat >= 0 && ci_mat < static_cast<int>(cells.size()) && !cells[ci_mat].empty())
                    ? cells[ci_mat] : L"PVC";

                // Auto-create missing source nodes (1, 2, 3, RIVER)
                auto ensureNode = [&](const std::wstring& nid)
                    {
                        if (!findNode(outNet, nid))
                        {
                            PipeNode src;
                            src.id = nid; src.x = 0.0; src.y = 0.0; src.z = 0.0; src.type = L"SOURCE";
                            outNet.nodes.push_back(src);
                            acutPrintf(L"  [INFO] Auto-created SOURCE node '%s' at (0,0,0)\n", nid.c_str());
                        }
                    };
                ensureNode(s.startNode);
                ensureNode(s.endNode);

                outNet.segments.push_back(std::move(s));
            }
        }

        if (outNet.segments.empty())
            return { BridgeResult::ERR_INVALID_DATA, L"No segments loaded from CSV" };

        acutPrintf(L"  Loaded %d segments.\n", static_cast<int>(outNet.segments.size()));
        return {};
    }

    // ─────────────────────────────────────────────────────────────
    //  SCRIPT WRITERS
    // ─────────────────────────────────────────────────────────────

    void PressureNetworkBridge::writeHeader(std::wofstream& f, const NetworkDef& net)
    {
        f << L"; Auto-generated by PressureNetworkBridge\n"
            << L"; Network: " << net.name << L"\n\n"
            << L"CMDECHO 0\n"
            << L"FILEDIA 0\n\n";
    }

    void PressureNetworkBridge::writeSetCurrentLayer(std::wofstream& f,
        const std::wstring& layer)
    {
        f << L"-LAYER\nM\n" << layer << L"\n\n";
    }

    void PressureNetworkBridge::writeCreateNetwork(std::wofstream& f,
        const NetworkDef& net)
    {
        f << L"CREATEPRESSURENETWORK\n"
            << escapeForScript(net.name) << L"\n"
            << escapeForScript(net.description) << L"\n"
            << escapeForScript(net.partsListName) << L"\n\n\n";
    }

    void PressureNetworkBridge::writeAddJunction(std::wofstream& f, const PipeNode& n)
    {
        f << L"ADDPRESSURENETWORKFITTING\n"
            << fmt(n.x) << L"," << fmt(n.y) << L"," << fmt(n.z) << L"\n\n";
    }

    void PressureNetworkBridge::writeAddHydrant(std::wofstream& f, const PipeNode& n)
    {
        f << L"ADDPRESSURENETWORKAPPURTENANCE\n"
            << fmt(n.x) << L"," << fmt(n.y) << L"," << fmt(n.z) << L"\n\n";
    }

    void PressureNetworkBridge::writeAddValve(std::wofstream& f, const PipeNode& n)
    {
        f << L"ADDPRESSURENETWORKAPPURTENANCE\n"
            << fmt(n.x) << L"," << fmt(n.y) << L"," << fmt(n.z) << L"\n\n";
    }

    void PressureNetworkBridge::writeAddPipe(std::wofstream& f,
        const PipeSeg& seg,
        const NetworkDef& net)
    {
        const PipeNode* s = findNode(net, seg.startNode);
        const PipeNode* e = findNode(net, seg.endNode);
        assert(s && e);

        f << L"; Pipe " << seg.id << L" " << seg.startNode << L"->" << seg.endNode << L"\n"
            << L"ADDPRESSURENETWORKPIPE\n"
            << fmt(s->x) << L"," << fmt(s->y) << L"," << fmt(s->z) << L"\n"
            << fmt(e->x) << L"," << fmt(e->y) << L"," << fmt(e->z) << L"\n\n";
    }

    // ─────────────────────────────────────────────────────────────
    //  MAIN: build .scr + execute
    //
    //  FIX for code=-5001:
    //  Cannot call SCRIPT from inside an active ARX command callback.
    //  Use acedSendStringToExecute() instead — posts "SCRIPT <path>"
    //  to Civil 3D's command queue, runs after our callback returns.
    // ─────────────────────────────────────────────────────────────

    BridgeError PressureNetworkBridge::createPressureNetwork(
        const NetworkDef& net,
        const std::wstring& scriptDir)
    {
        // Build script path
        std::wstring dir = scriptDir.empty() ? tempDir() : scriptDir;
        if (!dir.empty() && dir.back() != L'\\') dir += L'\\';
        m_lastScriptPath = dir + L"pn_" + net.name + L".scr";

        for (size_t i = dir.size(); i < m_lastScriptPath.size(); ++i)
            if (m_lastScriptPath[i] == L' ') m_lastScriptPath[i] = L'_';

        // Write the script file
        {
            std::wofstream f(m_lastScriptPath, std::ios::out | std::ios::trunc);
            if (!f.is_open())
                return { BridgeResult::ERR_SCRIPT_WRITE,
                         L"Cannot write script: " + m_lastScriptPath };

            writeHeader(f, net);
            writeSetCurrentLayer(f, net.layerName);
            writeCreateNetwork(f, net);

            for (const auto& node : net.nodes)
            {
                if (node.type == L"HYDRANT") writeAddHydrant(f, node);
                else if (node.type == L"VALVE")   writeAddValve(f, node);
                else                              writeAddJunction(f, node);
            }
            for (const auto& seg : net.segments)
                writeAddPipe(f, seg, net);

            f << L"\nCMDECHO 1\nFILEDIA 1\n";
            f.flush();
            if (!f.good())
                return { BridgeResult::ERR_SCRIPT_WRITE, L"Error flushing script" };
        }

        acutPrintf(L"  Script written: %s\n", m_lastScriptPath.c_str());

        // ── Execute via acedSendStringToExecute ───────────────────
        //
        //  acedCommandS / acedCommand both fail with -5001 when called
        //  from inside a command callback because Civil 3D won't accept
        //  a nested SCRIPT command synchronously.
        //
        //  acedSendStringToExecute(doc, string, activate, echo, cmd)
        //    - Posts the string to the document's command queue
        //    - Executes AFTER our command callback returns
        //    - This is the correct ARX way to trigger commands from callbacks
        //
        //  The string we send is: "SCRIPT <scriptpath>\n"
        //  The \n acts as Enter to confirm the file path prompt.

        // ACHAR = wchar_t in all modern ARX/Civil 3D SDKs.
        // Pass the wide string directly — no narrow conversion needed.
        // Build: SCRIPT "C:\path\pn_Network.scr"\n
        std::wstring sendStr = L"SCRIPT \"" + m_lastScriptPath + L"\"\n";

        AcApDocument* pDoc = acDocManager->curDocument();
        if (!pDoc)
            return { BridgeResult::ERR_COMMAND_EXEC,
                     L"Cannot get active document for command execution" };

        Acad::ErrorStatus es = acDocManager->sendStringToExecute(
            pDoc,
            sendStr.c_str(),  // ACHAR* == wchar_t* — wide string directly
            true,             // activate document
            false,            // don't echo
            true);            // is a command (not LISP)

        if (es != Acad::eOk)
            return { BridgeResult::ERR_COMMAND_EXEC,
                     L"sendStringToExecute failed, es=" + std::to_wstring((int)es)
                     + L"\nScript: " + m_lastScriptPath };

        // Script will run after this function returns.
        // Civil 3D picks it up from the command queue.
        acutPrintf(L"  Script queued — Civil 3D will execute it now.\n");
        return {};
    }

    // ─────────────────────────────────────────────────────────────
    //  CONVENIENCE
    // ─────────────────────────────────────────────────────────────

    BridgeError PressureNetworkBridge::run(
        const std::wstring& nodesCsvPath,
        const std::wstring& segsCsvPath,
        const std::wstring& networkName,
        const std::wstring& partsListName,
        const std::wstring& layerName)
    {
        NetworkDef net;
        net.name = networkName;
        net.description = L"Imported from CSV by PressureNetworkBridge";
        net.partsListName = partsListName;
        net.layerName = layerName;

        BridgeError err = loadFromCSV(nodesCsvPath, segsCsvPath, net);
        if (!err.ok()) return err;
        return createPressureNetwork(net);
    }

    // ─────────────────────────────────────────────────────────────
    //  PRIVATE HELPERS
    // ─────────────────────────────────────────────────────────────

    const PipeNode* PressureNetworkBridge::findNode(
        const NetworkDef& net, const std::wstring& id) const
    {
        for (const auto& n : net.nodes)
            if (n.id == id) return &n;
        return NULL;
    }

    std::wstring PressureNetworkBridge::tempDir() const
    {
        wchar_t buf[MAX_PATH];
        buf[0] = L'\0';
        DWORD len = GetTempPathW(MAX_PATH, buf);
        if (len > 0 && len < MAX_PATH)
            return std::wstring(buf);
        return L"C:\\Temp\\";
    }

    std::wstring PressureNetworkBridge::escapeForScript(const std::wstring& s) const
    {
        if (s.find(L' ') != std::wstring::npos)
            return L"\"" + s + L"\"";
        return s;
    }

    std::wstring PressureNetworkBridge::fmt(double v, int decimals) const
    {
        std::wostringstream ss;
        ss << std::fixed << std::setprecision(decimals) << v;
        return ss.str();
    }

} // namespace PNBridge



