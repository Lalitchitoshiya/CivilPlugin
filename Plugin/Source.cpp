// =======================
// ObjectARX + Late-bound COM
// No TLB, No Aecc headers
// =======================

//#include "rxregsvc.h"
//#include "aced.h"
//#include "adslib.h"
//#include "acutads.h"
//
//#include <Windows.h>
//#include <objbase.h>
//#include <oleauto.h>

// ------------------------------------------------------------
// Command: TEST_CIVIL3D_COM_NOTLB
// ------------------------------------------------------------



//void TestCivil3D_COM_NoTLB()
//{
//    HRESULT hr = CoInitialize(nullptr);
//    if (FAILED(hr))
//    {
//        acutPrintf(L"\n[ERROR] COM initialization failed.");
//        return;
//    }
//
//    acutPrintf(L"\n[OK] COM initialized.");
//
//    CLSID clsid;
//    IDispatch* pAeccApp = nullptr;
//
//    // Get Civil 3D COM ProgID
//    hr = CLSIDFromProgID(L"AeccXUiLand.AeccApplication", &clsid);
//    if (FAILED(hr))
//    {
//        acutPrintf(L"\n[ERROR] Civil 3D ProgID not found.");
//        CoUninitialize();
//        return;
//    }

//    // Connect to running Civil 3D
//    hr = GetActiveObject(clsid, nullptr, (IUnknown**)&pAeccApp);
//    if (FAILED(hr) || !pAeccApp)
//    {
//        acutPrintf(L"\n[ERROR] Civil 3D is not running.");
//        CoUninitialize();
//        return;
//    }
//
//    acutPrintf(L"\n[OK] Connected to Civil 3D COM (no TLB).");
//
//    // Cleanup
//    pAeccApp->Release();
//    CoUninitialize();
//
//    acutPrintf(L"\n[OK] COM released cleanly.");
//}
//
//// ------------------------------------------------------------
//// ObjectARX Entry Point (MANDATORY)
//// ------------------------------------------------------------
//extern "C"
//AcRx::AppRetCode acrxEntryPoint(AcRx::AppMsgCode msg, void* pkt)
//{
//    switch (msg)
//    {
//    case AcRx::kInitAppMsg:
//        acrxUnlockApplication(pkt);
//        acrxRegisterAppMDIAware(pkt);
//
//        acedRegCmds->addCommand(
//            L"WSPOC_GROUP",
//            L"TEST_CIVIL3D_COM_NOTLB",
//            L"TEST_CIVIL3D_COM_NOTLB",
//            ACRX_CMD_MODAL,
//            TestCivil3D_COM_NoTLB
//        );
//        break;
//
//    case AcRx::kUnloadAppMsg:
//        acedRegCmds->removeGroup(L"WSPOC_GROUP");
//        break;
//    }
//
//    return AcRx::kRetOK;
//}


// =====================================================
// ObjectARX + Late-bound COM
// STEP 4: Create Pressure Network (NO TLB)
// =====================================================




#include "rxregsvc.h"
#include "aced.h"
#include "adslib.h"
#include "acutads.h"

#include <Windows.h>
#include <objbase.h>
#include <oleauto.h>

// -----------------------------------------------------
// Helper: Get DISPID by name
// -----------------------------------------------------
bool GetDispId(IDispatch* pDisp, const wchar_t* name, DISPID& dispid)
{
    OLECHAR* names[1] = { const_cast<wchar_t*>(name) };
    return SUCCEEDED(pDisp->GetIDsOfNames(
        IID_NULL, names, 1, LOCALE_USER_DEFAULT, &dispid));
}

// -----------------------------------------------------
// Helper: Invoke property get
// -----------------------------------------------------
bool InvokeGet(IDispatch* pDisp, DISPID dispid, VARIANT& result)
{
    DISPPARAMS params = { nullptr, nullptr, 0, 0 };
    VariantInit(&result);

    return SUCCEEDED(pDisp->Invoke(
        dispid, IID_NULL, LOCALE_USER_DEFAULT,
        DISPATCH_PROPERTYGET,
        &params, &result, nullptr, nullptr));
}

// -----------------------------------------------------
// Helper: Invoke method with 1 string argument
// -----------------------------------------------------
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

// -----------------------------------------------------
// Command: CREATE_PRESSURE_NETWORK_NOTLB
// -----------------------------------------------------
void CreatePressureNetwork_NoTLB()
{
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr))
    {
        acutPrintf(L"\n[ERROR] COM initialization failed.");
        return;
    }

    CLSID clsid;
    IDispatch* pAeccApp = nullptr;

    // Get Civil 3D application
    hr = CLSIDFromProgID(L"AeccXUiLand.AeccApplication", &clsid);
    if (FAILED(hr) ||
        FAILED(GetActiveObject(clsid, nullptr, (IUnknown**)&pAeccApp)))
    {
        acutPrintf(L"\n[ERROR] Civil 3D COM not available.");
        CoUninitialize();
        return;
    }

    acutPrintf(L"\n[OK] Connected to Civil 3D.");

    // ActiveDocument
    DISPID dispid;
    VARIANT varDoc;

    if (!GetDispId(pAeccApp, L"ActiveDocument", dispid) ||
        !InvokeGet(pAeccApp, dispid, varDoc))
    {
        acutPrintf(L"\n[ERROR] Failed to get ActiveDocument.");
        pAeccApp->Release();
        CoUninitialize();
        return;
    }

    IDispatch* pDoc = varDoc.pdispVal;

    // PressureNetworks collection
    VARIANT varNetworks;
    if (!GetDispId(pDoc, L"PressureNetworks", dispid) ||
        !InvokeGet(pDoc, dispid, varNetworks))
    {
        acutPrintf(L"\n[ERROR] PressureNetworks not available.");
        pDoc->Release();
        pAeccApp->Release();
        CoUninitialize();
        return;
    }

    IDispatch* pNetworks = varNetworks.pdispVal;

    // Add Pressure Network
    VARIANT varNetwork;
    if (!GetDispId(pNetworks, L"Add", dispid) ||
        !InvokeMethod_String(pNetworks, dispid,
            L"WS_PRO_POC_NETWORK", varNetwork))
    {
        acutPrintf(L"\n[ERROR] Failed to create Pressure Network.");
    }
    else
    {
        acutPrintf(L"\n[OK] Pressure Network created successfully.");
    }

    // Cleanup
    if (varNetwork.vt == VT_DISPATCH && varNetwork.pdispVal)
        varNetwork.pdispVal->Release();

    pNetworks->Release();
    pDoc->Release();
    pAeccApp->Release();

    CoUninitialize();
}

// -----------------------------------------------------
// ObjectARX Entry Point
// -----------------------------------------------------
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
            CreatePressureNetwork_NoTLB
        );
        break;

    case AcRx::kUnloadAppMsg:
        acedRegCmds->removeGroup(L"WSPOC_GROUP");
        break;
    }
    return AcRx::kRetOK;
}
