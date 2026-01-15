#pragma once

#include "dbents.h"
#include "gept3dar.h"
#include "dbsymtb.h"

class WSProNodeEntity : public AcDbEntity
{
public:
    ACRX_DECLARE_MEMBERS(WSProNodeEntity);

    WSProNodeEntity();
    virtual ~WSProNodeEntity();

    // ---- WS Pro Properties ----
    void setNodeId(const AcString& id);
    AcString nodeId() const;

    void setPosition(const AcGePoint3d& pt);
    AcGePoint3d position() const;

    void setDiameter(double dia);
    double diameter() const;

    void setNodeType(const AcString& type);
    AcString nodeType() const;

    // ---- Drawing Overrides ----
    virtual Adesk::Boolean subWorldDraw(AcGiWorldDraw* wd) override;
    virtual void subViewportDraw(AcGiViewportDraw* vd) override;

protected:
    // ---- DWG I/O ----
    virtual Acad::ErrorStatus dwgOutFields(AcDbDwgFiler* filer) const override;
    virtual Acad::ErrorStatus dwgInFields(AcDbDwgFiler* filer) override;

private:
    AcString     m_nodeId;
    AcGePoint3d  m_position;
    double       m_diameter;
    AcString     m_nodeType;
};
