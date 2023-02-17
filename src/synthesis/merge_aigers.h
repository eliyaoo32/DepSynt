#ifndef MERGE_AIGERS_H
#define MERGE_AIGERS_H

#include <fstream>
#include <spot/twaalgos/aiger.hh>
#include <string>
#include <vector>

#include "utils.h"

using namespace std;

#define AIGER_STORE_PATH(X) "/tmp/" + X + ".aag"
#define BLIF_TMP_VAR(X) "In" + X

#define ABC_BLIF_TO_AIG(BLIF_PATH, AIGER_PATH)                               \
    "read " + BLIF_PATH +                                                    \
        "; strash; rewrite; balance; refactor; rewrite; balance; refactor; " \
        "rewrite; balance; refactor; write " +                               \
        AIGER_PATH + ";"

class MergeStrategies {
   private:
    spot::aig_ptr m_indeps_aiger;
    spot::aig_ptr deps_aiger;

    vector<string> inputs;
    vector<string> outputs;

    vector<string> indeps_outputs;
    vector<string> deps_outputs;

    string model_name;

    void aiger_to_blif(spot::aig_ptr aiger, string& dst, const char* prefix) {
        assert(aiger != nullptr && "Canont convert nullptr to blif");

        string aiger_path = AIGER_STORE_PATH(model_name + prefix);

        ofstream aiger_file_stream(aiger_path);
        spot::print_aiger(aiger_file_stream, aiger);
        aiger_file_stream.close();

        exec(("aigtoblif " + aiger_path).c_str(), dst);
    }

    void merge_blifs(string& indeps_blif, string& deps_blif) {
        string blif_inputs;   // TODO: inputs seperated by space
        string blif_outputs;  // TODO: outputs seperated by space

        string merged_blif;
        merged_blif += ".model " + model_name + "\r\n";
        merged_blif += ".inputs " + blif_inputs + "\r\n";
        merged_blif += ".outputs " + blif_outputs + "\r\n";

        for (int i = 0; i < outputs.size(); i++) {
            merged_blif +=
                ".names " + BLIF_TMP_VAR(outputs[i]) + " " + outputs[i] + "\r\n";
            merged_blif += "1 1\r\n";
        }

        merged_blif += ".subckt \r\n";
        merged_blif += ".end \r\n";

        merged_blif += indeps_blif;
        merged_blif += deps_blif;
    }

   public:
    ~MergeStrategies() {
        // TODO: remove tmp files
    }

    spot::aig_ptr merge() {
        // TODO: add validation that commands: aigtoblif, abc are exists

        assert(indeps_outputs.size() != outputs.size() && "No dependent outputs");
        assert(deps_outputs.size() > 0 && "No dependent outputs");

        string indeps_blif, deps_blif;
        aiger_to_blif(m_indeps_aiger, indeps_blif, "indeps");
        aiger_to_blif(deps_aiger, deps_blif, "deps");

        string merged_blif = merge_blifs(indeps_blif, deps_blif, inputs, outputs,
                                         indeps_outputs, deps_outputs);
        spot::aig_ptr merged_aiger = blif_to_aiger(merged_blif);
        return merged_aiger;
    }
}

#endif
