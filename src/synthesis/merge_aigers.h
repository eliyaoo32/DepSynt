#ifndef MERGE_AIGERS_H
#define MERGE_AIGERS_H

#include <spot/twaalgos/aiger.hh>

spot::aig_ptr merge_aigers(spot::aig_ptr indeps_aiger, spot::aig_ptr deps_aiger,
                           vector<string> inputs, vector<string> outputs) {
    // BLIF_indeps = AIGER_TO_BLIF(indeps_aiger)
    // BLIF_deps = AIGER_TO_BLIF(deps_aiger)

    // Create a BLIF model with the following structure:
    // .model merged
    // .inputs i1 ... in
    // .outputs o1 ... on
    // For each output variable: create
    // .names InO1  o1
    // 1 1

    // Conncet subcircuits:
    // .subckt deps-circ i1=i1 .... in=in o1=InO1 o2=InO2 ... on=InOn
    // .subckt indeps-circ i1=i1 .... in=in o1=InO1 o2=InO2 ... on=InOk (Only for
    // independent output)

    // Read the BLIF model into ABC
    // Write the merged circuit into AIGER
}

#endif
