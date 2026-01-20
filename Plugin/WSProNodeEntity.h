#pragma once
#include "dbents.h"
#include "gept3dar.h"
#include "acstring.h"

class WSProNodeEntity : public AcDbEntity
{
public:
    ACRX_DECLARE_MEMBERS(WSProNodeEntity);

    WSProNodeEntity();
    ~WSProNodeEntity();

    void setNodeId(const AcString& id);
    void setNodeType(const AcString& type);
    void setDiameter(double d);
    void setPosition(const AcGePoint3d& pt);

    AcString nodeId() const;
    AcGePoint3d position() const;

protected:
    virtual Adesk::Boolean subWorldDraw(AcGiWorldDraw* wd) override;

private:
    AcString    m_nodeId;
    AcString    m_nodeType;
    double      m_diameter;
    AcGePoint3d m_position;
};
