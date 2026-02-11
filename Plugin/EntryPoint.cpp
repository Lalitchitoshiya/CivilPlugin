#include "rxregsvc.h"
#include "aced.h"

void initCommands();

extern "C"
AcRx::AppRetCode acrxEntryPoint(AcRx::AppMsgCode msg, void* pkt)
{
    switch (msg)
    {
    case AcRx::kInitAppMsg:
        acrxUnlockApplication(pkt);
        acrxRegisterAppMDIAware(pkt);
        initCommands();
        break;

    case AcRx::kUnloadAppMsg:
        acedRegCmds->removeGroup(L"WSPRO_COMMANDS");
        break;
    }

    return AcRx::kRetOK;
}
