# Design Document: WS Pro to Civil 3D Exchange Plugin

## Overview

This plugin enables bidirectional data exchange between InfoWorks WS Pro and AutoCAD Civil 3D for hydraulic network design. The system imports WS Pro hydraulic networks as native Civil 3D Pressure Network objects, preserves engineering attributes through BIM metadata, supports editing in Civil 3D, and tracks changes for synchronization.

The plugin is implemented as a C++ AutoCAD ARX (AutoCAD Runtime Extension) module that integrates with Civil 3D's API. It provides import functionality that reads WS Pro network data, maps entities to Civil 3D objects, generates profiles, handles unit conversions, and maintains persistent links for round-trip synchronization.

## Architecture

### High-Level Architecture

```
┌─────────────────┐         ┌──────────────────┐         ┌─────────────────┐
│   WS Pro        │         │   Plugin         │         │   Civil 3D      │
│   Network File  │────────>│   (ARX Module)   │────────>│   Pressure      │
│   (.INP/.WSN)   │         │                  │         │   Network       │
└─────────────────┘         └──────────────────┘         └─────────────────┘
                                    │
                                    │
                            ┌───────▼────────┐
                            │  Mapping       │
                            │  Configuration │
                            │  (JSON/XML)    │
                            └────────────────┘
```

### Component Architecture

The plugin consists of the following major components:

1. **File Parser**: Reads WS Pro network files and extracts entity data
2. **Entity Mapper**: Maps WS Pro entities to Civil 3D part types using configuration rules
3. **Object Factory**: Creates Civil 3D Pressure Network objects with appropriate properties
4. **Profile Generator**: Creates pipe and surface profiles respecting vertical design rules
5. **Unit Converter**: Handles conversions between SI units and Civil 3D project units
6. **Attribute Manager**: Attaches and manages BIM attributes on Civil 3D objects
7. **Persistence Manager**: Maintains unique IDs and tracks object relationships across imports
8. **Change Tracker**: Monitors and flags modifications made in Civil 3D

## Components and Interfaces

### 1. File Parser

**Responsibility**: Parse WS Pro network files and extract entity data.

**Interface**:
```cpp
class WSProFileParser {
public:
    // Parse a WS Pro network file
    NetworkData parse(const std::string& filePath);
    
    // Validate file format
    bool isValidWSProFile(const std::string& filePath);
};

struct NetworkData {
    std::vector<PipeEntity> pipes;
    std::vector<NodeEntity> nodes;
    std::vector<ValveEntity> valves;
    std::vector<PumpEntity> pumps;
    std::vector<ReservoirEntity> reservoirs;
};
```

### 2. Entity Mapper

**Responsibility**: Map WS Pro entities to Civil 3D part types based on configuration rules.

**Interface**:
```cpp
class EntityMapper {
public:
    // Load mapping configuration
    void loadMappingRules(const std::string& configPath);
    
    // Map a pipe entity to Civil 3D part
    Civil3DPartSpec mapPipe(const PipeEntity& pipe);
    
    // Map other entity types
    Civil3DPartSpec mapValve(const ValveEntity& valve);
    Civil3DPartSpec mapPump(const PumpEntity& pump);
    
private:
    MappingRuleSet rules;
};

struct MappingRule {
    std::string sourceProperty;
    std::string targetProperty;
    std::map<std::string, std::string> valueMapping;
    int priority;
};
```

### 3. Object Factory

**Responsibility**: Create Civil 3D Pressure Network objects with appropriate properties.

**Interface**:
```cpp
class Civil3DObjectFactory {
public:
    // Create pressure pipe
    AcDbObjectId createPressurePipe(
        const PipeEntity& source,
        const Civil3DPartSpec& spec,
        const AcGePoint3d& startPoint,
        const AcGePoint3d& endPoint
    );
    
    // Create pressure network node
    AcDbObjectId createPressureNode(
        const NodeEntity& source,
        const Civil3DPartSpec& spec,
        const AcGePoint3d& location
    );
    
    // Create other network components
    AcDbObjectId createValve(const ValveEntity& source);
    AcDbObjectId createPump(const PumpEntity& source);
    AcDbObjectId createReservoir(const ReservoirEntity& source);
};
```

### 4. Profile Generator

**Responsibility**: Generate pipe and surface profiles respecting Civil 3D vertical design rules.

