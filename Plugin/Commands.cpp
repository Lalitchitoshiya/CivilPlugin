// ============================================================
//  Commands.cpp
//
//  ARX SDK ships acad_windows.h which intercepts <windows.h>
//  and may strip commdlg.h (where OFN_FILEMUSTEXIST lives).
//  Fix: define everything we need explicitly before ANY include.
// ============================================================

// ── Define Windows values we need BEFORE any header can block them ──
// OFN_FILEMUSTEXIST = 0x00001000  (from commdlg.h / winuser.h)
// DWORD             = unsigned long
// These definitions are safe — they match the SDK values exactly.
//#ifndef OFN_FILEMUSTEXIST
//#define OFN_FILEMUSTEXIST  0x00001000L
//#endif
//
//#ifndef DWORD
//typedef unsigned long DWORD;
//#endif
//
//// ── Now include ARX headers (they may partially redefine Windows) ─
//#include <aced.h>
//#include <acedads.h>
//#include <adscodes.h>
//#include <acutads.h>
//
//// ── Standard library ─────────────────────────────────────────────
//#include <string>
//#include <vector>
//
//// ── Bridge ───────────────────────────────────────────────────────
//#include "PressureNetworkBridge.h"
//
//// ─────────────────────────────────────────────────────────────────
////  GetEnvironmentVariableW — declare it ourselves if <windows.h>
////  was stripped by ARX headers.  This matches the real WinAPI sig.
//// ─────────────────────────────────────────────────────────────────
//#ifndef _WINDOWS_
//extern "C"
//{
//    __declspec(dllimport) DWORD __stdcall
//        GetEnvironmentVariableW(const wchar_t* lpName,
//            wchar_t* lpBuffer,
//            DWORD          nSize);
//}
//#endif
//
//// ─────────────────────────────────────────────────────────────────
////  Helper: get env var (replaces lambda — lambdas in some ARX
////  MSVC configs cause C3536 "cannot be used before initialized")
//// ─────────────────────────────────────────────────────────────────
//static std::wstring getEnvVar(const wchar_t* name)
//{
//    wchar_t buf[1024];
//    buf[0] = L'\0';
//    DWORD rc = GetEnvironmentVariableW(name, buf, 1024);
//    if (rc > 0 && rc < 1024)
//        return std::wstring(buf);
//    return std::wstring();
//}
//
//// ─────────────────────────────────────────────────────────────────
////  Helper: prompt user to select a CSV file
//// ─────────────────────────────────────────────────────────────────
//static bool promptFilePath(const wchar_t* prompt,
//    std::wstring& outPath,
//    bool           mustExist = true)
//{
//    struct resbuf* rb = acutNewRb(RTSTR);
//    if (!rb) return false;
//
//    int flags = mustExist ? static_cast<int>(OFN_FILEMUSTEXIST) : 0;
//    int rc = acedGetFileD(prompt, NULL, L"csv", flags, rb);
//
//    bool got = (rc == RTNORM
//        && rb
//        && rb->restype == RTSTR
//        && rb->resval.rstring != NULL
//        && rb->resval.rstring[0] != L'\0');
//
//    if (got)
//        outPath = rb->resval.rstring;
//
//    acutRelRb(rb);
//    return got;
//}
//
//// ─────────────────────────────────────────────────────────────────
////  Helper: prompt for a string with optional default
//// ─────────────────────────────────────────────────────────────────
//static bool promptString(const wchar_t* prompt,
//    std::wstring& out,
//    const wchar_t* defaultVal = NULL)
//{
//    wchar_t buf[512];
//    buf[0] = L'\0';
//    int rc = acedGetString(1, prompt, buf);
//
//    if (rc == RTNORM)
//    {
//        if (buf[0] != L'\0')
//        {
//            out = buf;
//            return true;
//        }
//        if (defaultVal != NULL)
//        {
//            out = defaultVal;
//            return true;
//        }
//    }
//    return false;
//}
//
//// ═════════════════════════════════════════════════════════════════
////  COMMAND: IMPORTPRESSURENETWORK  (interactive)
//// ═════════════════════════════════════════════════════════════════
//static void cmdImportPressureNetwork()
//{
//    acutPrintf(L"\n=== Import Pressure Network from CSV ===\n");
//
//    std::wstring nodesPath;
//    if (!promptFilePath(L"Select Nodes CSV file", nodesPath))
//    {
//        acutPrintf(L"Cancelled.\n"); return;
//    }
//    acutPrintf(L"Nodes   : %s\n", nodesPath.c_str());
//
//    std::wstring segsPath;
//    if (!promptFilePath(L"Select Segments CSV file", segsPath))
//    {
//        acutPrintf(L"Cancelled.\n"); return;
//    }
//    acutPrintf(L"Segments: %s\n", segsPath.c_str());
//
//    std::wstring networkName;
//    if (!promptString(L"\nNetwork name <PressureNetwork1>: ",
//        networkName, L"PressureNetwork1"))
//    {
//        acutPrintf(L"Cancelled.\n"); return;
//    }
//
//    std::wstring partsList;
//    promptString(L"Civil 3D Parts List name <Standard>: ",
//        partsList, L"Standard");
//    if (partsList.empty()) partsList = L"Standard";
//
//    std::wstring layer;
//    promptString(L"Target layer <PRESSURE-NETWORK>: ",
//        layer, L"PRESSURE-NETWORK");
//    if (layer.empty()) layer = L"PRESSURE-NETWORK";
//
//    acutPrintf(L"\nProcessing...\n");
//
//    PNBridge::PressureNetworkBridge bridge;
//    PNBridge::BridgeError err = bridge.run(nodesPath, segsPath,
//        networkName, partsList, layer);
//    if (!err.ok())
//    {
//        acutPrintf(L"\n[ERROR] Code %d\n%s\n",
//            static_cast<int>(err.code), err.message.c_str());
//        acutPrintf(L"Pressure network was NOT created.\n");
//        return;
//    }
//
//    acutPrintf(L"\n[SUCCESS] Network \"%s\" created.\n", networkName.c_str());
//    acutPrintf(L"Check Civil 3D Prospector > Pressure Networks.\n\n");
//}
//
//// ═════════════════════════════════════════════════════════════════
////  COMMAND: IMPORTPN_SILENT  (batch / env-var driven)
////
////  Set environment variables before launching Civil 3D:
////    set PN_NODES_CSV=C:\data\nodes.csv
////    set PN_SEGS_CSV=C:\data\segments.csv
////    set PN_NETWORK_NAME=WaterMain_Zone3
////    set PN_PARTS_LIST=Standard
//// ═════════════════════════════════════════════════════════════════
//static void cmdImportPressureNetworkSilent()
//{
//    std::wstring nodesPath = getEnvVar(L"PN_NODES_CSV");
//    std::wstring segsPath = getEnvVar(L"PN_SEGS_CSV");
//    std::wstring netName = getEnvVar(L"PN_NETWORK_NAME");
//    std::wstring partsList = getEnvVar(L"PN_PARTS_LIST");
//
//    if (nodesPath.empty() || segsPath.empty() || netName.empty())
//    {
//        acutPrintf(L"[IMPORTPN_SILENT] Set these environment variables first:\n");
//        acutPrintf(L"  PN_NODES_CSV     = path to nodes.csv\n");
//        acutPrintf(L"  PN_SEGS_CSV      = path to segments.csv\n");
//        acutPrintf(L"  PN_NETWORK_NAME  = network name\n");
//        acutPrintf(L"  PN_PARTS_LIST    = parts list name (optional)\n");
//        return;
//    }
//    if (partsList.empty()) partsList = L"Standard";
//
//    PNBridge::PressureNetworkBridge bridge;
//    PNBridge::BridgeError err = bridge.run(nodesPath, segsPath,
//        netName, partsList);
//    if (!err.ok())
//        acutPrintf(L"[ERROR] %s\n", err.message.c_str());
//    else
//        acutPrintf(L"[OK] Network \"%s\" created.\n", netName.c_str());
//}
//
//// ═════════════════════════════════════════════════════════════════
////  Registration — call from your acrxEntryPoint
//// ═════════════════════════════════════════════════════════════════
//
//void registerPressureNetworkCommands()
//{
//    acedRegCmds->addCommand(
//        L"PNBRIDGE_CMDS",
//        L"IMPORTPRESSURENETWORK",
//        L"IMPORTPRESSURENETWORK",
//        ACRX_CMD_MODAL,
//        cmdImportPressureNetwork);
//
//    acedRegCmds->addCommand(
//        L"PNBRIDGE_CMDS",
//        L"IMPORTPN_SILENT",
//        L"IMPORTPN_SILENT",
//        ACRX_CMD_MODAL,
//        cmdImportPressureNetworkSilent);
//
//    acutPrintf(L"PressureNetworkBridge loaded.\n");
//    acutPrintf(L"Commands: IMPORTPRESSURENETWORK  |  IMPORTPN_SILENT\n");
//}
//
//void unregisterPressureNetworkCommands()
//{
//    acedRegCmds->removeGroup(L"PNBRIDGE_CMDS");
//}



