

#include "WSProPipeEntity.h"
#include "rxregsvc.h"

#define WSPROAPP L"WSPRO"

ACRX_DXF_DEFINE_MEMBERS(
    WSProPipeEntity, AcDbEntity,
    AcDb::kDHL_CURRENT, AcDb::kMReleaseCurrent,
    0, WSPROPIPEENTITY, WSPROAPP
);

WSProPipeEntity::WSProPipeEntity()
    : m_start(AcGePoint3d::kOrigin),
    m_end(AcGePoint3d::kOrigin),
    m_diameter(0.0) {
}

WSProPipeEntity::~WSProPipeEntity() {}

void WSProPipeEntity::setPipeId(const AcString& id) { m_pipeId = id; }
void WSProPipeEntity::setStartPoint(const AcGePoint3d& pt) { m_start = pt; }
void WSProPipeEntity::setEndPoint(const AcGePoint3d& pt) { m_end = pt; }
void WSProPipeEntity::setDiameter(double d) { m_diameter = d; }

Adesk::Boolean WSProPipeEntity::subWorldDraw(AcGiWorldDraw* wd)
{
    AcGePoint3d pts[2] = { m_start, m_end };
    wd->geometry().polyline(2, pts);
    return Adesk::kTrue;
}
