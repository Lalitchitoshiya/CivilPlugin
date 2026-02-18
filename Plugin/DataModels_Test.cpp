// Simple compilation test for DataModels.h
// This file verifies that all data structures are properly defined

#include "DataModels.h"
#include <cassert>

using namespace WSProCivil3D;

// Test function to verify data structures can be instantiated
void testDataModels() {
    // Test PipeEntity
    PipeEntity pipe;
    pipe.id = "P001";
    pipe.diameter = 200.0;
    pipe.length = 100.0;
    pipe.material = "PVC";
    assert(pipe.diameter == 200.0);

    // Test NodeEntity
    NodeEntity node;
    node.id = "N001";
    node.type = "junction";
    node.elevation = 10.0;
    assert(node.elevation == 10.0);

    // Test ValveEntity
    ValveEntity valve;
    valve.id = "V001";
    valve.type = "isolating";
    valve.diameter = 150.0;
    assert(valve.diameter == 150.0);

    // Test PumpEntity
    PumpEntity pump;
    pump.id = "PU001";
    pump.ratedFlow = 0.5;
    pump.ratedHead = 50.0;
    assert(pump.ratedFlow == 0.5);

    // Test ReservoirEntity
    ReservoirEntity reservoir;
    reservoir.id = "R001";
    reservoir.type = "tank";
    reservoir.capacity = 1000.0;
    assert(reservoir.capacity == 1000.0);

    // Test NetworkData
    NetworkData network;
    network.pipes.push_back(pipe);
    network.nodes.push_back(node);
    network.valves.push_back(valve);
    network.pumps.push_back(pump);
    network.reservoirs.push_back(reservoir);
    assert(network.getTotalEntityCount() == 5);

    // Test Civil3DPartSpec
    Civil3DPartSpec partSpec;
    partSpec.partFamily = "PVC Pipe";
    partSpec.partSize = "200mm";
    partSpec.nominalDiameter = 200.0;
    assert(partSpec.nominalDiameter == 200.0);

    // Test BIMAttributes
    BIMAttributes bimAttrs;
    bimAttrs.uniqueId = "WS-P001";
    bimAttrs.roughness = 0.01;
    bimAttrs.designFlow = 0.1;
    bimAttrs.pressureZone = "Zone1";
    assert(bimAttrs.roughness == 0.01);

    // Test MappingRule
    MappingRule rule;
    rule.priority = 10;
    rule.conditions["diameter"] = "200";
    rule.mappings["material"] = "PVC";
    assert(rule.priority == 10);

    // Test MappingRuleSet
    MappingRuleSet ruleSet;
    MappingRule rule1, rule2;
    rule1.priority = 5;
    rule2.priority = 10;
    ruleSet.rules.push_back(rule1);
    ruleSet.rules.push_back(rule2);
    ruleSet.sortByPriority();
    assert(ruleSet.rules[0].priority == 10); // Highest priority first
}
