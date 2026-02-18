#pragma once
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include "gepnt3d.h"

// Forward declarations for AutoCAD types
class AcGePoint3d;

namespace WSProCivil3D {

// ============================================================================
// WS Pro Entity Models
// ============================================================================

/// Represents a pipe entity from WS Pro
struct PipeEntity {
    std::string id;
    std::string startNodeId;
    std::string endNodeId;
    double diameter;            // mm
    double length;              // m
    std::string material;
    std::string pressureClass;
    double wallThickness;       // mm (optional, 0.0 if not specified)
    double roughness;
    double designFlow;          // m³/s
    std::string pressureZone;
    double startInvert;         // m
    double endInvert;           // m
    AcGePoint3d startPoint;
    AcGePoint3d endPoint;

    PipeEntity() 
        : diameter(0.0), length(0.0), wallThickness(0.0), 
          roughness(0.0), designFlow(0.0), 
          startInvert(0.0), endInvert(0.0) {}
};

/// Represents a node/junction entity from WS Pro
struct NodeEntity {
    std::string id;
    std::string type;           // junction, connection, etc.
    AcGePoint3d location;
    double elevation;           // m
    double groundLevel;         // m
    std::string pressureZone;

    NodeEntity() 
        : elevation(0.0), groundLevel(0.0) {}
};

/// Represents a valve entity from WS Pro
struct ValveEntity {
    std::string id;
    std::string type;           // isolating, control, check, etc.
    AcGePoint3d location;
    double diameter;            // mm
    std::string status;         // open, closed, throttled

    ValveEntity() 
        : diameter(0.0) {}
};

/// Represents a pump entity from WS Pro
struct PumpEntity {
    std::string id;
    AcGePoint3d location;
    double ratedFlow;           // m³/s
    double ratedHead;           // m
    std::string pumpCurve;

    PumpEntity() 
        : ratedFlow(0.0), ratedHead(0.0) {}
};

/// Represents a reservoir or tank entity from WS Pro
struct ReservoirEntity {
    std::string id;
    AcGePoint3d location;
    double waterLevel;          // m
    double capacity;            // m³
    std::string type;           // reservoir, tank

    ReservoirEntity() 
        : waterLevel(0.0), capacity(0.0) {}
};

/// Container for all network entities parsed from WS Pro file
struct NetworkData {
    std::vector<PipeEntity> pipes;
    std::vector<NodeEntity> nodes;
    std::vector<ValveEntity> valves;
    std::vector<PumpEntity> pumps;
    std::vector<ReservoirEntity> reservoirs;

    /// Get total entity count
    size_t getTotalEntityCount() const {
        return pipes.size() + nodes.size() + valves.size() + 
               pumps.size() + reservoirs.size();
    }

    /// Clear all entities
    void clear() {
        pipes.clear();
        nodes.clear();
        valves.clear();
        pumps.clear();
        reservoirs.clear();
    }
};

// ============================================================================
// Civil 3D Part Specification
// ============================================================================

/// Specification for mapping to Civil 3D parts
struct Civil3DPartSpec {
    std::string partFamily;         // e.g., "PVC Pipe"
    std::string partSize;           // e.g., "200mm"
    std::string material;
    std::string pressureRating;
    double nominalDiameter;         // mm
    double wallThickness;           // mm

    Civil3DPartSpec() 
        : nominalDiameter(0.0), wallThickness(0.0) {}
};

// ============================================================================
// BIM Attributes
// ============================================================================

/// BIM metadata attached to Civil 3D objects
struct BIMAttributes {
    std::string uniqueId;
    double roughness;
    double designFlow;              // m³/s
    std::string pressureZone;
    std::string sourceSystem;       // "WS Pro"
    std::time_t lastSyncTimestamp;

    BIMAttributes() 
        : roughness(0.0), designFlow(0.0), 
          sourceSystem("WS Pro"), lastSyncTimestamp(0) {}
};

// ============================================================================
// Mapping Configuration Models
// ============================================================================

/// A single mapping rule for entity to part conversion
struct MappingRule {
    int priority;                                   // Higher priority rules applied first
    std::map<std::string, std::string> conditions;  // Property conditions to match
    std::map<std::string, std::string> mappings;    // Source to target mappings

    MappingRule() : priority(0) {}
};

/// Collection of mapping rules with metadata
struct MappingRuleSet {
    std::vector<MappingRule> rules;
    std::string version;
    std::time_t lastModified;

    MappingRuleSet() : lastModified(0) {}

    /// Sort rules by priority (highest first)
    void sortByPriority() {
        std::sort(rules.begin(), rules.end(), 
            [](const MappingRule& a, const MappingRule& b) {
                return a.priority > b.priority;
            });
    }
};

} // namespace WSProCivil3D
