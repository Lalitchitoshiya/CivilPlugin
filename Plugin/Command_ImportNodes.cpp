#include "Domain_WSNode.h"
#include "Import_CSVParser.h"

#include "dbsymtb.h"
#include "dbents.h"
#include "dbapserv.h"
#include "aced.h"

#include <Windows.h>

#include <vector>
#include <string>

void cmdImportNodes()
{
    wchar_t filepath[MAX_PATH];

    int acedGetFileD(
        const ACHAR * prompt,
        const ACHAR * def,
        const ACHAR * ext,
        int flags,
        resbuf * rb
    );

    // 5100 == RTNORM (success)
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

        if (r.size() < 4)
            continue;

        try
        {
            double x = std::stod(r[1]);
            double y = std::stod(r[2]);
            double z = std::stod(r[3]);

            AcDbPoint* pPoint = new AcDbPoint(AcGePoint3d(x, y, z));
            pModelSpace->appendAcDbEntity(pPoint);
            pPoint->close();
        }
        catch (...)
        {
            // Skip invalid numeric rows
            continue;
        }
    }

    pModelSpace->close();

    acutPrintf(L"\nNodes imported successfully.");
}