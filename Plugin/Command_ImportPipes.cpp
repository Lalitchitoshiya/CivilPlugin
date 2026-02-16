#include <Windows.h>
#include <commdlg.h>

#include "Domain_WSPipe.h"
#include "Import_CSVParser.h"
#include "Global_Data.h"

#include "dbsymtb.h"
#include "dbents.h"
#include "dbapserv.h"
#include "aced.h"

#include <vector>
#include <string>

void cmdImportPipes()
{
    if (g_NodeMap.empty())
    {
        acutPrintf(L"\nNo nodes found. Import nodes first.");
        return;
    }

    wchar_t filePath[MAX_PATH] = { 0 };

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Select Pipe CSV File";

    if (!GetOpenFileNameW(&ofn))
    {
        acutPrintf(L"\nFile selection cancelled.");
        return;
    }

    std::wstring ws(filePath);
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
        if (rows[i].size() < 5)
            continue;

        try
        {
            int usNode = std::stoi(rows[i][3]);
            int dsNode = std::stoi(rows[i][4]);

            if (g_NodeMap.find(usNode) == g_NodeMap.end())
                continue;

            if (g_NodeMap.find(dsNode) == g_NodeMap.end())
                continue;

            AcGePoint3d pt1 = g_NodeMap[usNode];
            AcGePoint3d pt2 = g_NodeMap[dsNode];

            AcDbLine* pLine = new AcDbLine(pt1, pt2);
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