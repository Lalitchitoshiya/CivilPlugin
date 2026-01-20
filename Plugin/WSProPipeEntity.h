#pragma once
#include "dbents.h"
#include "gept3dar.h"
#include "acstring.h"

class WSProPipeEntity : public AcDbEntity
{
public:
    ACRX_DECLARE_MEMBERS(WSProPipeEntity);

    WSProPipeEntity();
    ~WSProPipeEntity();

    void setPipeId(const AcString& id);
    void setStartPoint(const AcGePoint3d& pt);
    void setEndPoint(const AcGePoint3d& pt);
    void setDiameter(double d);

protected:
    virtual Adesk::Boolean subWorldDraw(AcGiWorldDraw* wd) override;

private:
    AcString     m_pipeId;
    AcGePoint3d  m_start;
    AcGePoint3d  m_end;
    double       m_diameter;
};
