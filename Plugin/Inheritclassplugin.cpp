//#include "dbents.h"     // AcDbEntity
//#include "dbmain.h"
//#include "rxobject.h"
//#include "rxregsvc.h"
//#include "acgi.h"
//#include "adscodes.h"
//
//
////#include "acrxentrypoint.h"
//
//
//// =======================================================
//// ObjectARX – Single File Custom Entity Example (C++)
//// =======================================================
//
//
//// =======================================================
//// WS PRO CUSTOM ENTITY
//// =======================================================
//
//class WsProNode : public AcDbEntity
//{
//public:
//    ACRX_DECLARE_MEMBERS(WsProNode);
//
//    WsProNode();
//    virtual ~WsProNode();
//
//    // --- Persistence ---
//    virtual Acad::ErrorStatus dwgOutFields(AcDbDwgFiler* filer) const override;
//    virtual Acad::ErrorStatus dwgInFields(AcDbDwgFiler* filer) override;
//
//    // --- Graphics ---
//    virtual Adesk::Boolean worldDraw(AcGiWorldDraw* wd) override;
//
//    // --- WS Pro properties ---
//    void setNodeId(const ACHAR* id);
//    void setDiameter(double d);
//
//private:
//    ACHAR  m_nodeId[64];
//    double m_diameter;
//};
////
////// =======================================================
////// RX REGISTRATION
////// =======================================================
////
////ACRX_DXF_DEFINE_MEMBERS(
////    WsProNode,
////    AcDbEntity,
////    AcDb::kDHL_CURRENT,
////    AcDb::kMReleaseCurrent,
////    0,
////    WSPRONODE,
////    WSPro
////)
////
////// =======================================================
////// ENTITY IMPLEMENTATION
////// =======================================================
////
////WsProNode::WsProNode()
////{
////    wcscpy_s(m_nodeId, L"NODE-001");
////    m_diameter = 500.0;
////}
////
////WsProNode::~WsProNode() {}
////
////void WsProNode::setNodeId(const ACHAR* id)
////{
////    wcscpy_s(m_nodeId, id);
////}
////
////void WsProNode::setDiameter(double d)
////{
////    m_diameter = d;
////}
////
////// ---------- DWG SAVE ----------
////
////Acad::ErrorStatus WsProNode::dwgOutFields(AcDbDwgFiler* filer) const
////{
////    Acad::ErrorStatus es = AcDbEntity::dwgOutFields(filer);
////    if (es != Acad::eOk) return es;
////
////    filer->writeString(m_nodeId);
////    filer->writeDouble(m_diameter);
////
////    return filer->filerStatus();
////}
////
////// ---------- DWG LOAD ----------
////
////Acad::ErrorStatus WsProNode::dwgInFields(AcDbDwgFiler* filer)
////{
////    Acad::ErrorStatus es = AcDbEntity::dwgInFields(filer);
////    if (es != Acad::eOk) return es;
////
////    filer->readString(m_nodeId);
////    filer->readDouble(&m_diameter);
////
////    return filer->filerStatus();
////}
////
////// ---------- GRAPHICS ----------
////
////Adesk::Boolean WsProNode::worldDraw(AcGiWorldDraw* wd)
////{
////    AcGePoint3d center(0.0, 0.0, 0.0);
////    double radius = m_diameter / 2.0;
////
////    wd->geometry().circle(
////        center,
////        radius,
////        AcGeVector3d::kZAxis
////    );
////
////    return Adesk::kTrue;
////}
////
////// =======================================================
////// COMMAND: CREATE WS PRO NODE
////// =======================================================
////
////void cmdCreateWsProNode()
////{
////    AcDbBlockTable* pBT = nullptr;
////    acdbHostApplicationServices()->workingDatabase()
////        ->getBlockTable(pBT, AcDb::kForRead);
////
////    AcDbBlockTableRecord* pMS = nullptr;
////    pBT->getAt(ACDB_MODEL_SPACE, pMS, AcDb::kForWrite);
////    pBT->close();
////
////    WsProNode* pNode = new WsProNode();
////    pNode->setNodeId(L"WSPRO-NODE-1001");
////    pNode->setDiameter(800.0);
////
////    pMS->appendAcDbEntity(pNode);
////
////    pNode->close();
////    pMS->close();
////}
////
////// =======================================================
////// ARX APPLICATION
////// =======================================================
////
////class PluginApp : public AcRxArxApp
////{
////public:
////    PluginApp() : AcRxArxApp() {}
////
////    virtual AcRx::AppRetCode On_kInitAppMsg(void* pkt)
////    {
////        AcRxArxApp::On_kInitAppMsg(pkt);
////
////        WsProNode::rxInit();
////        acrxBuildClassHierarchy();
////
////        acedRegCmds->addCommand(
////            L"WSProCmds",
////            L"WSPRONODE",
////            L"WSPRONODE",
////            ACRX_CMD_MODAL,
////            cmdCreateWsProNode
////        );
////
////        acutPrintf(L"\nWSPro ARX Loaded.");
////        return AcRx::kRetOK;
////    }
////
////    virtual AcRx::AppRetCode On_kUnloadAppMsg(void* pkt)
////    {
////        acedRegCmds->removeGroup(L"WSProCmds");
////        deleteAcRxClass(WsProNode::desc());
////
////        acutPrintf(L"\nWSPro ARX Unloaded.");
////        return AcRxArxApp::On_kUnloadAppMsg(pkt);
////    }
////};
////
////// =======================================================
////// ENTRY POINT (MANDATORY)
////// =======================================================
////
////ACRX_ENTRYPOINT(PluginApp)



