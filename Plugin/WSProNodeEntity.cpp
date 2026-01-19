

#include "WSProNodeEntity.h"
#include "rxregsvc.h"

#define WSPROAPP L"WSPRO"

ACRX_DXF_DEFINE_MEMBERS(
    WSProNodeEntity, AcDbEntity,
    AcDb::kDHL_CURRENT, AcDb::kMReleaseCurrent,
    0, WSPRONODEENTITY, WSPROAPP
);

WSProNodeEntity::WSProNodeEntity()
    : m_diameter(0.0), m_position(AcGePoint3d::kOrigin) {
}

WSProNodeEntity::~WSProNodeEntity() {}

void WSProNodeEntity::setNodeId(const AcString& id) { m_nodeId = id; }
void WSProNodeEntity::setNodeType(const AcString& t) { m_nodeType = t; }
void WSProNodeEntity::setDiameter(double d) { m_diameter = d; }
void WSProNodeEntity::setPosition(const AcGePoint3d& pt) { m_position = pt; }

AcString WSProNodeEntity::nodeId() const { return m_nodeId; }
AcGePoint3d WSProNodeEntity::position() const { return m_position; }

Adesk::Boolean WSProNodeEntity::subWorldDraw(AcGiWorldDraw* wd)
{
    wd->geometry().circle(m_position, 1.0, AcGeVector3d::kZAxis);
    return Adesk::kTrue;
}
