#include "WSProPipeEntity.h"

#include "acgi.h"
#include "dbfiler.h"
#include "rxregsvc.h"

ACRX_DXF_DEFINE_MEMBERS(
    WSProPipeEntity,
    AcDbEntity,
    AcDb::kDHL_CURRENT,
    AcDb::kMReleaseCurrent,
    0,
    WSPROPIPEENTITY,
    WSProEntities
)

WSProPipeEntity::WSProPipeEntity()
    : m_startPt(AcGePoint3d::kOrigin),
    m_endPt(AcGePoint3d::kOrigin),
    m_diameter(100.0)
{
}

WSProPipeEntity::~WSProPipeEntity() {}

void WSProPipeEntity::setPipeId(const AcString& id) { m_pipeId = id; }
AcString WSProPipeEntity::pipeId() const { return m_pipeId; }

void WSProPipeEntity::setStartPoint(const AcGePoint3d& pt) { m_startPt = pt; }
AcGePoint3d WSProPipeEntity::startPoint() const { return m_startPt; }

void WSProPipeEntity::setEndPoint(const AcGePoint3d& pt) { m_endPt = pt; }
AcGePoint3d WSProPipeEntity::endPoint() const { return m_endPt; }

void WSProPipeEntity::setDiameter(double dia) { m_diameter = dia; }
double WSProPipeEntity::diameter() const { return m_diameter; }

Adesk::Boolean WSProPipeEntity::subWorldDraw(AcGiWorldDraw* wd)
{
    AcGiGeometry* geom = &wd->geometry();
    if (!geom) return Adesk::kFalse;

    wd->subEntityTraits().setColor(5); // blue

    AcGePoint3d pts[2] = { m_startPt, m_endPt };
    geom->polyline(2, pts);

    return Adesk::kTrue;
}

Acad::ErrorStatus WSProPipeEntity::dwgOutFields(AcDbDwgFiler* filer) const
{
    Acad::ErrorStatus es = AcDbEntity::dwgOutFields(filer);
    if (es != Acad::eOk) return es;

    filer->writeString(m_pipeId);
    filer->writePoint3d(m_startPt);
    filer->writePoint3d(m_endPt);
    filer->writeDouble(m_diameter);

    return filer->filerStatus();
}

Acad::ErrorStatus WSProPipeEntity::dwgInFields(AcDbDwgFiler* filer)
{
    Acad::ErrorStatus es = AcDbEntity::dwgInFields(filer);
    if (es != Acad::eOk) return es;

    filer->readString(m_pipeId);
    filer->readPoint3d(&m_startPt);
    filer->readPoint3d(&m_endPt);
    filer->readDouble(&m_diameter);

    return filer->filerStatus();
}
