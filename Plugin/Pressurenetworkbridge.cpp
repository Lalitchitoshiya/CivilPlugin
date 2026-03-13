


// ============================================================
//  PressureNetworkBridge.cpp
//
//  Column mapping for your CSVs:
//  Nodes  (cssv4.CSV): ASSET_ID, X, Y, Z_ELEV, TYPE
//  Pipes  (csv3.CSV):  ASSET_ID, DIAM_MM, MATERIAL, US_NODE, DS_NODE
//
//  Fix: acedCommandS code=-5001 (RTERROR) when calling SCRIPT.
//  Root cause: cannot call SCRIPT from inside a command callback.
//  Solution: use acedSendStringToExecute() to post commands to
//  Civil 3D's command queue — runs AFTER our command returns.
// ============================================================

#ifndef DWORD
typedef unsigned long DWORD;
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#ifndef CP_ACP
#define CP_ACP 0
#endif

extern "C"
{
    __declspec(dllimport) DWORD __stdcall
        GetTempPathW(DWORD nBufferLength, wchar_t* lpBuffer);

    __declspec(dllimport) int __stdcall
        WideCharToMultiByte(unsigned int   CodePage,
            DWORD          dwFlags,
            const wchar_t* lpWideCharStr,
            int            cchWideChar,
            char* lpMultiByteStr,
            int            cbMultiByte,
            const char* lpDefaultChar,
            int* lpUsedDefaultChar);
}

#include "PressureNetworkBridge.h"
#include <aced.h>
#include <acedads.h>
#include <acutads.h>
#include <acedCmdNF.h>
#include <acdocman.h>
// No extra declaration needed — acedInvoke is in acedads.h (already included)
// acedInvoke: calls a LISP function by name with a resbuf argument list

#include <map>
#include <algorithm>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <fstream>

// Late-bound Civil 3D COM (no TLB) — used to create/get pressure network.
#include <Windows.h>
#include <objbase.h>
#include <oleauto.h>

// ─────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────

static std::wstring wsTrim(const std::wstring& s)
{
    size_t b = s.find_first_not_of(L" \t\r\n");
    if (b == std::wstring::npos) return L"";
    size_t e = s.find_last_not_of(L" \t\r\n");
    return s.substr(b, e - b + 1);
}

static std::vector<std::wstring> wsSplitCSV(const std::wstring& line)
{
    std::vector<std::wstring> cols;
    std::wstring cur;
    bool inQuote = false;
    for (wchar_t c : line)
    {
        if (c == L'"') { inQuote = !inQuote; }
        else if (c == L',' && !inQuote) { cols.push_back(wsTrim(cur)); cur.clear(); }
        else { cur += c; }
    }
    cols.push_back(wsTrim(cur));
    return cols;
}

static void wsToUpper(std::wstring& s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::towupper);
}

static int resolveCol(const std::map<std::wstring, int>& col,
    const wchar_t* aliases[], int count)
{
    for (int i = 0; i < count; ++i)
    {
        auto it = col.find(aliases[i]);
        if (it != col.end()) return it->second;
    }
    return -1;
}

// ─────────────────────────────────────────────────────────────
//  Civil 3D COM helpers (late-bound, best-effort)
// ─────────────────────────────────────────────────────────────

static bool ComGetDispId(IDispatch* disp, const wchar_t* name, DISPID& outId)
{
    if (!disp) return false;
    OLECHAR* names[1] = { const_cast<wchar_t*>(name) };
    return SUCCEEDED(disp->GetIDsOfNames(IID_NULL, names, 1, LOCALE_USER_DEFAULT, &outId));
}

static bool ComInvokeGet(IDispatch* disp, DISPID id, VARIANT& result)
{
    if (!disp) return false;
    DISPPARAMS params = { nullptr, nullptr, 0, 0 };
    VariantInit(&result);
    return SUCCEEDED(disp->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
        &params, &result, nullptr, nullptr));
}

static bool ComInvokeMethod(IDispatch* disp, DISPID id, VARIANTARG* args, int argc, VARIANT* outResult)
{
    if (!disp) return false;
    DISPPARAMS params;
    params.rgvarg = args;
    params.rgdispidNamedArgs = nullptr;
    params.cArgs = argc;
    params.cNamedArgs = 0;
    VARIANT local;
    VARIANT* target = outResult ? outResult : &local;
    VariantInit(target);
    HRESULT hr = disp->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD,
        &params, target, nullptr, nullptr);
    if (!outResult) VariantClear(&local);
    return SUCCEEDED(hr);
}