**Interface**:
```cpp
class ProfileGenerator {
public:
    // Generate profile for a pipe
    AcDbObjectId generatePipeProfile(
        AcDbObjectId pipeId,
        const std::vector<double>& invertLevels
    );
    
    // Generate surface profile
    AcDbObjectId generateSurfaceProfile(
        AcDbObjectId alignmentId,
        AcDbObjectId surfaceId
    );
    
    // Validate profile against design rules
    bool validateProfile(AcDbObjectId profileId);
};
```

### 5. Unit Converter

**Responsibility**: Convert between SI units (WS Pro) and Civil 3D project units.

**Interface**:
```cpp
class UnitConverter {
public:
    // Initialize with Civil 3D project units
    void initialize(const Civil3DUnits& projectUnits);
    
    // Convert length from SI to project units
    double convertLength(double meters);
    
    // Convert diameter (always stored in mm)
    double convertDiameter(double millimeters);
    
    // Convert elevation from SI to project units
    double convertElevation(double meters);
    
    // Reverse conversions for export
    double convertLengthToSI(double projectUnits);
    double convertElevationToSI(double projectUnits);
};
```

### 6. Attribute Manager

**Responsibility**: Attach and manage BIM attributes on Civil 3D objects.

**Interface**:
```cpp
class AttributeManager {
public:
    // Attach BIM attributes to an object
    void attachAttributes(
        AcDbObjectId objectId,
        const BIMAttributes& attributes
    );
    
    // Retrieve BIM attributes from an object
    BIMAttributes getAttributes(AcDbObjectId objectId);
    
    // Update specific attribute
    void updateAttribute(
        AcDbObjectId objectId,
        const std::string& key,
        const std::string& value
    );
};

struct BIMAttributes {
    std::string uniqueId;
    double roughness;
    double designFlow;
    std::string pressureZone;
    std::string sourceSystem;
    std::time_t lastSyncTimestamp;
};
```

### 7. Persistence Manager

**Responsibility**: Maintain unique IDs and track object relationships across imports.

**Interface**:
```cpp
class PersistenceManager {
public:
    // Register a new object with unique ID
    void registerObject(const std::string& uniqueId, AcDbObjectId objectId);
    
    // Find existing object by unique ID
    AcDbObjectId findObject(const std::string& uniqueId);
    
    // Check if object exists
    bool objectExists(const std::string& uniqueId);
    
    // Update object mapping
    void updateObject(const std::string& uniqueId, AcDbObjectId newObjectId);
    
    // Get all registered objects
    std::map<std::string, AcDbObjectId> getAllObjects();
};
```

### 8. Change Tracker

**Responsibility**: Monitor and flag modifications made in Civil 3D.

**Interface**:
```cpp
class ChangeTracker {
public:
    // Track a change to an object
    void trackChange(
        AcDbObjectId objectId,
        ChangeType type,
        const std::string& description
    );
    
    // Get all changes for an object
    std::vector<Change> getChanges(AcDbObjectId objectId);
    
    // Get all changed objects
    std::vector<AcDbObjectId> getChangedObjects();
    
    // Classify change impact
    ChangeImpact classifyChange(const Change& change);
    
    // Generate change report
    std::string generateReport();
};

enum class ChangeType {
    GeometryModified,
    AttributeModified,
    ObjectCreated,
    ObjectDeleted
};

enum class ChangeImpact {
    DesignOnly,
    HydraulicImpacting
};

struct Change {
    AcDbObjectId objectId;
    ChangeType type;
    std::string description;
    std::time_t timestamp;
    ChangeImpact impact;
};
```

## Data Models

### WS Pro Entity Models

```cpp
struct PipeEntity {
    std::string id;
    std::string startNodeId;
    std::string endNodeId;
    double diameter;        // mm
    double length;          // m
    std::string material;
    std::string pressureClass;
    double wallThickness;   // mm (optional)
    double roughness;
    double designFlow;      // m³/s
    std::string pressureZone;
    double startInvert;     // m
    double endInvert;       // m
    AcGePoint3d startPoint;
    AcGePoint3d endPoint;
};

struct NodeEntity {
    std::string id;
    std::string type;       // junction, connection, etc.
    AcGePoint3d location;
    double elevation;       // m
    double groundLevel;     // m
    std::string pressureZone;
};

struct ValveEntity {
    std::string id;
    std::string type;       // isolating, control, check, etc.
    AcGePoint3d location;
    double diameter;        // mm
    std::string status;     // open, closed, throttled
};

struct PumpEntity {
    std::string id;
    AcGePoint3d location;
    double ratedFlow;       // m³/s
    double ratedHead;       // m
    std::string pumpCurve;
};

struct ReservoirEntity {
    std::string id;
    AcGePoint3d location;
    double waterLevel;      // m
    double capacity;        // m³
    std::string type;       // reservoir, tank
};
```

