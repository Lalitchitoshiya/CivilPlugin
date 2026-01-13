// =====================================================
// ObjectARX + Late-bound COM (NO TLB)
// STEP 1 → STEP 5 COMPLETE
// =====================================================

#include "rxregsvc.h"
#include "aced.h"
#include "adslib.h"
#include "acutads.h"

#include <Windows.h>
#include <objbase.h>
#include <oleauto.h>

// =====================================================
// COM Utility Helpers
// =====================================================

bool GetDispId(IDispatch* pDisp, const wchar_t* name, DISPID& dispid)
{
    OLECHAR* names[1] = { const_cast<wchar_t*>(name) };
    return SUCCEEDED(pDisp->GetIDsOfNames(
        IID_NULL, names, 1, LOCALE_USER_DEFAULT, &dispid));
}

bool InvokeGet(IDispatch* pDisp, DISPID dispid, VARIANT& result)
{
    DISPPARAMS params = { nullptr, nullptr, 0, 0 };
    VariantInit(&result);

    return SUCCEEDED(pDisp->Invoke(
        dispid, IID_NULL, LOCALE_USER_DEFAULT,
        DISPATCH_PROPERTYGET,
        &params, &result, nullptr, nullptr));
}

bool InvokeMethod_String(
    IDispatch* pDisp,
    DISPID dispid,
    const wchar_t* arg,
    VARIANT& result)
{
    VARIANTARG varg;
    VariantInit(&varg);
    varg.vt = VT_BSTR;
    varg.bstrVal = SysAllocString(arg);

    DISPPARAMS params;
    params.cArgs = 1;
    params.cNamedArgs = 0;
    params.rgvarg = &varg;
    params.rgdispidNamedArgs = nullptr;

    VariantInit(&result);

    HRESULT hr = pDisp->Invoke(
        dispid, IID_NULL, LOCALE_USER_DEFAULT,
        DISPATCH_METHOD,
        &params, &result, nullptr, nullptr);

    SysFreeString(varg.bstrVal);
    return SUCCEEDED(hr);
}

// =====================================================
// STEP 4 — Create Pressure Network (NO TLB)
// =====================================================

IDispatch* g_PressureNetwork = nullptr;

void CreatePressureNetwork_NoTLB()
{
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr))
    {
        acutPrintf(L"\n[ERROR] COM init failed.");
        return;
    }

    CLSID clsid;
    IDispatch* pAeccApp = nullptr;

    hr = CLSIDFromProgID(L"AeccXUiLand.AeccApplication", &clsid);
    if (FAILED(hr) ||
        FAILED(GetActiveObject(clsid, nullptr, (IUnknown**)&pAeccApp)))
    {
        acutPrintf(L"\n[ERROR] Civil 3D COM not available.");
        CoUninitialize();
        return;
    }

    // ActiveDocument
    DISPID dispid;
    VARIANT varDoc;

    GetDispId(pAeccApp, L"ActiveDocument", dispid);
    InvokeGet(pAeccApp, dispid, varDoc);
    IDispatch* pDoc = varDoc.pdispVal;

    // PressureNetworks
    VARIANT varNetworks;
    GetDispId(pDoc, L"PressureNetworks", dispid);
    InvokeGet(pDoc, dispid, varNetworks);
    IDispatch* pNetworks = varNetworks.pdispVal;

    // Add network
    VARIANT varNetwork;
    if (GetDispId(pNetworks, L"Add", dispid) &&
        InvokeMethod_String(pNetworks, dispid,
            L"WS_PRO_POC_NETWORK", varNetwork))
    {
        acutPrintf(L"\n[OK] Pressure Network created.");
        g_PressureNetwork = varNetwork.pdispVal;
        g_PressureNetwork->AddRef();
    }
    else
    {
        acutPrintf(L"\n[ERROR] Failed to create Pressure Network.");
    }

    pNetworks->Release();
    pDoc->Release();
    pAeccApp->Release();
    CoUninitialize();
}

// =====================================================
// STEP 5 — Map WS Pro Node (Semantic, NO GEOMETRY)
// =====================================================

void MapWsProNode_NoTLB()
{
    if (!g_PressureNetwork)
    {
        acutPrintf(L"\n[ERROR] Create Pressure Network first.");
        return;
    }

    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) return;

    DISPID dispid;
    VARIANT varProps;

    if (!GetDispId(g_PressureNetwork,
        L"UserDefinedProperties", dispid) ||
        !InvokeGet(g_PressureNetwork, dispid, varProps))
    {
        acutPrintf(L"\n[ERROR] Cannot access metadata.");
        CoUninitialize();
        return;
    }

    IDispatch* pProps = varProps.pdispVal;

    auto AddProp = [&](const wchar_t* name, VARIANT& val)
        {
            DISPID addId;
            GetDispId(pProps, L"Add", addId);

            VARIANTARG args[2];
            args[1].vt = VT_BSTR;
            args[1].bstrVal = SysAllocString(name);
            args[0] = val;

            DISPPARAMS params = { args, nullptr, 2, 0 };
            pProps->Invoke(
                addId, IID_NULL, LOCALE_USER_DEFAULT,
                DISPATCH_METHOD,
                &params, nullptr, nullptr, nullptr);

            SysFreeString(args[1].bstrVal);
        };

    VARIANT v;

    v.vt = VT_BSTR;
    v.bstrVal = SysAllocString(L"WS_NODE_001");
    AddProp(L"WS_PRO_ASSET_ID", v);
    SysFreeString(v.bstrVal);

    v.vt = VT_BSTR;
    v.bstrVal = SysAllocString(L"JUNCTION");
    AddProp(L"WS_PRO_NODE_TYPE", v);
    SysFreeString(v.bstrVal);

    v.vt = VT_R8; v.dblVal = 100.0; AddProp(L"WS_X", v);
    v.dblVal = 200.0; AddProp(L"WS_Y", v);
    v.dblVal = 0.0;   AddProp(L"WS_Z", v);

    acutPrintf(L"\n[OK] WS Pro node mapped semantically.");

    pProps->Release();
    CoUninitialize();
}

// =====================================================
// ObjectARX Entry Point
// =====================================================

extern "C"
AcRx::AppRetCode acrxEntryPoint(AcRx::AppMsgCode msg, void* pkt)
{
    switch (msg)
    {
    case AcRx::kInitAppMsg:
        acrxUnlockApplication(pkt);
        acrxRegisterAppMDIAware(pkt);

        acedRegCmds->addCommand(
            L"WSPOC_GROUP",
            L"CREATE_PRESSURE_NETWORK_NOTLB",
            L"CREATE_PRESSURE_NETWORK_NOTLB",
            ACRX_CMD_MODAL,
            CreatePressureNetwork_NoTLB);

        acedRegCmds->addCommand(
            L"WSPOC_GROUP",
            L"MAP_WS_NODE_NOTLB",
            L"MAP_WS_NODE_NOTLB",
            ACRX_CMD_MODAL,
            MapWsProNode_NoTLB);
        break;

    case AcRx::kUnloadAppMsg:
        if (g_PressureNetwork)
            g_PressureNetwork->Release();
        acedRegCmds->removeGroup(L"WSPOC_GROUP");
        break;
    }
    return AcRx::kRetOK;
}