static bool ComGetPropDisp(IDispatch* disp, const wchar_t* propName, IDispatch** out)
{
    if (!out) return false;
    *out = nullptr;
    DISPID id;
    if (!ComGetDispId(disp, propName, id)) return false;
    VARIANT v;
    if (!ComInvokeGet(disp, id, v)) return false;
    if (v.vt == VT_DISPATCH && v.pdispVal)
    {
        *out = v.pdispVal;
        (*out)->AddRef();
        VariantClear(&v);
        return true;
    }
    VariantClear(&v);
    return false;
}

static bool ComCall_ItemByName(IDispatch* collection, const std::wstring& name, IDispatch** outItem)
{
    if (!outItem) return false;
    *outItem = nullptr;
    DISPID itemId;
    if (!ComGetDispId(collection, L"Item", itemId)) return false;

    VARIANTARG arg;
    VariantInit(&arg);
    arg.vt = VT_BSTR;
    arg.bstrVal = SysAllocString(name.c_str());

    VARIANT result;
    bool ok = ComInvokeMethod(collection, itemId, &arg, 1, &result);
    SysFreeString(arg.bstrVal);
    if (!ok) return false;

    if (result.vt == VT_DISPATCH && result.pdispVal)
    {
        *outItem = result.pdispVal;
        (*outItem)->AddRef();
        VariantClear(&result);
        return true;
    }
    VariantClear(&result);
    return false;
}

static bool ComCall_AddWithName(IDispatch* collection, const std::wstring& name, IDispatch** outItem)
{
    if (!outItem) return false;
    *outItem = nullptr;
    DISPID addId;
    if (!ComGetDispId(collection, L"Add", addId)) return false;

    VARIANTARG arg;
    VariantInit(&arg);
    arg.vt = VT_BSTR;
    arg.bstrVal = SysAllocString(name.c_str());

    VARIANT result;
    bool ok = ComInvokeMethod(collection, addId, &arg, 1, &result);
    SysFreeString(arg.bstrVal);
    if (!ok) return false;

    if (result.vt == VT_DISPATCH && result.pdispVal)
    {
        *outItem = result.pdispVal;
        (*outItem)->AddRef();
        VariantClear(&result);
        return true;
    }
    VariantClear(&result);
    return false;
}

