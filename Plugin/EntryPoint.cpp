#include "rxregsvc.h"
#include "aced.h"

// Custom entities
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

        // =========================================
        // REGISTER CUSTOM ENTITIES (THIS WAS MISSING)
        // =========================================
      //  WSProNodeEntity::rxInit();
        //WSProPipeEntity::rxInit();

        // Build runtime RTTI hierarchy
        //acrxBuildClassHierarchy();

        initCommands();
        break;

    case AcRx::kUnloadAppMsg:
        acedRegCmds->removeGroup(L"WSPRO_COMMANDS");
        break;
    }

    return AcRx::kRetOK;
}
