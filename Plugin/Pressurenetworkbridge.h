#pragma once

// ObjectARX core headers — adjust paths to match your ARX SDK location
#include <adscodes.h>   // RTNORM, RTSTR, RTNONE, RTLB
//#include "adscodes.h"   // RTNORM, RTSTR, RTNONE, RTLB
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>

// ============================================================
//  PressureNetworkBridge.h
//  Bridges CSV pipe data → Civil 3D Pressure Network via
//  acedCommand() + .scr script execution (Option B).
//  No Civil 3D SDK required — works with ObjectARX only.
// ============================================================

namespace PNBridge
{
    // ----------------------------------------------------------
    //  Data structures (populated from your CSV reader)
    // ----------------------------------------------------------

    struct PipeNode
    {
        std::wstring id;        // Unique node ID  (e.g. "J-01")
        double       x;
        double       y;
        double       z;
        std::wstring type;      // "JUNCTION" | "HYDRANT" | "VALVE" | "SOURCE"
    };

    struct PipeSeg
    {
        std::wstring id;        // Segment ID  (e.g. "P-01")
        std::wstring startNode; // references PipeNode::id
        std::wstring endNode;
        double       diameter;  // mm
        std::wstring material;  // e.g. "PVC", "DUCTILE_IRON"
    };

    struct NetworkDef
    {
        std::wstring         name;          // Civil 3D network name
        std::wstring         description;
        std::wstring         partsListName; // Civil 3D parts list (must exist in drawing)
        std::wstring         layerName;     // target layer
        std::vector<PipeNode> nodes;
        std::vector<PipeSeg>  segments;
    };

    // ----------------------------------------------------------
    //  Result / error reporting
    // ----------------------------------------------------------

    enum class BridgeResult
    {
        OK = 0,
        ERR_CSV_PARSE,
        ERR_SCRIPT_WRITE,
        ERR_COMMAND_EXEC,
        ERR_INVALID_DATA,
    };

    struct BridgeError
    {
        BridgeResult code = BridgeResult::OK;
        std::wstring message;
        bool         ok()  const { return code == BridgeResult::OK; }
    };

    // ----------------------------------------------------------
    //  Main bridge class
    // ----------------------------------------------------------

    class PressureNetworkBridge
    {
    public:
        // ---- CSV import ------------------------------------------------
        BridgeError loadFromCSV(const std::wstring& nodesCsvPath,
            const std::wstring& segsCsvPath,
            NetworkDef& outNet);

        // ---- Script generation + execution ----------------------------
        //   Writes a .scr file then runs it via acedCommand.
        //   scriptDir defaults to %TEMP% if empty.
        BridgeError createPressureNetwork(const NetworkDef& net,
            const std::wstring& scriptDir = L"");

        // ---- Convenience: load + create in one call -------------------
        BridgeError run(const std::wstring& nodesCsvPath,
            const std::wstring& segsCsvPath,
            const std::wstring& networkName,
            const std::wstring& partsListName = L"Standard",
            const std::wstring& layerName = L"PRESSURE-NETWORK");

    private:
        // Script builders
        void writeHeader(std::wofstream& f, const NetworkDef& net);
        void writeSetCurrentLayer(std::wofstream& f, const std::wstring& layer);
        void writeCreateNetwork(std::wofstream& f, const NetworkDef& net);
        void writeAddJunction(std::wofstream& f, const PipeNode& node);
        void writeAddHydrant(std::wofstream& f, const PipeNode& node);
        void writeAddValve(std::wofstream& f, const PipeNode& node);
        void writeAddPipe(std::wofstream& f, const PipeSeg& seg,
            const NetworkDef& net);

        // Helpers
        const PipeNode* findNode(const NetworkDef& net, const std::wstring& id) const;
        std::wstring    tempDir() const;
        std::wstring    escapeForScript(const std::wstring& s) const;
        std::wstring    fmt(double v, int decimals = 4) const;

        // State
        std::wstring m_lastScriptPath;
    };

} // namespace PNBridge