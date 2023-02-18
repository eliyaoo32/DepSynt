#ifndef MERGE_AIGERS_H
#define MERGE_AIGERS_H

#include <boost/algorithm/string/join.hpp>
#include <fstream>
#include <spot/twa/bdddict.hh>
#include <spot/twaalgos/aiger.hh>
#include <string>
#include <vector>

#include "utils.h"

using namespace std;

#define AAG_PATH(X) "/tmp/" + X + ".aag"
#define AIG_PATH(X) "/tmp/" + X + ".aig"
#define BLIF_STORE_PATH(X) "/tmp/" + X + ".blif"
#define INDEPENDENT_BLIF_MODEL "independent_model"
#define DEPENDENT_BLIF_MODEL "dependent_model"

#define ABC_COMMAND_BLIF_TO_AIG                                                   \
    "read {}; ; strash; rewrite; balance; refactor; rewrite; balance; refactor; " \
    "rewrite; balance; refactor; write {};"

class MergeStrategies {
   private:
    spot::aig_ptr m_indeps_aiger;
    spot::aig_ptr deps_aiger;

    vector<string> inputs;
    vector<string> outputs;
    vector<string> indeps_outputs;
    vector<string> deps_outputs;

    string model_name;

    void aiger_to_blif(spot::aig_ptr aiger, string& dst, const char* blif_name) {
        assert(aiger != nullptr && "Canont convert nullptr to blif");

        string aiger_path = AAG_PATH(blif_name);

        ofstream aiger_file_stream(aiger_path);
        spot::print_aiger(aiger_file_stream, aiger);
        aiger_file_stream.close();

        exec(("aigtoblif " + aiger_path).c_str(), dst);
        dst = replaceFirstLine(dst, ".model " + string(blif_name));

        // TODO: Remove the file here maybe?
    }

    void merge_blifs(std::ostream& out, string& indeps_blif, string& deps_blif) {
        string blif_inputs = boost::algorithm::join(inputs, " ");

        // Base of the blif
        out << ".model " << model_name << endl;
        out << ".inputs " << blif_inputs << endl;
        out << ".outputs " << boost::algorithm::join(outputs, " ") << endl;

        // Declare Wired Variables
        for (int i = 0; i < outputs.size(); i++) {
            out << ".names " << blif_wired_var(outputs[i]) << " " << outputs[i]
                << endl;
            out << "1 1" << endl;
        }

        // Create Input Connectors
        string input_wires = "";
        string indeps_wires = "";
        string deps_wires = "";

        for (int i = 0; i < inputs.size(); i++) {
            input_wires += inputs[i] + "=" + inputs[i];
        }
        for (int i = 0; i < indeps_outputs.size(); i++) {
            indeps_wires +=
                indeps_outputs[i] + "=" + blif_wired_var(indeps_outputs[i]);
        }
        for (int i = 0; i < deps_outputs.size(); i++) {
            deps_wires += deps_outputs[i] + "=" + blif_wired_var(deps_outputs[i]);
        }

        // Create subckt
        out << ".subckt " << string(INDEPENDENT_BLIF_MODEL) << " " << input_wires
            << " " << indeps_wires << endl;
        out << ".subckt " << string(DEPENDENT_BLIF_MODEL) << " " << input_wires
            << " " << indeps_wires << " " << deps_wires << endl;

        out << ".end" << endl;
        out << indeps_blif << endl;
        out << deps_blif << endl;
    }

    spot::aig_ptr blif_to_aiger(string& blif_path) {
        string aiger_path = AIG_PATH(model_name);
        string aager_path = AAG_PATH(model_name);

        // Convert blif to aiger via ABC
        string exec_res;
        string cmd = "read " + blif_path + "; strash; " +
                     "rewrite; balance; refactor; "
                     "rewrite; balance; refactor; "
                     "rewrite; balance; refactor; " +
                     "write " + aiger_path + ";";

        exec(cmd.c_str(), exec_res);

        // Convert aiger to ascii via aigtoaig
        exec(("aigtoaig " + aiger_path + " " + aager_path).c_str(), exec_res);
        return spot::aig::parse_aag(aager_path);
    }

   public:
    ~MergeStrategies() {
        // TODO: remove tmp files
    }

    spot::aig_ptr merge() {
        // TODO: add validation that commands: aigtoblif, aigtoaig, abc are exists
        assert(indeps_outputs.size() != outputs.size() && "No dependent outputs");
        assert(deps_outputs.size() > 0 && "No dependent outputs");

        // Create BLIF files for each strategy
        string indeps_blif, deps_blif;
        aiger_to_blif(m_indeps_aiger, indeps_blif, INDEPENDENT_BLIF_MODEL);
        aiger_to_blif(deps_aiger, deps_blif, DEPENDENT_BLIF_MODEL);

        // Create BLIF file
        string blif_path = BLIF_STORE_PATH(model_name);
        ofstream blif_file_stream(blif_path);
        merge_blifs(blif_file_stream, indeps_blif, deps_blif);
        blif_file_stream.close();

        // Convert BLIF to AIGER via ABC
        return blif_to_aiger(blif_path);
    }
};

