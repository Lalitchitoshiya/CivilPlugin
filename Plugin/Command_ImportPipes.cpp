#include "Domain_WSPipe.h"
#include "Import_CSVParser.h"

#include "dbsymtb.h"
#include "dbents.h"
#include "dbapserv.h"
#include "aced.h"

#include <vector>
#include <string>

void cmdImportPipes()
{
    wchar_t filepath[512];

    if (acedGetString(
        0,
        L"\nEnter full path of Pipe CSV file: ",
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
        if (rows[i].size() < 8)
            continue;

        try
        {
            double x1 = std::stod(rows[i][2]);
            double y1 = std::stod(rows[i][3]);
            double z1 = std::stod(rows[i][4]);

            double x2 = std::stod(rows[i][5]);
            double y2 = std::stod(rows[i][6]);
            double z2 = std::stod(rows[i][7]);

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