static bool ComEnsurePressureNetwork(const std::wstring& netName, IDispatch** outNet, std::wstring& outErr)
{
    if (!outNet) return false;
    *outNet = nullptr;

    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool didInit = SUCCEEDED(hrInit);

    auto getActiveDispatchByProgId = [&](const wchar_t* progId, IDispatch** outDisp) -> bool
        {
            if (!outDisp) return false;
            *outDisp = nullptr;
            CLSID clsid;
            HRESULT hr = CLSIDFromProgID(progId, &clsid);
            if (FAILED(hr)) return false;
            IDispatch* disp = nullptr;
            hr = GetActiveObject(clsid, nullptr, (IUnknown**)&disp);
            if (FAILED(hr) || !disp) return false;
            *outDisp = disp;
            return true;
        };

    auto callGetInterfaceObject = [&](IDispatch* acadApp, const wchar_t* ifaceProgId, IDispatch** outDisp) -> bool
        {
            if (!outDisp) return false;
            *outDisp = nullptr;
            if (!acadApp) return false;

            DISPID mid;
            if (!ComGetDispId(acadApp, L"GetInterfaceObject", mid)) return false;

            VARIANTARG arg;
            VariantInit(&arg);
            arg.vt = VT_BSTR;
            arg.bstrVal = SysAllocString(ifaceProgId);

            VARIANT result;
            bool ok = ComInvokeMethod(acadApp, mid, &arg, 1, &result);
            SysFreeString(arg.bstrVal);
            if (!ok) { VariantClear(&result); return false; }

            if (result.vt == VT_DISPATCH && result.pdispVal)
            {
                *outDisp = result.pdispVal;
                (*outDisp)->AddRef();
                VariantClear(&result);
                return true;
            }
            VariantClear(&result);
            return false;
        };

    // 1) Try direct Civil 3D application progids
    IDispatch* aeccApp = nullptr;
    const wchar_t* civilProgIds[] = {
        L"AeccXUiLand.AeccApplication",
        L"AeccXUiLand.AeccApplication.13.8",
        L"AeccXUiLand.AeccApplication.13.7",
        L"AeccXUiLand.AeccApplication.13.6",
    };
    for (const auto* pid : civilProgIds)
    {
        if (getActiveDispatchByProgId(pid, &aeccApp)) break;
    }

    // 2) If that fails, go through the running AutoCAD.Application and ask for Civil 3D interface.
    if (!aeccApp)
    {
        IDispatch* acadApp = nullptr;
        const wchar_t* acadProgIds[] = {
            L"AutoCAD.Application",
            L"AutoCAD.Application.25",
            L"AutoCAD.Application.25.1",
            L"AutoCAD.Application.25.0",
        };
        for (const auto* pid : acadProgIds)
        {
            if (getActiveDispatchByProgId(pid, &acadApp)) break;
        }

        if (acadApp)
        {
            // This is the typical robust path inside Civil 3D.
            if (!callGetInterfaceObject(acadApp, L"AeccXUiLand.AeccApplication", &aeccApp))
                callGetInterfaceObject(acadApp, L"AeccXUiRoadway.AeccApplication", &aeccApp);
            acadApp->Release();
        }
    }

    if (!aeccApp)
    {
        outErr = L"COM: Civil 3D application object not available (direct or via AutoCAD.GetInterfaceObject).";
        if (didInit) CoUninitialize();
        return false;
    }

    IDispatch* doc = nullptr;
    {
        DISPID id;
        VARIANT v;
        if (!ComGetDispId(aeccApp, L"ActiveDocument", id) || !ComInvokeGet(aeccApp, id, v) ||
            v.vt != VT_DISPATCH || !v.pdispVal)
        {
            outErr = L"COM: Cannot get ActiveDocument.";
            VariantClear(&v);
            aeccApp->Release();
            if (didInit) CoUninitialize();
            return false;
        }
        doc = v.pdispVal;
        doc->AddRef();
        VariantClear(&v);
    }

    IDispatch* nets = nullptr;
    if (!ComGetPropDisp(doc, L"PressureNetworks", &nets))
    {
        outErr = L"COM: ActiveDocument.PressureNetworks not available.";
        doc->Release();
        aeccApp->Release();
        if (didInit) CoUninitialize();
        return false;
    }

    IDispatch* net = nullptr;
    if (!ComCall_ItemByName(nets, netName, &net))
    {
        if (!ComCall_AddWithName(nets, netName, &net))
        {
            outErr = L"COM: Failed to get/create pressure network (Item/Add).";
            nets->Release();
            doc->Release();
            aeccApp->Release();
            if (didInit) CoUninitialize();
            return false;
        }
    }

    *outNet = net; // already AddRef'd
    nets->Release();
    doc->Release();
    aeccApp->Release();
    if (didInit) CoUninitialize();
    return true;
}

static SAFEARRAY* ComMakePointArray(double x, double y, double z)
{
    SAFEARRAYBOUND b;
    b.lLbound = 0;
    b.cElements = 3;
    SAFEARRAY* sa = SafeArrayCreate(VT_R8, 1, &b);
    if (!sa) return nullptr;
    double v[3] = { x, y, z };
    for (LONG i = 0; i < 3; ++i)
        SafeArrayPutElement(sa, &i, &v[i]);
    return sa;
}

