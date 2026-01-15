
#include "WSProNodeEntity.h"

#include "acgi.h"
#include "dbfiler.h"
#include "rxregsvc.h"

ACRX_DXF_DEFINE_MEMBERS(
    WSProNodeEntity,
    AcDbEntity,
    AcDb::kDHL_CURRENT,
    AcDb::kMReleaseCurrent,
    0,
    WSPRONODEENTITY,
    WSProEntities
)

WSProNodeEntity::WSProNodeEntity()
    : m_position(AcGePoint3d::kOrigin),
    m_diameter(100.0)
{
}

WSProNodeEntity::~WSProNodeEntity() {}

void WSProNodeEntity::setNodeId(const AcString& id) { m_nodeId = id; }
AcString WSProNodeEntity::nodeId() const { return m_nodeId; }

void WSProNodeEntity::setNodeType(const AcString& type) { m_nodeType = type; }
AcString WSProNodeEntity::nodeType() const { return m_nodeType; }

void WSProNodeEntity::setDiameter(double dia) { m_diameter = dia; }
double WSProNodeEntity::diameter() const { return m_diameter; }

void WSProNodeEntity::setPosition(const AcGePoint3d& pt) { m_position = pt; }
AcGePoint3d WSProNodeEntity::position() const { return m_position; }

Adesk::Boolean WSProNodeEntity::subWorldDraw(AcGiWorldDraw* wd)
{
    AcGiGeometry* geom = &wd->geometry();
    if (!geom) return Adesk::kFalse;

    // Color by type
    if (m_nodeType == L"Valve")
        wd->subEntityTraits().setColor(1); // red
    else if (m_nodeType == L"Junction")
        wd->subEntityTraits().setColor(3); // green
    else
        wd->subEntityTraits().setColor(7); // white

    double radius =
        (m_nodeType == L"Valve") ? m_diameter / 2.0 :
        (m_nodeType == L"Junction") ? m_diameter / 4.0 :
        m_diameter / 3.0;

    geom->circle(m_position, radius, AcGeVector3d::kZAxis);

    if (!m_nodeId.isEmpty())
    {
        geom->text(
            m_position + AcGeVector3d(0, m_diameter, 0),
            AcGeVector3d::kZAxis,
            AcGeVector3d::kXAxis,
            m_diameter / 3.0,
            1.0,
            0.0,
            m_nodeId.kwszPtr()
        );
    }

    return Adesk::kTrue;
}

void WSProNodeEntity::subViewportDraw(AcGiViewportDraw*) {}

Acad::ErrorStatus WSProNodeEntity::dwgOutFields(AcDbDwgFiler* filer) const
{
    Acad::ErrorStatus es = AcDbEntity::dwgOutFields(filer);
    if (es != Acad::eOk) return es;

    filer->writeString(m_nodeId);
    filer->writeString(m_nodeType);
    filer->writePoint3d(m_position);
    filer->writeDouble(m_diameter);

    return filer->filerStatus();
}

Acad::ErrorStatus WSProNodeEntity::dwgInFields(AcDbDwgFiler* filer)
{
    Acad::ErrorStatus es = AcDbEntity::dwgInFields(filer);
    if (es != Acad::eOk) return es;

    filer->readString(m_nodeId);
    filer->readString(m_nodeType);
    filer->readPoint3d(&m_position);
    filer->readDouble(&m_diameter);

    return filer->filerStatus();
}