// ============================================================
//  Commands.cpp
// ============================================================

// ── Windows values defined BEFORE any ARX header ─────────────
#ifndef OFN_FILEMUSTEXIST
#define OFN_FILEMUSTEXIST  0x00001000L
#endif

#ifndef DWORD
typedef unsigned long DWORD;
#endif

// GetEnvironmentVariableW — forward declare unconditionally
// (ARX sets _WINDOWS_ guard which blocks windows.h, so we
//  declare the function ourselves instead)
extern "C"
{
    __declspec(dllimport) DWORD __stdcall
        GetEnvironmentVariableW(const wchar_t* lpName,
            wchar_t* lpBuffer,
            DWORD          nSize);
}

// ── ARX headers ───────────────────────────────────────────────
#include <aced.h>
#include <acedads.h>
#include <adscodes.h>
#include <acutads.h>
#include <acedCmdNF.h>   // acedCommandS declaration

// ── Standard library ─────────────────────────────────────────
#include <string>
#include <vector>

// ── Bridge ───────────────────────────────────────────────────
#include "PressureNetworkBridge.h"

// ─────────────────────────────────────────────────────────────
//  Helper: get env var
// ─────────────────────────────────────────────────────────────
static std::wstring getEnvVar(const wchar_t* name)
{
    wchar_t buf[1024];
    buf[0] = L'\0';
    DWORD rc = GetEnvironmentVariableW(name, buf, 1024);
    if (rc > 0 && rc < 1024)
        return std::wstring(buf);
    return std::wstring();
}

