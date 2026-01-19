//#pragma once
//
//#include "dbents.h"
//#include "gept3dar.h"
//
//class WSProNodeEntity : public AcDbEntity
//{
//public:
//    ACRX_DECLARE_MEMBERS(WSProNodeEntity);
//
//    WSProNodeEntity();
//    virtual ~WSProNodeEntity();
//
//    // WS Pro properties
//    void setNodeId(const AcString& id);
//    AcString nodeId() const;
//
//    void setNodeType(const AcString& type);
//    AcString nodeType() const;
//
//    void setDiameter(double dia);
//    double diameter() const;
//
//    void setPosition(const AcGePoint3d& pt);
//    AcGePoint3d position() const;
//
//    // Drawing
//    virtual Adesk::Boolean subWorldDraw(AcGiWorldDraw* wd) override;
//    virtual void subViewportDraw(AcGiViewportDraw* vd) override;
//
//protected:
//    // DWG I/O
//    virtual Acad::ErrorStatus dwgOutFields(AcDbDwgFiler* filer) const override;
//    virtual Acad::ErrorStatus dwgInFields(AcDbDwgFiler* filer) override;
//
//private:
//    AcString    m_nodeId;
//    AcString    m_nodeType;
//    AcGePoint3d m_position;
//    double      m_diameter;
//};



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