string blif_wired_var(string var) { return "In" + var; }

void aiger_to_blif(spot::aig_ptr aiger, string& blif_dst, string blif_name) {
    // Create a file of the aiger
    // TODO: extract this constant to a global variable
    string aiger_path = "/tmp/" + blif_name + ".aag";
    ofstream aiger_file_stream(aiger_path);
    spot::print_aiger(aiger_file_stream, aiger);
    aiger_file_stream.close();

    // Convert aiger file to blif
    exec(("aigtoblif " + aiger_path).c_str(), blif_dst);
    blif_dst = replaceFirstLine(blif_dst, ".model " + string(blif_name));

    // TODO: Remove the file here maybe?
}

void merge_strategies_blifs(ostream& out, string& indeps_blif, string& deps_blif,
                            string& indeps_model_name, string& deps_model_name,
                            const vector<string>& inputs,
                            const vector<string>& independent_vars,
                            const vector<string>& dependent_vars,
                            string& model_name) {
    string blif_inputs = boost::algorithm::join(inputs, " ");
    string blif_independent_vars = boost::algorithm::join(independent_vars, " ");
    string blif_dependent_vars = boost::algorithm::join(dependent_vars, " ");

    // Base of the blif
    out << ".model " << model_name << endl;
    out << ".inputs " << blif_inputs << endl;
    out << ".outputs " << blif_independent_vars << " " << blif_dependent_vars
        << endl;

    // Declare Wired Variables
    for (int i = 0; i < independent_vars.size(); i++) {
        out << ".names " << blif_wired_var(independent_vars[i]) << " "
            << independent_vars[i] << endl;
        out << "1 1" << endl;
    }
    for (int i = 0; i < dependent_vars.size(); i++) {
        out << ".names " << blif_wired_var(dependent_vars[i]) << " "
            << dependent_vars[i] << endl;
        out << "1 1" << endl;
    }

    // Create Circuits Connectors
    string input_wires = "";
    string indeps_wires = "";
    string deps_wires = "";

    for (int i = 0; i < inputs.size(); i++) {
        input_wires += inputs[i] + "=" + inputs[i];
    }
    for (int i = 0; i < independent_vars.size(); i++) {
        indeps_wires +=
            independent_vars[i] + "=" + blif_wired_var(independent_vars[i]);
    }
    for (int i = 0; i < dependent_vars.size(); i++) {
        deps_wires += dependent_vars[i] + "=" + blif_wired_var(dependent_vars[i]);
    }

    // Create subsckts
    out << ".subckt " << indeps_model_name << " " << input_wires << " "
        << indeps_wires << endl;
    out << ".subckt " << deps_model_name << " " << input_wires << " " << indeps_wires
        << " " << deps_wires << endl;
    out << ".end" << endl;

    // Attach subckts to the base
    out << indeps_blif << endl;
    out << deps_blif << endl;
}

spot::aig_ptr merge_strategies(spot::aig_ptr independent_strategy,
                               spot::aig_ptr dependent_strategy,
                               const vector<string>& inputs,
                               const vector<string>& independent_vars,
                               const vector<string>& dependent_vars,
                               spot::bdd_dict_ptr dict, string& model_name) {
    // TODO: validate the commands: aigtoblif, aigtoaig, abc are exists

    if (independent_vars.size() == 0 || independent_strategy == nullptr) {
        return dependent_strategy;
    }
    if (dependent_vars.size() == 0 || dependent_strategy == nullptr) {
        return independent_strategy;
    }

    // Create BLIF for dependent and independent strategies
    string indeps_blif, deps_blif;
    string deps_model_name = model_name + "_deps";
    string indeps_model_name = model_name + "_indeps";
    aiger_to_blif(dependent_strategy, deps_blif, deps_model_name);
    aiger_to_blif(independent_strategy, indeps_blif, indeps_model_name);

    // Merge BLIF
    // TODO: extract this constant to a global variable
    std::string_view merged_blif_path = "/tmp/" + model_name + ".blif";
    ofstream merged_blif_file(merged_blif_path);
    merge_strategies_blifs(merged_blif_file, indeps_blif, deps_blif,
                           indeps_model_name, deps_model_name, inputs,
                           independent_vars, dependent_vars, model_name);
    merged_blif_file.close();

    spot::aig_ptr merged_strategy = blif_file_to_aiger(merged_blif_path);
    // TODO: remove files
    return merged_strategy;
}

#endif
