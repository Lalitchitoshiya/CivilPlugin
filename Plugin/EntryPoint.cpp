#include "rxregsvc.h"
#include "WSProNodeEntity.h"
#include "Commands.h"

extern "C" AcRx::AppRetCode acrxEntryPoint(AcRx::AppMsgCode msg, void* pkt)
{
    switch (msg)
    {
    case AcRx::kInitAppMsg:
        acrxUnlockApplication(pkt);
        acrxRegisterAppMDIAware(pkt);
        WSProNodeEntity::rxInit();
		initCommands();
        acrxBuildClassHierarchy();
        break;

    case AcRx::kUnloadAppMsg:
        deleteAcRxClass(WSProNodeEntity::desc());
        break;
    }
    return AcRx::kRetOK;
}
