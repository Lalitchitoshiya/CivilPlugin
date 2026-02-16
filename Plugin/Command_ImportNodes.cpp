#include "Domain_WSNode.h"
#include "Import_CSVParser.h"

#include "dbsymtb.h"
#include "dbents.h"
#include "dbapserv.h"
#include "aced.h"

#include <vector>
#include <string>

void cmdImportNodes()
{
    wchar_t filepath[512];

    if (acedGetString(
        0,
        L"\nEnter full path of Node CSV file: ",
        filepath) != Acad::eOk)
    {
        acutPrintf(L"\nFile input cancelled.");
        return;
    }

    std::wstring ws(filepath);
    std::string path(ws.begin(), ws.end());

    auto rows = CSVParser::Parse(path);

    if (rows.size() <= 1)
    {
        acutPrintf(L"\nNo data found in file.");
        return;
    }

    AcDbDatabase* pDb = acdbHostApplicationServices()->workingDatabase();

    AcDbBlockTable* pBlockTable = nullptr;
    pDb->getBlockTable(pBlockTable, AcDb::kForRead);

    AcDbBlockTableRecord* pModelSpace = nullptr;
    pBlockTable->getAt(ACDB_MODEL_SPACE, pModelSpace, AcDb::kForWrite);
    pBlockTable->close();

    for (size_t i = 1; i < rows.size(); ++i)
    {
        if (rows[i].size() < 4)
            continue;

        try
        {
            double x = std::stod(rows[i][1]);
            double y = std::stod(rows[i][2]);
            double z = std::stod(rows[i][3]);

            AcDbPoint* pPoint = new AcDbPoint(AcGePoint3d(x, y, z));
            pModelSpace->appendAcDbEntity(pPoint);
            pPoint->close();
        }
        catch (...)
        {
            continue;
        }
    }

    pModelSpace->close();
    acutPrintf(L"\nNodes imported successfully.");
}