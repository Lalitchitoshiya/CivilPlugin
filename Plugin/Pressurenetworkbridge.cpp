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
#include <aced.h> // for acedCommandS
#ifndef DWORD
typedef unsigned long DWORD;
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef CP_ACP
#define CP_ACP 0
#endif

// Declare the two Win32 functions we use, without pulling windows.h
extern "C"
{
    // kernel32 — GetTempPathW
    __declspec(dllimport) DWORD __stdcall
        GetTempPathW(DWORD nBufferLength, wchar_t* lpBuffer);

    // kernel32 — WideCharToMultiByte
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

// ── Step 2: ARX headers (may now load acad_windows.h safely) ──
#include "PressureNetworkBridge.h" // pulls adscodes.h → acad_windows.h
#include <aced.h>                    // acedCommandS
#include <acedads.h>                 // resbuf
#include <acutads.h>                 // acutPrintf
#include "aced.h"  
#include <acedCmdNF.h>   // ← this was the missing include
// ── Step 3: Standard library ──────────────────────────────────
#include <map>
#include <algorithm>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <fstream>

// ─────────────────────────────────────────────────────────────
//  File-scope helpers
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
                        wsToUpper(h);
                        col[h] = i;
                    }
                    const wchar_t* req[] = { L"ID", L"X", L"Y", L"Z", L"TYPE" };
                    for (int r = 0; r < 5; ++r)
                    {
                        if (col.find(req[r]) == col.end())
                            return { BridgeResult::ERR_CSV_PARSE,
                                     std::wstring(L"Nodes CSV missing column: ") + req[r] };
                    }
                    continue;
                }

                if (cells.size() < 5) continue;

                PipeNode n;
                n.id = cells[col[L"ID"]];
                n.x = std::stod(cells[col[L"X"]]);
                n.y = std::stod(cells[col[L"Y"]]);
                n.z = std::stod(cells[col[L"Z"]]);
                n.type = cells[col[L"TYPE"]];
                wsToUpper(n.type);

                if (n.id.empty())
                    return { BridgeResult::ERR_INVALID_DATA, L"Node row has empty ID" };

                outNet.nodes.push_back(std::move(n));
            }
        }

        if (outNet.nodes.empty())
            return { BridgeResult::ERR_INVALID_DATA, L"No nodes loaded from CSV" };

        // ── Segments ──────────────────────────────────────────────
        {
            std::wifstream f(segsCsvPath);
            if (!f.is_open())
                return { BridgeResult::ERR_CSV_PARSE,
                         L"Cannot open segments CSV: " + segsCsvPath };

            std::wstring line;
            bool headerDone = false;
            std::map<std::wstring, int> col;

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
                        wsToUpper(h);
                        col[h] = i;
                    }
                    const wchar_t* req[] = {
                        L"ID", L"START_NODE", L"END_NODE", L"DIAMETER_MM", L"MATERIAL"
                    };
                    for (int r = 0; r < 5; ++r)
                    {
                        if (col.find(req[r]) == col.end())
                            return { BridgeResult::ERR_CSV_PARSE,
                                     std::wstring(L"Segments CSV missing column: ") + req[r] };
                    }
                    continue;
                }

                if (cells.size() < 5) continue;

                PipeSeg s;
                s.id = cells[col[L"ID"]];
                s.startNode = cells[col[L"START_NODE"]];
                s.endNode = cells[col[L"END_NODE"]];
                s.diameter = std::stod(cells[col[L"DIAMETER_MM"]]);
                s.material = cells[col[L"MATERIAL"]];

                if (s.id.empty() || s.startNode.empty() || s.endNode.empty())
                    return { BridgeResult::ERR_INVALID_DATA,
                             L"Segment row has empty ID/start/end" };

                if (!findNode(outNet, s.startNode))
                    return { BridgeResult::ERR_INVALID_DATA,
                             L"Segment " + s.id + L" unknown start node: " + s.startNode };

                if (!findNode(outNet, s.endNode))
                    return { BridgeResult::ERR_INVALID_DATA,
                             L"Segment " + s.id + L" unknown end node: " + s.endNode };

                outNet.segments.push_back(std::move(s));
            }
        }

        if (outNet.segments.empty())
            return { BridgeResult::ERR_INVALID_DATA, L"No segments loaded from CSV" };

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

        f << L"ADDPRESSURENETWORKPIPE\n"
            << fmt(s->x) << L"," << fmt(s->y) << L"," << fmt(s->z) << L"\n"
            << fmt(e->x) << L"," << fmt(e->y) << L"," << fmt(e->z) << L"\n\n";
    }

    // ─────────────────────────────────────────────────────────────
    //  MAIN: build .scr + run via acedCommandS
    // ─────────────────────────────────────────────────────────────

    BridgeError PressureNetworkBridge::createPressureNetwork(
        const NetworkDef& net,
        const std::wstring& scriptDir)
    {
        std::wstring dir = scriptDir.empty() ? tempDir() : scriptDir;
        if (!dir.empty() && dir.back() != L'\\') dir += L'\\';
        m_lastScriptPath = dir + L"pn_" + net.name + L".scr";

        // Sanitise filename part
        for (size_t i = dir.size(); i < m_lastScriptPath.size(); ++i)
            if (m_lastScriptPath[i] == L' ') m_lastScriptPath[i] = L'_';

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

        // Wide → narrow path conversion using our forward-declared WideCharToMultiByte
        int needed = WideCharToMultiByte(CP_ACP, 0,
            m_lastScriptPath.c_str(), -1,
            NULL, 0, NULL, NULL);
        std::string pathA(static_cast<size_t>(needed), '\0');
        WideCharToMultiByte(CP_ACP, 0,
            m_lastScriptPath.c_str(), -1,
            &pathA[0], needed, NULL, NULL);
        if (!pathA.empty() && pathA.back() == '\0')
            pathA.pop_back();

        // acedCommandS — replacement for deprecated acedCommand (ARX 2021+)
        int rc = acedCommandS(RTSTR, "SCRIPT",
            RTSTR, pathA.c_str(),
            RTNONE);

        if (rc != RTNORM)
            return { BridgeResult::ERR_COMMAND_EXEC,
                     L"acedCommandS failed, code=" + std::to_wstring(rc)
                     + L"  Script: " + m_lastScriptPath };

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
            return std::wstring(buf);   // already ends with backslash
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