static bool ComTryAddPipeBestEffort(IDispatch* pressureNetwork,
    const PNBridge::PipeNode& s,
    const PNBridge::PipeNode& e,
    std::wstring& outErr)
{
    // Try 1: pressureNetwork.Pipes.Add(startPointArray, endPointArray)
    // Try 2: pressureNetwork.AddPipe(startPointArray, endPointArray)
    // (COM model varies by version/config; if unsupported we return false)
    if (!pressureNetwork) return false;

    auto tryCall = [&](IDispatch* target, const wchar_t* method) -> bool
        {
            DISPID mid;
            if (!ComGetDispId(target, method, mid)) return false;

            SAFEARRAY* saS = ComMakePointArray(s.x, s.y, s.z);
            SAFEARRAY* saE = ComMakePointArray(e.x, e.y, e.z);
            if (!saS || !saE)
            {
                if (saS) SafeArrayDestroy(saS);
                if (saE) SafeArrayDestroy(saE);
                return false;
            }

            VARIANTARG args[2];
            VariantInit(&args[0]);
            VariantInit(&args[1]);

            // reverse order
            args[0].vt = VT_ARRAY | VT_R8;
            args[0].parray = saE;
            args[1].vt = VT_ARRAY | VT_R8;
            args[1].parray = saS;

            VARIANT res;
            bool ok = ComInvokeMethod(target, mid, args, 2, &res);
            VariantClear(&res);

            SafeArrayDestroy(saS);
            SafeArrayDestroy(saE);
            return ok;
        };

    // Pipes collection path
    IDispatch* pipes = nullptr;
    if (ComGetPropDisp(pressureNetwork, L"Pipes", &pipes))
    {
        bool ok = tryCall(pipes, L"Add");
        pipes->Release();
        if (ok) return true;
    }

    // Direct method path
    if (tryCall(pressureNetwork, L"AddPipe")) return true;

    outErr = L"COM: Pipe creation method not found (Pipes.Add / AddPipe).";
    return false;
}

// ─────────────────────────────────────────────────────────────
namespace PNBridge
{

    // ─────────────────────────────────────────────────────────────
    //  CSV IMPORT
    // ─────────────────────────────────────────────────────────────

