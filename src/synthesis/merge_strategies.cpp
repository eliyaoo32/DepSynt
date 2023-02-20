#include "merge_strategies.h"

using namespace std;

#define AIGTOBLIF_CMD                                                       \
    "/Users/eliyahub/Thesis/reactive-synthesis-bfss/tools/aiger-utilities/" \
    "aigtoblif"
#define AIGTOAIG_CMD \
    "/Users/eliyahub/Thesis/reactive-synthesis-bfss/tools/aiger-utilities/aigtoaig"
#define ABC_CMD "/Users/eliyahub/Thesis/aiger/abc/abc"

spot::aig_ptr blif_file_to_aiger(string& blif_path, spot::bdd_dict_ptr dict,
                                 string& model_name) {
    // TODO: extract this constant to a global variable
    string bin_aig_path = "/tmp/" + model_name + ".aig";
    string ascii_aig_path = "/tmp/" + model_name + ".aag";

    string cmd_res;
    string cmd = string(ABC_CMD) + " -c \"read " + blif_path + "; strash; " +
                 "rewrite; balance; refactor; "
                 "rewrite; balance; refactor; "
                 "rewrite; balance; refactor; " +
                 "write " + bin_aig_path + ";\"";
    exec(cmd.c_str(), cmd_res);

    // Convert aiger to ascii via aigtoaig
    string cmd2 = string(AIGTOAIG_CMD) + " " + bin_aig_path + " " + ascii_aig_path;
    exec(cmd2.c_str(), cmd_res);

    // Load the ASCII Aiger
    return spot::aig::parse_aag(ascii_aig_path, dict);
    // TODO: remove the AIGER files
}

void aiger_to_blif(spot::aig_ptr aiger, string& blif_dst, string blif_name) {
    // Create a file of the aiger
    // TODO: extract this constant to a global variable
    string aiger_path = "/tmp/" + blif_name + ".aag";
    ofstream aiger_file_stream(aiger_path);
    spot::print_aiger(aiger_file_stream, aiger);
    aiger_file_stream << endl;
    aiger_file_stream.close();

    // Convert aiger file to blif
    string blif_path = "/tmp/" + blif_name + ".blif";
    string cmd = string(AIGTOBLIF_CMD) + " " + aiger_path;
    exec(cmd.c_str(), blif_dst);

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
        input_wires += inputs[i] + "=" + inputs[i] + " ";
    }
    for (int i = 0; i < independent_vars.size(); i++) {
        indeps_wires +=
            independent_vars[i] + "=" + blif_wired_var(independent_vars[i]) + " ";
    }
    for (int i = 0; i < dependent_vars.size(); i++) {
        deps_wires += dependent_vars[i] + "=" + blif_wired_var(dependent_vars[i]) + " ";
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
    std::string merged_blif_path = "/tmp/" + model_name + ".blif";
    ofstream merged_blif_file(merged_blif_path);
    merge_strategies_blifs(merged_blif_file, indeps_blif, deps_blif,
                           indeps_model_name, deps_model_name, inputs,
                           independent_vars, dependent_vars, model_name);
    merged_blif_file.close();

    spot::aig_ptr merged_strategy =
        blif_file_to_aiger(merged_blif_path, dict, model_name);

    // TODO: remove BLIF file
    return merged_strategy;
}
