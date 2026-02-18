# Implementation Plan: WS Pro to Civil 3D Exchange Plugin

## Overview

This implementation plan breaks down the development of the Civil 3D plugin into discrete coding tasks. The plugin will be implemented as a C++ ARX module that imports WS Pro hydraulic networks into Civil 3D as native Pressure Network objects with full BIM attributes and change tracking capabilities.

## Tasks

- [x] 1. Set up core data structures and entity models
  - Create header files for WS Pro entity structures (PipeEntity, NodeEntity, ValveEntity, PumpEntity, ReservoirEntity)
  - Create NetworkData structure to hold collections of entities
  - Create Civil3DPartSpec structure for mapping specifications
  - Create BIMAttributes structure for metadata
  - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 5.1, 5.2, 5.3, 5.4, 5.5, 5.6_

- [ ]* 1.1 Write property test for entity data structures
  - **Property 9: BIM Attribute Completeness**
  - **Validates: Requirements 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7**

- [ ] 2. Implement File Parser component
  - [ ] 2.1 Create WSProFileParser class with parse() and isValidWSProFile() methods
    - Implement WS Pro file format detection
    - Parse pipe entities from file
    - Parse node entities from file
    - Parse valve, pump, and reservoir entities
    - Handle file I/O errors gracefully
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5_

  - [ ]* 2.2 Write property test for entity count preservation
    - **Property 1: Entity Count Preservation**
    - **Validates: Requirements 1.1, 1.2, 1.3, 1.4, 1.5**

  - [ ]* 2.3 Write unit tests for file parser
    - Test parsing valid WS Pro files
    - Test handling invalid file formats
    - Test handling corrupted data
    - Test handling missing required fields
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5_

- [ ] 3. Implement Unit Converter component
  - [ ] 3.1 Create UnitConverter class with conversion methods
    - Implement initialize() to detect Civil 3D project units
    - Implement convertLength() for SI to project units
    - Implement convertDiameter() for millimeter handling
    - Implement convertElevation() for elevation values
    - Implement reverse conversion methods for export
    - _Requirements: 4.1, 4.2, 4.3, 4.4, 4.5, 4.6_

  - [ ]* 3.2 Write property test for unit conversion round trip
    - **Property 7: Unit Conversion Round Trip**
    - **Validates: Requirements 4.1, 4.2**

  - [ ]* 3.3 Write property test for unit storage invariants
    - **Property 8: Unit Storage Invariants**
    - **Validates: Requirements 4.3, 4.4, 4.5, 4.6**

  - [ ]* 3.4 Write unit tests for unit converter
    - Test conversion with different project unit systems
    - Test edge cases (zero, negative, very large values)
    - Test floating-point precision handling
    - _Requirements: 4.1, 4.2, 4.3, 4.4, 4.5, 4.6_

- [ ] 4. Implement Entity Mapper component
  - [ ] 4.1 Create mapping configuration data structures
    - Create MappingRule structure
    - Create MappingRuleSet structure
    - Implement JSON/XML configuration file parser
    - _Requirements: 2.5, 2.6_

  - [ ] 4.2 Create EntityMapper class with mapping methods
    - Implement loadMappingRules() to read configuration
    - Implement mapPipe() to map pipe entities to Civil 3D parts
    - Implement mapValve(), mapPump() for other entity types
    - Implement rule priority resolution logic
    - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.7_

  - [ ]* 4.3 Write property test for attribute mapping completeness
    - **Property 3: Attribute Mapping Completeness**
    - **Validates: Requirements 2.1, 2.2, 2.3, 2.4**

  - [ ]* 4.4 Write property test for highest priority rule selection
    - **Property 4: Highest Priority Rule Selection**
    - **Validates: Requirements 2.7**

  - [ ]* 4.5 Write unit tests for entity mapper
    - Test loading mapping rules from JSON
    - Test loading mapping rules from XML
    - Test rule matching with single rule
    - Test rule matching with multiple rules
    - Test default mapping when no rules match
    - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7_