    BridgeError PressureNetworkBridge::loadFromCSV(
        const std::wstring& nodesCsvPath,
        const std::wstring& segsCsvPath,
        NetworkDef& outNet)
    {
        // ── Nodes ─────────────────────────────────────────────────
        {
            std::wifstream f(nodesCsvPath);
            if (!f.is_open())
                return { BridgeResult::ERR_CSV_PARSE,
                         L"Cannot open nodes CSV: " + nodesCsvPath };

            std::wstring line;
            bool headerDone = false;
            std::map<std::wstring, int> col;
            int ci_id = -1, ci_x = -1, ci_y = -1, ci_z = -1, ci_type = -1;

            while (std::getline(f, line))
            {
                if (wsTrim(line).empty()) continue;
                std::vector<std::wstring> cells = wsSplitCSV(line);

                if (!headerDone)
                {
                    headerDone = true;
                    for (int i = 0; i < static_cast<int>(cells.size()); ++i)
                    {
                        std::wstring h = cells[i];
                        if (!h.empty() && (unsigned short)h[0] == 0xFEFF)
                            h = h.substr(1);
                        wsToUpper(h);
                        col[h] = i;
                    }

                    {
                        const wchar_t* a[] = { L"ASSET_ID",L"ID",L"NODE_ID",L"NODEID",L"NAME",L"LABEL",L"NO",L"NUMBER",L"NODE" };
                        ci_id = resolveCol(col, a, 9);
                        if (ci_id < 0) return{ BridgeResult::ERR_CSV_PARSE,L"Nodes CSV: no ID column. Tried: ASSET_ID, ID, NODE_ID" };
                    }

                    {
                        const wchar_t* a[] = { L"X",L"EASTING",L"X_COORD",L"XCOORD",L"EAST" };
                        ci_x = resolveCol(col, a, 5);
                        if (ci_x < 0) return{ BridgeResult::ERR_CSV_PARSE,L"Nodes CSV: no X column. Tried: X, EASTING, X_COORD" };
                    }

                    {
                        const wchar_t* a[] = { L"Y",L"NORTHING",L"Y_COORD",L"YCOORD",L"NORTH" };
                        ci_y = resolveCol(col, a, 5);
                        if (ci_y < 0) return{ BridgeResult::ERR_CSV_PARSE,L"Nodes CSV: no Y column. Tried: Y, NORTHING, Y_COORD" };
                    }

                    {
                        const wchar_t* a[] = { L"Z_ELEV",L"Z",L"ELEVATION",L"ELEV",L"EL",L"HEIGHT",L"ALT",L"INVERT",L"Z_COORD",L"INVERT_ELEV" };
                        ci_z = resolveCol(col, a, 10);
                        if (ci_z < 0) return{ BridgeResult::ERR_CSV_PARSE,L"Nodes CSV: no Z column. Tried: Z_ELEV, Z, ELEVATION, ELEV" };
                    }

                    {
                        const wchar_t* a[] = { L"TYPE",L"NODE_TYPE",L"NODETYPE",L"KIND",L"CATEGORY",L"CLASS" };
                        ci_type = resolveCol(col, a, 6);
                    }

                    acutPrintf(L"  Nodes columns: ID=%d X=%d Y=%d Z=%d TYPE=%d\n",
                        ci_id, ci_x, ci_y, ci_z, ci_type);
                    continue;
                }

                if (static_cast<int>(cells.size()) <= ci_id) continue;

                PipeNode n;
                n.id = cells[ci_id];
                if (n.id.empty()) continue;

                try { n.x = std::stod(cells[ci_x]); }
                catch (...) { continue; }
                try { n.y = std::stod(cells[ci_y]); }
                catch (...) { continue; }
                try { n.z = std::stod(cells[ci_z]); }
                catch (...) { n.z = 0.0; }

                n.type = L"JUNCTION";
                if (ci_type >= 0 && ci_type < static_cast<int>(cells.size())
                    && !cells[ci_type].empty())
                {
                    n.type = cells[ci_type];
                    wsToUpper(n.type);
                    if (n.type == L"H" || n.type == L"FH" || n.type == L"FIRE_HYDRANT") n.type = L"HYDRANT";
                    else if (n.type == L"V" || n.type == L"GV" || n.type == L"GATE_VALVE") n.type = L"VALVE";
                    else if (n.type == L"J" || n.type == L"JN" || n.type == L"JCT" || n.type == L"TEE" || n.type == L"CROSS") n.type = L"JUNCTION";
                    else if (n.type == L"S" || n.type == L"SRC" || n.type == L"RESERVOIR" || n.type == L"TANK" || n.type == L"INLET") n.type = L"SOURCE";
                }

                outNet.nodes.push_back(std::move(n));
            }
        }

        if (outNet.nodes.empty())
            return { BridgeResult::ERR_INVALID_DATA, L"No nodes loaded from CSV" };

        acutPrintf(L"  Loaded %d nodes.\n", static_cast<int>(outNet.nodes.size()));

        // ── Segments ──────────────────────────────────────────────
        {
            std::wifstream f(segsCsvPath);
            if (!f.is_open())
                return { BridgeResult::ERR_CSV_PARSE,
                         L"Cannot open segments CSV: " + segsCsvPath };

            std::wstring line;
            bool headerDone = false;
            std::map<std::wstring, int> col;
            int ci_id = -1, ci_diam = -1, ci_mat = -1, ci_start = -1, ci_end = -1;

            while (std::getline(f, line))
            {
                if (wsTrim(line).empty()) continue;
                std::vector<std::wstring> cells = wsSplitCSV(line);

                if (!headerDone)
                {
                    headerDone = true;
                    for (int i = 0; i < static_cast<int>(cells.size()); ++i)
                    {
                        std::wstring h = cells[i];
                        if (!h.empty() && (unsigned short)h[0] == 0xFEFF) h = h.substr(1);
                        wsToUpper(h);
                        col[h] = i;
                    }

                    {
                        const wchar_t* a[] = { L"ASSET_ID",L"ID",L"PIPE_ID",L"PIPEID",L"NAME",L"LABEL",L"NO",L"PIPE" };
                        ci_id = resolveCol(col, a, 8);
                        if (ci_id < 0) return{ BridgeResult::ERR_CSV_PARSE,L"Segments CSV: no ID column." };
                    }

                    {
                        const wchar_t* a[] = { L"DIAM_MM",L"DIAMETER_MM",L"DIAMETER",L"DIAM",L"DIA",L"DIA_MM",L"SIZE",L"SIZE_MM" };
                        ci_diam = resolveCol(col, a, 8);
                        if (ci_diam < 0) return{ BridgeResult::ERR_CSV_PARSE,L"Segments CSV: no DIAMETER column. Tried: DIAM_MM, DIAMETER, DIAM" };
                    }

                    {
                        const wchar_t* a[] = { L"MATERIAL",L"MAT",L"PIPE_MATERIAL",L"PIPE_TYPE",L"MATL" };
                        ci_mat = resolveCol(col, a, 5);
                    }

                    {
                        const wchar_t* a[] = { L"US_NODE",L"START_NODE",L"START",L"FROM_NODE",L"FROM",L"UPSTREAM",L"NODE1",L"JUNC1",L"INLET_NODE" };
                        ci_start = resolveCol(col, a, 9);
                        if (ci_start < 0) return{ BridgeResult::ERR_CSV_PARSE,L"Segments CSV: no START node column. Tried: US_NODE, START_NODE, FROM" };
                    }

                    {
                        const wchar_t* a[] = { L"DS_NODE",L"END_NODE",L"END",L"TO_NODE",L"TO",L"DOWNSTREAM",L"NODE2",L"JUNC2",L"OUTLET_NODE" };
                        ci_end = resolveCol(col, a, 9);
                        if (ci_end < 0) return{ BridgeResult::ERR_CSV_PARSE,L"Segments CSV: no END node column. Tried: DS_NODE, END_NODE, TO" };
                    }

                    acutPrintf(L"  Pipes columns: ID=%d DIAM=%d MAT=%d START=%d END=%d\n",
                        ci_id, ci_diam, ci_mat, ci_start, ci_end);
                    continue;
                }

                if (static_cast<int>(cells.size()) <= ci_id) continue;

                PipeSeg s;
                s.id = cells[ci_id];
                s.startNode = cells[ci_start];
                s.endNode = cells[ci_end];
                if (s.id.empty() || s.startNode.empty() || s.endNode.empty()) continue;

                try { s.diameter = std::stod(cells[ci_diam]); }
                catch (...) { s.diameter = 100.0; }

                s.material = (ci_mat >= 0 && ci_mat < static_cast<int>(cells.size()) && !cells[ci_mat].empty())
                    ? cells[ci_mat] : L"PVC";

                // Auto-create missing source nodes (1, 2, 3, RIVER)
                auto ensureNode = [&](const std::wstring& nid)
                    {
                        if (!findNode(outNet, nid))
                        {
                            PipeNode src;
                            src.id = nid; src.x = 0.0; src.y = 0.0; src.z = 0.0; src.type = L"SOURCE";
                            outNet.nodes.push_back(src);
                            acutPrintf(L"  [INFO] Auto-created SOURCE node '%s' at (0,0,0)\n", nid.c_str());
                        }
                    };
                ensureNode(s.startNode);
                ensureNode(s.endNode);

                outNet.segments.push_back(std::move(s));
            }
        }

        if (outNet.segments.empty())
            return { BridgeResult::ERR_INVALID_DATA, L"No segments loaded from CSV" };

        acutPrintf(L"  Loaded %d segments.\n", static_cast<int>(outNet.segments.size()));
        return {};
    }

