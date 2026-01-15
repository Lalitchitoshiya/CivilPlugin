#include "rxregsvc.h"
#include "aced.h"

#include "WSProNodeEntity.h"
#include "WSProPipeEntity.h"

void initCommands();

extern "C"
AcRx::AppRetCode acrxEntryPoint(AcRx::AppMsgCode msg, void* pkt)
{
    switch (msg)
    {
    case AcRx::kInitAppMsg:
        acrxUnlockApplication(pkt);
        acrxRegisterAppMDIAware(pkt);

        WSProNodeEntity::rxInit();
        WSProPipeEntity::rxInit();

        initCommands();
        acrxBuildClassHierarchy();
        break;

    case AcRx::kUnloadAppMsg:
        acedRegCmds->removeGroup(L"WSPRO_COMMANDS");
        deleteAcRxClass(WSProPipeEntity::desc());
        deleteAcRxClass(WSProNodeEntity::desc());
        break;
    }
    return AcRx::kRetOK;
}