// ─────────────────────────────────────────────────────────────
//  Helper: prompt user to select a CSV file
// ─────────────────────────────────────────────────────────────
static bool promptFilePath(const wchar_t* prompt,
    std::wstring& outPath,
    bool           mustExist = true)
{
    struct resbuf* rb = acutNewRb(RTSTR);
    if (!rb) return false;

    int flags = mustExist ? static_cast<int>(OFN_FILEMUSTEXIST) : 0;
    int rc = acedGetFileD(prompt, NULL, L"csv", flags, rb);

    bool got = (rc == RTNORM
        && rb
        && rb->restype == RTSTR
        && rb->resval.rstring != NULL
        && rb->resval.rstring[0] != L'\0');

    if (got)
        outPath = rb->resval.rstring;

    acutRelRb(rb);
    return got;
}

// ─────────────────────────────────────────────────────────────
//  Helper: prompt for a string with optional default
// ─────────────────────────────────────────────────────────────
static bool promptString(const wchar_t* prompt,
    std::wstring& out,
    const wchar_t* defaultVal = NULL)
{
    wchar_t buf[512];
    buf[0] = L'\0';
    int rc = acedGetString(1, prompt, buf);

    if (rc == RTNORM)
    {
        if (buf[0] != L'\0')
        {
            out = buf;
            return true;
        }
        if (defaultVal != NULL)
        {
            out = defaultVal;
            return true;
        }
    }
    return false;
}

