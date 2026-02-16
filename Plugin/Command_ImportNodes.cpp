//#include <Windows.h>
//#include <commdlg.h>
//
//#include "Domain_WSNode.h"
//#include "Import_CSVParser.h"
//
//#include "dbsymtb.h"
//#include "dbents.h"
//#include "dbapserv.h"
//#include "aced.h"
//
//#include <vector>
//#include <string>
//
//void cmdImportNodes()
//{
//    wchar_t filePath[MAX_PATH] = { 0 };
//
//    OPENFILENAMEW ofn;
//    ZeroMemory(&ofn, sizeof(ofn));
//
//    ofn.lStructSize = sizeof(ofn);
//    ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0";
//    ofn.lpstrFile = filePath;
//    ofn.nMaxFile = MAX_PATH;
//    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
//    ofn.lpstrTitle = L"Select Node CSV File";
//
//    if (!GetOpenFileNameW(&ofn))
//    {
//        acutPrintf(L"\nFile selection cancelled.");
//        return;
//    }
//
//    std::wstring ws(filePath);
//    std::string path(ws.begin(), ws.end());
//
//    auto rows = CSVParser::Parse(path);
//
//    if (rows.size() <= 1)
//    {
//        acutPrintf(L"\nNo data found in file.");
//        return;
//    }
//
//    AcDbDatabase* pDb = acdbHostApplicationServices()->workingDatabase();
//
//    AcDbBlockTable* pBlockTable = nullptr;
//    pDb->getBlockTable(pBlockTable, AcDb::kForRead);
//
//    AcDbBlockTableRecord* pModelSpace = nullptr;
//    pBlockTable->getAt(ACDB_MODEL_SPACE, pModelSpace, AcDb::kForWrite);
//    pBlockTable->close();
//
//    for (size_t i = 1; i < rows.size(); ++i)
//    {
//        if (rows[i].size() < 4)
//            continue;
//
//        try
//        {
//            double x = std::stod(rows[i][1]);
//            double y = std::stod(rows[i][2]);
//            double z = std::stod(rows[i][3]);
//
//            AcDbPoint* pPoint = new AcDbPoint(AcGePoint3d(x, y, z));
//            pModelSpace->appendAcDbEntity(pPoint);
//            pPoint->close();
//        }
//        catch (...)
//        {
//            continue;
//        }
//    }
//
//    pModelSpace->close();
//
//    acutPrintf(L"\nNodes imported successfully.");
//}


#include <Windows.h>
#include <commdlg.h>

#include "Domain_WSNode.h"
#include "Import_CSVParser.h"
#include "Global_Data.h"

#include "dbsymtb.h"
#include "dbents.h"
#include "dbapserv.h"
#include "aced.h"

#include <vector>
#include <string>

void cmdImportNodes()
{
    wchar_t filePath[MAX_PATH] = { 0 };

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Select Node CSV File";

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

    // Clear old node map before re-import
    g_NodeMap.clear();

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
            int nodeId = std::stoi(rows[i][0]);

            double x = std::stod(rows[i][1]);
            double y = std::stod(rows[i][2]);
            double z = std::stod(rows[i][3]);

            AcGePoint3d pt(x, y, z);

            // Store in global map
            g_NodeMap[nodeId] = pt;

            // Create AutoCAD point
            AcDbPoint* pPoint = new AcDbPoint(pt);
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