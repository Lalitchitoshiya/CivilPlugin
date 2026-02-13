#include "Domain_WSPipe.h"
#include "Import_CSVParser.h"

#include "dbsymtb.h"
#include "dbents.h"
#include "dbapserv.h"
#include "aced.h"

#include <Windows.h>

#include <vector>
#include <string>

void cmdImportPipes()
{
    wchar_t filepath[MAX_PATH];

    int result = acedGetFileD(
        L"Select Pipe CSV",
        NULL,
        L"csv",
        0,
        filepath);

    if (result != 5100)
    {
        acutPrintf(L"\nFile selection cancelled.");
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
        auto& r = rows[i];

        // Expecting:
        // ID,StartNodeID,X1,Y1,Z1,X2,Y2,Z2
        if (r.size() < 8)
            continue;

        try
        {
            double x1 = std::stod(r[2]);
            double y1 = std::stod(r[3]);
            double z1 = std::stod(r[4]);

            double x2 = std::stod(r[5]);
            double y2 = std::stod(r[6]);
            double z2 = std::stod(r[7]);

            AcDbLine* pLine = new AcDbLine(
                AcGePoint3d(x1, y1, z1),
                AcGePoint3d(x2, y2, z2));

            pModelSpace->appendAcDbEntity(pLine);
            pLine->close();
        }
        catch (...)
        {
            continue;
        }
    }

    pModelSpace->close();

    acutPrintf(L"\nPipes imported successfully.");
}