    // ─────────────────────────────────────────────────────────────
    //  SCRIPT WRITERS
    // ─────────────────────────────────────────────────────────────

    void PressureNetworkBridge::writeHeader(std::wostringstream& f, const NetworkDef& net)
    {
        // FILEDIA removed — cannot reliably set system variables from within
        // a script file in Civil 3D 2024+. FILEDIA is handled by sendStr.
        // CMDECHO 0 kept to suppress command echo during script execution.
        f << L"; Auto-generated by PressureNetworkBridge\n"
            << L"; Network: " << net.name << L"\n\n"
            << L"CMDECHO 0\n\n";
    }

    void PressureNetworkBridge::writeSetCurrentLayer(std::wostringstream& f,
        const std::wstring& layer)
    {
        f << L"-LAYER\nM\n" << layer << L"\n\n";
    }

    void PressureNetworkBridge::writeCreateNetwork(std::wostringstream& f,
        const NetworkDef& net)
    {
        // CREATEPRESSURENETWORK and -CREATEPRESSURENETWORK both require
        // GUI interaction in Civil 3D 2024/2025/2026 and cannot be driven
        // from a script file.
        //
        // SOLUTION: The user creates the network manually once in Civil 3D
        // (Home → Pressure Network → Create Pressure Network).
        // This function now only sets the active network via SETCURRENTPRESSURENETWORK
        // so all subsequent ADDPRESSURENETWORKPIPE/FITTING commands target it.
        f << L"; Set active pressure network — must already exist in drawing\n"
            << L"SETCURRENTPRESSURENETWORK\n"
            << escapeForScript(net.name) << L"\n\n";
    }

