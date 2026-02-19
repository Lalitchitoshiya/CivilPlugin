//#include "EntryPoint.h"
//
//void cmdImportNodes();
//void cmdImportPipes();
//
//void cmdCreateProfile();
//
//void WSProApp::RegisterCommands()
//{
//    acedRegCmds->addCommand(
//        L"WSPRO_CMDS",
//        L"WSPROIMPORTNODES",
//        L"WSPROIMPORTNODES",
//        ACRX_CMD_MODAL,
//        cmdImportNodes);
//
//    acedRegCmds->addCommand(
//        L"WSPRO_CMDS",
//        L"WSPROIMPORTPIPES",
//        L"WSPROIMPORTPIPES",
//        ACRX_CMD_MODAL,
//        cmdImportPipes);
//
//
//    acedRegCmds->addCommand(
//        L"WSPRO_CMDS",
//        L"WSPROCREATEPROFILE",
//        L"WSPROCREATEPROFILE",
//        ACRX_CMD_MODAL,
//        cmdCreateProfile);
//}
//
//void WSProApp::UnregisterCommands()
//{
//    acedRegCmds->removeGroup(L"WSPRO_CMDS");
//}
//
//extern "C" AcRx::AppRetCode acrxEntryPoint(AcRx::AppMsgCode msg, void* pkt)
//{
//    switch (msg)
//    {
//    case AcRx::kInitAppMsg:
//        acrxUnlockApplication(pkt);
//        acrxRegisterAppMDIAware(pkt);
//        WSProApp::RegisterCommands();
//        break;
//
//    case AcRx::kUnloadAppMsg:
//        WSProApp::UnregisterCommands();
//        break;
//    }
//    return AcRx::kRetOK;
//}


#include "EntryPoint.h"

// ── Existing commands ─────────────────────────────────────────────
void cmdImportNodes();
void cmdImportPipes();
void cmdCreateProfile();

// ── Pressure Network bridge commands (from Commands.cpp) ──────────
extern void registerPressureNetworkCommands();
extern void unregisterPressureNetworkCommands();

void WSProApp::RegisterCommands()
{
    // Existing
    acedRegCmds->addCommand(
        L"WSPRO_CMDS",
        L"WSPROIMPORTNODES",
        L"WSPROIMPORTNODES",
        ACRX_CMD_MODAL,
        cmdImportNodes);

    acedRegCmds->addCommand(
        L"WSPRO_CMDS",
        L"WSPROIMPORTPIPES",
        L"WSPROIMPORTPIPES",
        ACRX_CMD_MODAL,
        cmdImportPipes);

    acedRegCmds->addCommand(
        L"WSPRO_CMDS",
        L"WSPROCREATEPROFILE",
        L"WSPROCREATEPROFILE",
        ACRX_CMD_MODAL,
        cmdCreateProfile);

    // Pressure Network — registers IMPORTPRESSURENETWORK + IMPORTPN_SILENT
    registerPressureNetworkCommands();
}

void WSProApp::UnregisterCommands()
{
    acedRegCmds->removeGroup(L"WSPRO_CMDS");
    unregisterPressureNetworkCommands();
}

extern "C" AcRx::AppRetCode acrxEntryPoint(AcRx::AppMsgCode msg, void* pkt)
{
    switch (msg)
    {
    case AcRx::kInitAppMsg:
        acrxUnlockApplication(pkt);
        acrxRegisterAppMDIAware(pkt);
        WSProApp::RegisterCommands();
        break;

    case AcRx::kUnloadAppMsg:
        WSProApp::UnregisterCommands();
        break;
    }
    return AcRx::kRetOK;
}