- [ ] 5. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 6. Implement Attribute Manager component
  - [ ] 6.1 Create AttributeManager class
    - Implement attachAttributes() to add BIM data to Civil 3D objects
    - Implement getAttributes() to retrieve BIM data
    - Implement updateAttribute() for individual attribute updates
    - Use Civil 3D XData or extension dictionary for storage
    - _Requirements: 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7_

  - [ ]* 6.2 Write property test for BIM attribute completeness
    - **Property 9: BIM Attribute Completeness**
    - **Validates: Requirements 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7**

  - [ ]* 6.3 Write unit tests for attribute manager
    - Test attaching attributes to objects
    - Test retrieving attributes from objects
    - Test updating individual attributes
    - Test handling missing attributes
    - _Requirements: 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7_

- [ ] 7. Implement Persistence Manager component
  - [ ] 7.1 Create PersistenceManager class
    - Implement registerObject() to track unique IDs
    - Implement findObject() to locate objects by unique ID
    - Implement objectExists() to check for existing objects
    - Implement updateObject() to update mappings
    - Implement getAllObjects() to retrieve all tracked objects
    - Use persistent storage (drawing database or external file)
    - _Requirements: 6.1, 6.2_

  - [ ]* 7.2 Write property test for re-import identity preservation
    - **Property 10: Re-import Identity Preservation**
    - **Validates: Requirements 6.1, 6.2**

  - [ ]* 7.3 Write property test for user edit preservation
    - **Property 11: User Edit Preservation**
    - **Validates: Requirements 6.3, 6.4**

  - [ ]* 7.4 Write property test for deleted entity flagging
    - **Property 12: Deleted Entity Flagging**
    - **Validates: Requirements 6.5**

  - [ ]* 7.5 Write unit tests for persistence manager
    - Test registering new objects
    - Test finding existing objects
    - Test handling duplicate unique IDs
    - Test handling orphaned objects
    - _Requirements: 6.1, 6.2, 6.3, 6.4, 6.5_

- [ ] 8. Implement Civil3D Object Factory component
  - [ ] 8.1 Create Civil3DObjectFactory class
    - Implement createPressurePipe() using Civil 3D API
    - Implement createPressureNode() using Civil 3D API
    - Implement createValve() for valve entities
    - Implement createPump() for pump entities
    - Implement createReservoir() for reservoir/tank entities
    - Handle Civil 3D API errors and transactions
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5_

  - [ ]* 8.2 Write property test for connectivity preservation
    - **Property 2: Connectivity Preservation**
    - **Validates: Requirements 1.6**

  - [ ]* 8.3 Write unit tests for object factory
    - Test creating pressure pipes with valid parameters
    - Test creating nodes with valid parameters
    - Test handling object creation failures
    - Test transaction rollback on errors
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 1.6_

- [ ] 9. Implement Profile Generator component
  - [ ] 9.1 Create ProfileGenerator class
    - Implement generatePipeProfile() to create pipe profiles
    - Implement generateSurfaceProfile() for existing ground
    - Implement validateProfile() to check Civil 3D design rules
    - Handle profile creation errors gracefully
    - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5_

  - [ ]* 9.2 Write property test for profile generation completeness
    - **Property 5: Profile Generation Completeness**
    - **Validates: Requirements 3.1**

  - [ ]* 9.3 Write property test for profile elevation accuracy
    - **Property 6: Profile Elevation Accuracy**
    - **Validates: Requirements 3.3, 3.4**

  - [ ]* 9.4 Write unit tests for profile generator
    - Test generating profiles for single pipe
    - Test generating profiles for multiple pipes
    - Test handling missing elevation data
    - Test validation against Civil 3D rules
    - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5_