// ═════════════════════════════════════════════════════════════
//  COMMAND: IMPORTPRESSURENETWORK  (interactive)
// ═════════════════════════════════════════════════════════════
static void cmdImportPressureNetwork()
{
    acutPrintf(L"\n=== Import Pressure Network from CSV ===\n");

    std::wstring nodesPath;
    if (!promptFilePath(L"Select Nodes CSV file", nodesPath))
    {
        acutPrintf(L"Cancelled.\n"); return;
    }
    acutPrintf(L"Nodes   : %s\n", nodesPath.c_str());

    std::wstring segsPath;
    if (!promptFilePath(L"Select Segments CSV file", segsPath))
    {
        acutPrintf(L"Cancelled.\n"); return;
    }
    acutPrintf(L"Segments: %s\n", segsPath.c_str());

    std::wstring networkName;
    if (!promptString(L"\nNetwork name <PressureNetwork1>: ",
        networkName, L"PressureNetwork1"))
    {
        acutPrintf(L"Cancelled.\n"); return;
    }

    std::wstring partsList;
    promptString(L"Civil 3D Parts List name <Standard>: ",
        partsList, L"Standard");
    if (partsList.empty()) partsList = L"Standard";

    std::wstring layer;
    promptString(L"Target layer <PRESSURE-NETWORK>: ",
        layer, L"PRESSURE-NETWORK");
    if (layer.empty()) layer = L"PRESSURE-NETWORK";

    acutPrintf(L"\nProcessing...\n");

    PNBridge::PressureNetworkBridge bridge;
    PNBridge::BridgeError err = bridge.run(nodesPath, segsPath,
        networkName, partsList, layer);
    if (!err.ok())
    {
        acutPrintf(L"\n[ERROR] Code %d\n%s\n",
            static_cast<int>(err.code), err.message.c_str());
        acutPrintf(L"Pressure network was NOT created.\n");
        return;
    }

    acutPrintf(L"\n[SUCCESS] Network \"%s\" created.\n", networkName.c_str());
    acutPrintf(L"Check Civil 3D Prospector > Pressure Networks.\n\n");
}

// ═════════════════════════════════════════════════════════════
//  COMMAND: IMPORTPN_SILENT  (batch / env-var driven)
// ═════════════════════════════════════════════════════════════
static void cmdImportPressureNetworkSilent()
{
    std::wstring nodesPath = getEnvVar(L"PN_NODES_CSV");
    std::wstring segsPath = getEnvVar(L"PN_SEGS_CSV");
    std::wstring netName = getEnvVar(L"PN_NETWORK_NAME");
    std::wstring partsList = getEnvVar(L"PN_PARTS_LIST");

    if (nodesPath.empty() || segsPath.empty() || netName.empty())
    {
        acutPrintf(L"[IMPORTPN_SILENT] Set these environment variables first:\n");
        acutPrintf(L"  PN_NODES_CSV     = path to nodes.csv\n");
        acutPrintf(L"  PN_SEGS_CSV      = path to segments.csv\n");
        acutPrintf(L"  PN_NETWORK_NAME  = network name\n");
        acutPrintf(L"  PN_PARTS_LIST    = parts list name (optional)\n");
        return;
    }
    if (partsList.empty()) partsList = L"Standard";

    PNBridge::PressureNetworkBridge bridge;
    PNBridge::BridgeError err = bridge.run(nodesPath, segsPath,
        netName, partsList);
    if (!err.ok())
        acutPrintf(L"[ERROR] %s\n", err.message.c_str());
    else
        acutPrintf(L"[OK] Network \"%s\" created.\n", netName.c_str());
}

// ═════════════════════════════════════════════════════════════
//  Registration
// ═════════════════════════════════════════════════════════════

void registerPressureNetworkCommands()
{
    acedRegCmds->addCommand(
        L"PNBRIDGE_CMDS",
        L"IMPORTPRESSURENETWORK",
        L"IMPORTPRESSURENETWORK",
        ACRX_CMD_MODAL,
        cmdImportPressureNetwork);

    acedRegCmds->addCommand(
        L"PNBRIDGE_CMDS",
        L"IMPORTPN_SILENT",
        L"IMPORTPN_SILENT",
        ACRX_CMD_MODAL,
        cmdImportPressureNetworkSilent);

    acutPrintf(L"PressureNetworkBridge loaded.\n");
    acutPrintf(L"Commands: IMPORTPRESSURENETWORK  |  IMPORTPN_SILENT\n");
}

void unregisterPressureNetworkCommands()
{
    acedRegCmds->removeGroup(L"PNBRIDGE_CMDS");
}