// ======================================================
// Single-file ObjectARX Plugin with Custom AcDbEntity
// ======================================================

//#include "acrxentrypoint.h"
//#include "aced.h"
//#include "rxregsvc.h"
//
//#include "dbents.h"
//#include "dbsymtb.h"
//#include "dbapserv.h"
//#include "acgi.h"

// ======================================================
// Custom WS Pro Entity
// ======================================================

//class WsProNode : public AcDbEntity
//{
//public:
//    ACRX_DECLARE_MEMBERS(WsProNode);
//
//    WsProNode();
//    virtual ~WsProNode();
//
//    // DWG persistence
//    virtual Acad::ErrorStatus dwgOutFields(AcDbDwgFiler* filer) const override;
//    virtual Acad::ErrorStatus dwgInFields(AcDbDwgFiler* filer) override;

    // Graphics
    //virtual Adesk::Boolean worldDraw(AcGiWorldDraw* wd) override;

//private:
//    double m_diameter;
//};
//
//// ======================================================
//// RX Registration
//// ======================================================
//
//ACRX_DXF_DEFINE_MEMBERS(
//    WsProNode,
//    AcDbEntity,
//    AcDb::kDHL_CURRENT,
//    AcDb::kMReleaseCurrent,
//    0,
//    WSPRONODE,
//    WSPro
//)

// ======================================================
// Entity Implementation
// ======================================================

//WsProNode::WsProNode()
//{
//    m_diameter = 500.0;
//}
//
//WsProNode::~WsProNode() {}
//
//Acad::ErrorStatus WsProNode::dwgOutFields(AcDbDwgFiler* filer) const
//{
//    Acad::ErrorStatus es = AcDbEntity::dwgOutFields(filer);
//    if (es != Acad::eOk) return es;
//
//    filer->writeDouble(m_diameter);
//    return filer->filerStatus();
//}
//
//Acad::ErrorStatus WsProNode::dwgInFields(AcDbDwgFiler* filer)
//{
//    Acad::ErrorStatus es = AcDbEntity::dwgInFields(filer);
//    if (es != Acad::eOk) return es;
//
//    filer->readDouble(&m_diameter);
//    return filer->filerStatus();
//}

//Adesk::Boolean WsProNode::worldDraw(AcGiWorldDraw* wd)
//{
//    AcGePoint3d center(0.0, 0.0, 0.0);
//    double radius = m_diameter / 2.0;
//
//    wd->geometry().circle(
//        center,
//        radius,
//        AcGeVector3d::kZAxis
//    );
//
//    return Adesk::kTrue;
//}

// ======================================================
// Command to Create Entity
// ======================================================

//void cmdCreateWsProNode()
//{
//    AcDbDatabase* db = acdbHostApplicationServices()->workingDatabase();
//
//    AcDbBlockTable* pBT = nullptr;
//    db->getBlockTable(pBT, AcDb::kForRead);
//
//    AcDbBlockTableRecord* pMS = nullptr;
//    pBT->getAt(ACDB_MODEL_SPACE, pMS, AcDb::kForWrite);
//    pBT->close();
//
//    WsProNode* pEnt = new WsProNode();
//    pMS->appendAcDbEntity(pEnt);
//
//    pEnt->close();
//    pMS->close();
//}

// ======================================================
// ARX Application Class
// ======================================================

//class PluginApp : public AcRxArxApp
//{
//public:
//    PluginApp() : AcRxArxApp() {}
//
//    virtual AcRx::AppRetCode On_kInitAppMsg(void* pkt)
//    {
//        AcRxArxApp::On_kInitAppMsg(pkt);
//
//        WsProNode::rxInit();
//        acrxBuildClassHierarchy();
//
//        acedRegCmds->addCommand(
//            L"WSProCmds",
//            L"WSPRONODE",
//            L"WSPRONODE",
//            ACRX_CMD_MODAL,
//            cmdCreateWsProNode
//        );
//
//        acutPrintf(L"\nWSPro ARX Loaded.");
//        return AcRx::kRetOK;
//    }
//
//    virtual AcRx::AppRetCode On_kUnloadAppMsg(void* pkt)
//    {
//        acedRegCmds->removeGroup(L"WSProCmds");
//        deleteAcRxClass(WsProNode::desc());
//
//        acutPrintf(L"\nWSPro ARX Unloaded.");
//        return AcRxArxApp::On_kUnloadAppMsg(pkt);
//    }
//};
//
//// ======================================================
//// Entry Point (MANDATORY)
//// ======================================================
//
//ACRX_ENTRYPOINT(PluginApp)