### Civil 3D Part Specification

```cpp
struct Civil3DPartSpec {
    std::string partFamily;     // e.g., "PVC Pipe"
    std::string partSize;       // e.g., "200mm"
    std::string material;
    std::string pressureRating;
    double nominalDiameter;     // mm
    double wallThickness;       // mm
};
```

### Mapping Configuration Model

```cpp
struct MappingRuleSet {
    std::vector<MappingRule> rules;
    std::string version;
    std::time_t lastModified;
};

struct MappingRule {
    int priority;                           // Higher priority rules applied first
    std::map<std::string, std::string> conditions;  // Property conditions to match
    std::map<std::string, std::string> mappings;    // Source to target mappings
};
```

## Correctness Properties


A property is a characteristic or behavior that should hold true across all valid executions of a system—essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.

### Property 1: Entity Count Preservation

*For any* valid WS Pro network file, importing it should create exactly the same number of Civil 3D objects as there are entities in the source file (pipes, nodes, valves, pumps, and reservoirs combined).

**Validates: Requirements 1.1, 1.2, 1.3, 1.4, 1.5**

### Property 2: Connectivity Preservation

*For any* WS Pro network where pipe P connects nodes A and B, after import the corresponding Civil 3D pipe object should connect to the Civil 3D objects corresponding to nodes A and B.

**Validates: Requirements 1.6**

### Property 3: Attribute Mapping Completeness

*For any* pipe entity with diameter, material, pressure class, and optional wall thickness, the imported Civil 3D object should have all these properties correctly mapped according to the mapping rules.

**Validates: Requirements 2.1, 2.2, 2.3, 2.4**

### Property 4: Highest Priority Rule Selection

*For any* entity that matches multiple mapping rules, the rule with the highest priority value should be applied to determine the Civil 3D part specification.

**Validates: Requirements 2.7**

### Property 5: Profile Generation Completeness

*For any* set of imported pipes, the number of generated pipe profiles should equal the number of pipes.

**Validates: Requirements 3.1**

### Property 6: Profile Elevation Accuracy

*For any* pipe with specified invert levels and connected nodes with elevations, the generated profile should match these elevation values within the tolerance of the unit conversion precision.

**Validates: Requirements 3.3, 3.4**

### Property 7: Unit Conversion Round Trip

*For any* numeric value in SI units, converting to Civil 3D project units and then back to SI units should produce a value within acceptable floating-point precision of the original.

**Validates: Requirements 4.1, 4.2**

### Property 8: Unit Storage Invariants

*For any* imported entity, pipe diameters should be stored in millimeters, and all lengths, elevations, and coordinates should be stored in meters, regardless of the Civil 3D project unit settings.

**Validates: Requirements 4.3, 4.4, 4.5, 4.6**

### Property 9: BIM Attribute Completeness

*For any* imported entity, all hydraulic attributes (roughness, design flow, pressure zone) and metadata (unique ID, source system, timestamp) should be attached to the Civil 3D object and retrievable through the property interface.

**Validates: Requirements 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7**

### Property 10: Re-import Identity Preservation

*For any* network that is imported, then re-imported without source changes, the unique IDs of all Civil 3D objects should remain the same and no duplicate objects should be created.

**Validates: Requirements 6.1, 6.2**

### Property 11: User Edit Preservation

*For any* Civil 3D object that has been modified by the user, if the corresponding source entity has not changed in WS Pro, re-importing should preserve the user's modifications.

**Validates: Requirements 6.3, 6.4**

### Property 12: Deleted Entity Flagging

*For any* entity that exists in a previous import but not in the current WS Pro source data, the corresponding Civil 3D object should be flagged for review after re-import.

**Validates: Requirements 6.5**

### Property 13: Connectivity Invariant After Edits

*For any* pipe that is connected to two nodes, after any geometry edit (moving pipe, moving nodes, adjusting elevations, changing alignment), the pipe should remain connected to the same two nodes.

**Validates: Requirements 7.6**

### Property 14: Change Detection Completeness

