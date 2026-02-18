# Data Models Documentation

## Overview

This file documents the core data structures created for the WS Pro to Civil 3D exchange plugin.

## Files Created

### DataModels.h
Contains all core data structures for the plugin:

#### WS Pro Entity Models
- `PipeEntity` - Represents pipes with diameter, material, roughness, flow, etc.
- `NodeEntity` - Represents nodes/junctions with elevation and location
- `ValveEntity` - Represents valves with type and status
- `PumpEntity` - Represents pumps with rated flow and head
- `ReservoirEntity` - Represents reservoirs/tanks with capacity

#### Container Structures
- `NetworkData` - Container for all network entities with utility methods

#### Civil 3D Integration
- `Civil3DPartSpec` - Specification for mapping to Civil 3D parts
- `BIMAttributes` - BIM metadata for Civil 3D objects

#### Mapping Configuration
- `MappingRule` - Individual mapping rule with priority and conditions
- `MappingRuleSet` - Collection of mapping rules with sorting capability

## Design Decisions

### Units
All measurements follow the design specification:
- Diameters: millimeters (mm)
- Lengths: meters (m)
- Elevations: meters (m)
- Coordinates: meters (m)
- Flow: cubic meters per second (m³/s)

### Default Values
All numeric fields are initialized to 0.0 to prevent undefined behavior.

### Namespace
All structures are in the `WSProCivil3D` namespace to avoid naming conflicts.

## Testing

A test file `DataModels_Test.cpp` has been created to verify:
- All structures can be instantiated
- Default constructors work correctly
- Fields can be set and retrieved
- Container methods work as expected
- Sorting functionality works correctly

## Next Steps

These data structures will be used by:
1. File Parser - to populate NetworkData from WS Pro files
2. Entity Mapper - to use MappingRuleSet for part mapping
3. Object Factory - to create Civil 3D objects from entities
4. Attribute Manager - to attach BIMAttributes to objects
5. All other components in the plugin architecture
