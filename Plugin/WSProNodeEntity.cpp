//#include "aced.h"
//#include "dbsymtb.h"
//#include "dbapserv.h"
//#include "acdb.h"
//#include "WSProNodeEntity.h"
//
//void createWSProNode()
//{
//    // -------------------------------
//    // Step A1 — Ask user for location
//    // -------------------------------
//    ads_point pt;
//    if (acedGetPoint(nullptr, L"\nSpecify WS Pro node location: ", pt) != RTNORM)
//        return;
//
//    // -------------------------------
//    // Step A2 — Ask user for node type
//    // -------------------------------
//    ACHAR type[32];
//    if (acedGetString(Adesk::kFalse,
//        L"\nEnter node type [Valve/Junction]: ",
//        type) != RTNORM)
//        return;
//
//    // -------------------------------
//    // Step A3 — Ask user for diameter
//    // -------------------------------
//    double dia = 0.0;
//    if (acedGetReal(L"\nEnter diameter: ", &dia) != RTNORM || dia <= 0.0)
//        return;
//
//    // -------------------------------
//    // Create WS Pro Node Entity
//    // -------------------------------
//    WSProNodeEntity* pNode = new WSProNodeEntity();
//
//    pNode->setNodeId(L"USER_NODE");   // placeholder ID
//    pNode->setNodeType(type);
//    pNode->setDiameter(dia);
//    pNode->setPosition(AcGePoint3d(pt[0], pt[1], pt[2]));
//
//    // -------------------------------
//    // Append to ModelSpace
//    // -------------------------------
//    AcDbBlockTable* pBT = nullptr;
//    acdbHostApplicationServices()
//        ->workingDatabase()
//        ->getBlockTable(pBT, AcDb::kForRead);
//
//    AcDbBlockTableRecord* pMS = nullptr;
//    pBT->getAt(ACDB_MODEL_SPACE, pMS, AcDb::kForWrite);
//
//    AcDbObjectId id;
//    pMS->appendAcDbEntity(id, pNode);
//
//    pNode->close();
//    pMS->close();
//    pBT->close();
//}



#include "WSProNodeEntity.h"

#include "acgi.h"
#include "dbfiler.h"
#include "rxregsvc.h"

// --------------------------------------------------------------------------
// Runtime type registration
// --------------------------------------------------------------------------
ACRX_DXF_DEFINE_MEMBERS(
    WSProNodeEntity,
    AcDbEntity,
    AcDb::kDHL_CURRENT,
    AcDb::kMReleaseCurrent,
    0,
    WSPRONODEENTITY,
    WSProEntities
)

// --------------------------------------------------------------------------
// Constructor / Destructor
// --------------------------------------------------------------------------
WSProNodeEntity::WSProNodeEntity()
    : m_position(AcGePoint3d::kOrigin),
    m_diameter(100.0)
{
}

WSProNodeEntity::~WSProNodeEntity()
{
}

// --------------------------------------------------------------------------
// Setters / Getters
// --------------------------------------------------------------------------
void WSProNodeEntity::setNodeId(const AcString& id)
{
    m_nodeId = id;
}

AcString WSProNodeEntity::nodeId() const
{
    return m_nodeId;
}

void WSProNodeEntity::setNodeType(const AcString& type)
{
    m_nodeType = type;
}

AcString WSProNodeEntity::nodeType() const
{
    return m_nodeType;
}

void WSProNodeEntity::setDiameter(double dia)
{
    m_diameter = dia;
}

double WSProNodeEntity::diameter() const
{
    return m_diameter;
}

void WSProNodeEntity::setPosition(const AcGePoint3d& pt)
{
    m_position = pt;
}

AcGePoint3d WSProNodeEntity::position() const
{
    return m_position;
}

// --------------------------------------------------------------------------
// Drawing logic (semantic visualization)
// --------------------------------------------------------------------------
Adesk::Boolean WSProNodeEntity::subWorldDraw(AcGiWorldDraw* wd)
{
    AcGiGeometry* geom = &wd->geometry();
    if (!geom)
        return Adesk::kFalse;

    // ------------------------------------------------------
    // Color by node type (semantic meaning)
    // ------------------------------------------------------
    if (m_nodeType == L"Valve")
        wd->subEntityTraits().setColor(1);      // Red
    else if (m_nodeType == L"Junction")
        wd->subEntityTraits().setColor(3);      // Green
    else
        wd->subEntityTraits().setColor(7);      // White / default

    // ------------------------------------------------------
    // Shape by node type
    // ------------------------------------------------------
    if (m_nodeType == L"Valve")
    {
        geom->circle(
            m_position,
            m_diameter / 2.0,
            AcGeVector3d::kZAxis
        );
    }
    else if (m_nodeType == L"Junction")
    {
        geom->circle(
            m_position,
            m_diameter / 4.0,
            AcGeVector3d::kZAxis
        );
    }
    else
    {
        // Default fallback visualization
        geom->circle(
            m_position,
            m_diameter / 3.0,
            AcGeVector3d::kZAxis
        );
    }

    // ------------------------------------------------------
    // Optional label (Node ID)
    // ------------------------------------------------------
    if (!m_nodeId.isEmpty())
    {
        geom->text(
            m_position + AcGeVector3d(0, m_diameter, 0),
            AcGeVector3d::kZAxis,
            AcGeVector3d::kXAxis,
            m_diameter / 3.0,
            0.0,
            0.0,
            m_nodeId.kwszPtr()
        );
    }

    return Adesk::kTrue;
}

// --------------------------------------------------------------------------
// Optional viewport-specific drawing (not used yet)
// --------------------------------------------------------------------------
void WSProNodeEntity::subViewportDraw(AcGiViewportDraw* /*vd*/)
{
    // Intentionally empty
}

// --------------------------------------------------------------------------
// DWG save (persistence)
// --------------------------------------------------------------------------
Acad::ErrorStatus WSProNodeEntity::dwgOutFields(AcDbDwgFiler* filer) const
{
    Acad::ErrorStatus es = AcDbEntity::dwgOutFields(filer);
    if (es != Acad::eOk)
        return es;

    filer->writeString(m_nodeId);
    filer->writeString(m_nodeType);
    filer->writePoint3d(m_position);
    filer->writeDouble(m_diameter);

    return filer->filerStatus();
}

// --------------------------------------------------------------------------
// DWG load (persistence)
// --------------------------------------------------------------------------
Acad::ErrorStatus WSProNodeEntity::dwgInFields(AcDbDwgFiler* filer)
{
    Acad::ErrorStatus es = AcDbEntity::dwgInFields(filer);
    if (es != Acad::eOk)
        return es;

    filer->readString(m_nodeId);
    filer->readString(m_nodeType);
    filer->readPoint3d(&m_position);
    filer->readDouble(&m_diameter);

    return filer->filerStatus();
}
