#pragma once
#pragma once

#include "dbents.h"
#include "gept3dar.h"

class WSProPipeEntity : public AcDbEntity
{
public:
    ACRX_DECLARE_MEMBERS(WSProPipeEntity);

    WSProPipeEntity();
    virtual ~WSProPipeEntity();

    void setPipeId(const AcString& id);
    AcString pipeId() const;

    void setStartPoint(const AcGePoint3d& pt);
    AcGePoint3d startPoint() const;

    void setEndPoint(const AcGePoint3d& pt);
    AcGePoint3d endPoint() const;

    void setDiameter(double dia);
    double diameter() const;

    virtual Adesk::Boolean subWorldDraw(AcGiWorldDraw* wd) override;

protected:
    virtual Acad::ErrorStatus dwgOutFields(AcDbDwgFiler* filer) const override;
    virtual Acad::ErrorStatus dwgInFields(AcDbDwgFiler* filer) override;

private:
    AcString    m_pipeId;
    AcGePoint3d m_startPt;
    AcGePoint3d m_endPt;
    double      m_diameter;
};