*For any* modification to an object's geometry or attributes, the change tracker should flag that object as changed and categorize it as either a design change or hydraulic-impacting change.

**Validates: Requirements 8.1, 8.2, 8.3**

### Property 15: Change Report Completeness

*For any* set of modified objects, the generated change report should list all objects that have been flagged as changed, with no omissions.

**Validates: Requirements 8.4**

### Property 16: Change Source Classification

*For any* change recorded by the system, it should be classified as either a user edit or a system update, with no changes left unclassified.

**Validates: Requirements 8.5**

### Property 17: Hydraulic Impact Detection

*For any* change that modifies hydraulic-relevant properties (diameter, roughness, length, elevation, connectivity), the change should be marked as hydraulic-impacting.

**Validates: Requirements 8.6**

## Error Handling

### File Parsing Errors

- **Invalid file format**: Return error code with descriptive message, do not create partial imports
- **Missing required fields**: Log warning, use default values where safe, or skip entity with error report
- **Corrupted data**: Validate all numeric values, reject entities with invalid data

### Mapping Errors

- **No matching rule**: Use default mapping based on entity type, log warning
- **Ambiguous rules**: Apply highest priority rule, log which rule was selected
- **Invalid part specification**: Fall back to generic part type, flag for user review

### Civil 3D API Errors

- **Object creation failure**: Roll back transaction, report error with entity details
- **Profile generation failure**: Continue with import, flag affected pipes for manual profile creation
- **Attribute attachment failure**: Log error but continue import, report missing attributes

### Unit Conversion Errors

- **Unknown unit system**: Prompt user to specify units, do not proceed with import
- **Out of range values**: Clamp to valid range, log warning with original value

### Persistence Errors

- **Duplicate unique IDs**: Generate new unique ID, log conflict
- **Orphaned objects**: Flag for user review, provide cleanup utility
- **Missing linked objects**: Create new object, log that link was broken

## Testing Strategy

### Dual Testing Approach

This system will be validated using both unit tests and property-based tests:

- **Unit tests** verify specific examples, edge cases, and error conditions
- **Property tests** verify universal properties across all inputs
- Both approaches are complementary and necessary for comprehensive coverage

### Unit Testing

Unit tests will focus on:

- **Specific examples**: Test import of known WS Pro files with expected results
- **Edge cases**: Empty networks, single-entity networks, networks with missing optional data
- **Error conditions**: Invalid files, corrupted data, API failures
- **Integration points**: Interaction with Civil 3D API, file I/O, configuration loading

Example unit test scenarios:
- Import a network with 5 pipes and verify 5 Civil 3D pipes are created
- Import a pipe with no wall thickness and verify it uses default value
- Attempt to import an invalid file and verify appropriate error is returned
- Re-import a network and verify no duplicates are created

### Property-Based Testing

Property-based testing will be implemented using **Google Test with RapidCheck** (C++ property testing library).

Each property test will:
- Run a minimum of 100 iterations with randomly generated inputs
- Reference the design document property it validates
- Use the tag format: **Feature: ws-pro-civil3d-exchange, Property N: [property text]**

Example property test structure:
```cpp
// Feature: ws-pro-civil3d-exchange, Property 1: Entity Count Preservation
TEST(WSProImportProperties, EntityCountPreservation) {
    rc::check("imported entity count matches source", [](const NetworkData& network) {
        auto importedObjects = importNetwork(network);
        int sourceCount = network.pipes.size() + network.nodes.size() + 
                         network.valves.size() + network.pumps.size() + 
                         network.reservoirs.size();
        RC_ASSERT(importedObjects.size() == sourceCount);
    });
}
```

Property test generators will:
- Generate random but valid WS Pro network data
- Create networks with varying sizes (0-1000 entities)
- Include optional fields randomly
- Generate valid coordinate ranges and elevation values
- Create realistic material and pressure class combinations

### Test Coverage Goals

- **Unit test coverage**: Minimum 80% code coverage for all components
- **Property test coverage**: All 17 correctness properties must have corresponding property tests
- **Integration testing**: End-to-end import scenarios with real WS Pro files
- **Performance testing**: Import networks with 10,000+ entities within acceptable time limits

### Testing Infrastructure

- **Test framework**: Google Test for unit tests, RapidCheck for property tests
- **Mock objects**: Mock Civil 3D API for isolated component testing
- **Test data**: Repository of sample WS Pro files with known characteristics
- **Continuous integration**: Automated test execution on every commit

