#include "WSProPipeEntity.h"

#include "acgi.h"
#include "dbfiler.h"
#include "rxregsvc.h"
#include <cmath> // Include cmath for mathematical functions

// Define AcGePi if not already defined
#ifndef AcGePi
constexpr double AcGePi = 3.14159265358979323846;
#endif

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

//Adesk::Boolean WSProPipeEntity::subWorldDraw(AcGiWorldDraw* wd)
//{
//    AcGiGeometry* geom = &wd->geometry();
//    if (!geom) return Adesk::kFalse;
//
//    wd->subEntityTraits().setColor(5); // blue
//
//    AcGePoint3d pts[2] = { m_startPt, m_endPt };
//    geom->polyline(2, pts);
//
//    return Adesk::kTrue;
//}

Adesk::Boolean WSProPipeEntity::subWorldDraw(AcGiWorldDraw* wd)
{
    AcGiGeometry* geom = &wd->geometry();
    if (!geom)
        return Adesk::kFalse;

    // Pipe color (blue)
    wd->subEntityTraits().setColor(5);

    // Direction vector
    AcGeVector3d axis = m_endPt - m_startPt;
    double length = axis.length();
    if (length <= 1e-6)
        return Adesk::kFalse;

    axis.normalize();

    // Create perpendicular vectors
    AcGeVector3d up = axis.perpVector().normalize();
    AcGeVector3d right = axis.crossProduct(up).normalize();

    const int slices = 12;               // smoothness
    const double radius = m_diameter / 2.0;

    AcGePoint3d vertices[slices * 2];
    Adesk::Int32 faces[slices * 5];

    for (int i = 0; i < slices; ++i)
    {
        double angle = (2.0 * AcGePi * i) / slices;
        AcGeVector3d offset =
            cos(angle) * right * radius +
            sin(angle) * up * radius;

        vertices[i] = m_startPt + offset;
        vertices[i + slices] = m_endPt + offset;

        int f = i * 5;
        int next = (i + 1) % slices;

        faces[f + 0] = 4;
        faces[f + 1] = i;
        faces[f + 2] = next;
        faces[f + 3] = next + slices;
        faces[f + 4] = i + slices;
    }

    geom->shell(
        slices * 2,           // number of vertices
        vertices,
        slices * 5,           // face list size
        faces
    );

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
