// ============================================================
//  Commands.cpp
//  Registered commands:
//    WSPROIMPORT  — main import command (reads nodes + pipes CSV)
// ============================================================

#ifndef OFN_FILEMUSTEXIST
#define OFN_FILEMUSTEXIST  0x00001000L
#endif

#ifndef DWORD
typedef unsigned long DWORD;
#endif

// ── ARX headers ──────────────────────────────────────────────
#include <aced.h>
#include <acedads.h>
#include <adscodes.h>
#include <acutads.h>

// ── Standard library ─────────────────────────────────────────
#include <string>

// ── Forward declaration — implemented in Command_ImportNetwork.cpp ──
extern void cmdWSProImport();

// ═════════════════════════════════════════════════════════════
//  Registration — called from EntryPoint.cpp
// ═════════════════════════════════════════════════════════════

void registerPressureNetworkCommands()
{
    acedRegCmds->addCommand(
        L"WSPRO_CMDS",
        L"WSPROIMPORT",
        L"WSPROIMPORT",
        ACRX_CMD_MODAL,
        cmdWSProImport);

    acutPrintf(L"WS Pro Plugin loaded.\n");
    acutPrintf(L"Type WSPROIMPORT to import pipes and nodes from CSV.\n");
}

void unregisterPressureNetworkCommands()
{
    acedRegCmds->removeGroup(L"WSPRO_CMDS");
}