- [ ] 10. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 11. Implement Change Tracker component
  - [ ] 11.1 Create Change Tracker data structures
    - Create Change structure with type, description, timestamp, impact
    - Create ChangeType and ChangeImpact enumerations
    - _Requirements: 8.1, 8.2, 8.3, 8.5, 8.6_

  - [ ] 11.2 Create ChangeTracker class
    - Implement trackChange() to record modifications
    - Implement getChanges() to retrieve object changes
    - Implement getChangedObjects() to list all changed objects
    - Implement classifyChange() to determine impact
    - Implement generateReport() to create change summary
    - Set up Civil 3D object reactors to detect modifications
    - _Requirements: 8.1, 8.2, 8.3, 8.4, 8.5, 8.6_

  - [ ]* 11.3 Write property test for change detection completeness
    - **Property 14: Change Detection Completeness**
    - **Validates: Requirements 8.1, 8.2, 8.3**

  - [ ]* 11.4 Write property test for change report completeness
    - **Property 15: Change Report Completeness**
    - **Validates: Requirements 8.4**

  - [ ]* 11.5 Write property test for change source classification
    - **Property 16: Change Source Classification**
    - **Validates: Requirements 8.5**

  - [ ]* 11.6 Write property test for hydraulic impact detection
    - **Property 17: Hydraulic Impact Detection**
    - **Validates: Requirements 8.6**

  - [ ]* 11.7 Write unit tests for change tracker
    - Test tracking geometry changes
    - Test tracking attribute changes
    - Test change classification logic
    - Test report generation
    - _Requirements: 8.1, 8.2, 8.3, 8.4, 8.5, 8.6_

- [ ] 12. Implement main import orchestration
  - [ ] 12.1 Create main import command handler
    - Wire together all components (parser, mapper, factory, etc.)
    - Implement transaction management for atomic imports
    - Implement progress reporting for large networks
    - Handle errors at each stage gracefully
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 2.1, 2.2, 2.3, 2.4, 2.7, 3.1, 3.2, 3.3, 3.4, 3.5, 4.1, 4.2, 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7_

  - [ ]* 12.2 Write integration tests for end-to-end import
    - Test importing complete WS Pro network files
    - Test re-importing networks with changes
    - Test handling large networks (1000+ entities)
    - Test error recovery and rollback
    - _Requirements: All_

- [ ] 13. Implement re-import functionality
  - [ ] 13.1 Add re-import logic to import command
    - Use PersistenceManager to find existing objects
    - Update existing objects instead of creating new ones
    - Preserve user edits where source hasn't changed
    - Flag deleted entities for review
    - _Requirements: 6.1, 6.2, 6.3, 6.4, 6.5_

  - [ ]* 13.2 Write property test for connectivity invariant after edits
    - **Property 13: Connectivity Invariant After Edits**
    - **Validates: Requirements 7.6**

  - [ ]* 13.3 Write integration tests for re-import scenarios
    - Test re-importing unchanged network
    - Test re-importing with source changes
    - Test re-importing with user edits
    - Test re-importing with deleted entities
    - _Requirements: 6.1, 6.2, 6.3, 6.4, 6.5_

- [ ] 14. Implement ARX plugin entry points
  - [ ] 14.1 Create ARX initialization and command registration
    - Implement acrxEntryPoint() for plugin lifecycle
    - Register WSPRO_IMPORT command
    - Register WSPRO_REIMPORT command
    - Register WSPRO_CHANGES command for change report
    - Set up plugin resources and cleanup
    - _Requirements: All_

  - [ ]* 14.2 Write unit tests for command handlers
    - Test command registration
    - Test command invocation
    - Test error handling in commands
    - _Requirements: All_

- [ ] 15. Add configuration and settings UI
  - [ ] 15.1 Create configuration file templates
    - Create default mapping rules JSON file
    - Create example XML mapping file
    - Document configuration file format
    - _Requirements: 2.5, 2.6_

  - [ ] 15.2 Add command to reload configuration
    - Implement WSPRO_CONFIG command
    - Allow runtime configuration reload
    - Validate configuration on load
    - _Requirements: 2.5, 2.6_

- [ ] 16. Final checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 17. Create sample data and documentation
  - Create sample WS Pro network files for testing
  - Create sample mapping configuration files
  - Document command usage and workflow
  - Document configuration file format
  - _Requirements: All_

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation at reasonable breaks
- Property tests validate universal correctness properties with minimum 100 iterations each
- Unit tests validate specific examples and edge cases
- All tests use Google Test framework with RapidCheck for property-based testing
- Each property test must include a comment tag: **Feature: ws-pro-civil3d-exchange, Property N: [property text]**