    void PressureNetworkBridge::writeAddJunction(std::wostringstream& f, const PipeNode& n)
    {
        f << L"ADDPRESSURENETWORKFITTING\n"
            << fmt(n.x) << L"," << fmt(n.y) << L"," << fmt(n.z) << L"\n\n";
    }

    void PressureNetworkBridge::writeAddHydrant(std::wostringstream& f, const PipeNode& n)
    {
        f << L"ADDPRESSURENETWORKAPPURTENANCE\n"
            << fmt(n.x) << L"," << fmt(n.y) << L"," << fmt(n.z) << L"\n\n";
    }

    void PressureNetworkBridge::writeAddValve(std::wostringstream& f, const PipeNode& n)
    {
        f << L"ADDPRESSURENETWORKAPPURTENANCE\n"
            << fmt(n.x) << L"," << fmt(n.y) << L"," << fmt(n.z) << L"\n\n";
    }

    void PressureNetworkBridge::writeAddPipe(std::wostringstream& f,
        const PipeSeg& seg,
        const NetworkDef& net)
    {
        const PipeNode* s = findNode(net, seg.startNode);
        const PipeNode* e = findNode(net, seg.endNode);
        assert(s && e);

        f << L"; Pipe " << seg.id << L" " << seg.startNode << L"->" << seg.endNode << L"\n"
            << L"ADDPRESSURENETWORKPIPE\n"
            << fmt(s->x) << L"," << fmt(s->y) << L"," << fmt(s->z) << L"\n"
            << fmt(e->x) << L"," << fmt(e->y) << L"," << fmt(e->z) << L"\n\n";
    }

    // ─────────────────────────────────────────────────────────────
    //  MAIN: build .scr + execute
    //
    //  FIX for code=-5001:
    //  Cannot call SCRIPT from inside an active ARX command callback.
    //  Use acedSendStringToExecute() instead — posts "SCRIPT <path>"
    //  to Civil 3D's command queue, runs after our callback returns.
    // ─────────────────────────────────────────────────────────────

