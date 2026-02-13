#include "EntryPoint.h"

void cmdImportNodes();
void cmdImportPipes();

void WSProApp::RegisterCommands()
{
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
}

void WSProApp::UnregisterCommands()
{
    acedRegCmds->removeGroup(L"WSPRO_CMDS");
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