    BridgeError PressureNetworkBridge::createPressureNetwork(
        const NetworkDef& net,
        const std::wstring& scriptDir)
    {
        // Step 0: Ensure the Pressure Network exists (no manual creation).
        // We use Civil 3D COM for this because Civil 3D 2026 commands are hard to automate.
        {
            IDispatch* comNet = nullptr;
            std::wstring comErr;
            if (!ComEnsurePressureNetwork(net.name, &comNet, comErr))
            {
                acutPrintf(L"  [WARN] %s\n", comErr.c_str());
            }
            else
            {
                comNet->Release();
            }
        }

        // Build script path — sanitise network name for use as filename
        // Keep only alphanumeric + underscore, replace everything else with _
        std::wstring safeName;
        for (wchar_t c : net.name)
            safeName += (iswalnum(c) || c == L'_') ? c : L'_';
        if (safeName.empty()) safeName = L"Network";

        std::wstring dir = scriptDir.empty() ? tempDir() : scriptDir;
        if (!dir.empty() && dir.back() != L'\\') dir += L'\\';
        m_lastScriptPath = dir + L"pn_" + safeName + L".scr";

        // Write script using std::wostringstream → narrow UTF-8 → WriteFile
        // Avoids std::wofstream locale/encoding failures on some Windows configs
        {
            std::wostringstream ws;
            writeHeader(ws, net);
            writeSetCurrentLayer(ws, net.layerName);
            writeCreateNetwork(ws, net);

            for (const auto& node : net.nodes)
            {
                if (node.type == L"HYDRANT") writeAddHydrant(ws, node);
                else if (node.type == L"VALVE")   writeAddValve(ws, node);
                else                              writeAddJunction(ws, node);
            }
            for (const auto& seg : net.segments)
                writeAddPipe(ws, seg, net);

            ws << L"\nCMDECHO 1\n";

            // Convert wide string to narrow (ANSI) for .scr file
            std::wstring wContent = ws.str();
            int needed = WideCharToMultiByte(CP_ACP, 0,
                wContent.c_str(), -1,
                NULL, 0, NULL, NULL);
            std::string content(static_cast<size_t>(needed), '\0');
            WideCharToMultiByte(CP_ACP, 0, wContent.c_str(), -1,
                &content[0], needed, NULL, NULL);
            if (!content.empty() && content.back() == '\0')
                content.pop_back();

            // Write via standard ofstream (narrow) — avoids wofstream locale issues
            std::ofstream f(m_lastScriptPath, std::ios::out | std::ios::trunc | std::ios::binary);
            if (!f.is_open())
                return { BridgeResult::ERR_SCRIPT_WRITE,
                         L"Cannot open script for writing: " + m_lastScriptPath };
            f.write(content.c_str(), static_cast<std::streamsize>(content.size()));
            f.flush();
            if (!f.good())
                return { BridgeResult::ERR_SCRIPT_WRITE, L"Error writing script file" };
        }

        acutPrintf(L"  Script written: %s\n", m_lastScriptPath.c_str());

        // Step 1: Try creating pipes via COM (best-effort).
        // If pipe creation is not exposed in this COM model, we'll fall back to script/command.
        bool createdViaCom = false;
        {
            IDispatch* comNet = nullptr;
            std::wstring comErr;
            if (ComEnsurePressureNetwork(net.name, &comNet, comErr))
            {
                bool any = false;
                bool all = true;
                for (const auto& seg : net.segments)
                {
                    const PipeNode* s = findNode(net, seg.startNode);
                    const PipeNode* e = findNode(net, seg.endNode);
                    if (!s || !e) continue;
                    any = true;
                    std::wstring perr;
                    if (!ComTryAddPipeBestEffort(comNet, *s, *e, perr))
                    {
                        all = false;
                        break;
                    }
                }
                createdViaCom = any && all;
                comNet->Release();
            }
        }

        if (createdViaCom)
        {
            acutPrintf(L"  Created pressure pipes via COM.\n");
            return {};
        }

        // Step 2: Queue SCRIPT execution after our command returns (fixes -5001).
        // Use forward slashes to avoid escaping issues.
        std::wstring fwdPath = m_lastScriptPath;
        for (auto& c : fwdPath) if (c == L'\\') c = L'/';

        std::wstring cmd = L"(command \"_.FILEDIA\" \"0\" \"_.SCRIPT\" \"" + fwdPath + L"\" \"_.FILEDIA\" \"1\")\n";
        // Queue for execution after our command fully finishes.
        // bActivate=false + bWrapUpInactiveDoc=true prevents our queued input
        // from being consumed by any still-active prompt in this command.
        acDocManager->sendStringToExecute(acDocManager->curDocument(), cmd.c_str(), false, true, false);
        acutPrintf(L"  Queued script execution.\n");

        return {};
    }

    // ─────────────────────────────────────────────────────────────
    //  CONVENIENCE
    // ─────────────────────────────────────────────────────────────

    BridgeError PressureNetworkBridge::run(
        const std::wstring& nodesCsvPath,
        const std::wstring& segsCsvPath,
        const std::wstring& networkName,
        const std::wstring& partsListName,
        const std::wstring& layerName)
    {
        NetworkDef net;
        net.name = networkName;
        net.description = L"Imported from CSV by PressureNetworkBridge";
        net.partsListName = partsListName;
        net.layerName = layerName;

        BridgeError err = loadFromCSV(nodesCsvPath, segsCsvPath, net);
        if (!err.ok()) return err;
        return createPressureNetwork(net);
    }

    // ─────────────────────────────────────────────────────────────
    //  PRIVATE HELPERS
    // ─────────────────────────────────────────────────────────────

    const PipeNode* PressureNetworkBridge::findNode(
        const NetworkDef& net, const std::wstring& id) const
    {
        for (const auto& n : net.nodes)
            if (n.id == id) return &n;
        return NULL;
    }

    std::wstring PressureNetworkBridge::tempDir() const
    {
        wchar_t buf[MAX_PATH];
        buf[0] = L'\0';
        DWORD len = GetTempPathW(MAX_PATH, buf);
        if (len > 0 && len < MAX_PATH)
            return std::wstring(buf);
        return L"C:\\Temp\\";
    }

    std::wstring PressureNetworkBridge::escapeForScript(const std::wstring& s) const
    {
        if (s.find(L' ') != std::wstring::npos)
            return L"\"" + s + L"\"";
        return s;
    }

    std::wstring PressureNetworkBridge::fmt(double v, int decimals) const
    {
        std::wostringstream ss;
        ss << std::fixed << std::setprecision(decimals) << v;
        return ss.str();
    }

} // namespace PNBridge