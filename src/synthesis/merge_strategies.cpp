#include "merge_strategies.h"

using namespace std;

#define AIGTOBLIF_CMD   "aigtoblif"
#define AIGTOAIG_CMD "aigtoaig"
#define ABC_CMD "abc"
#define TMP_DIR "/tmp/"


spot::aig_ptr blif_file_to_aiger(string& blif_path, spot::bdd_dict_ptr dict,
                                 string& model_name) {
    // TODO: extract this constant to a global variable
    string bin_aig_path = TMP_DIR + model_name + ".aig";
    string ascii_aig_path = TMP_DIR + model_name + ".aag";

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
    auto parsed_aiger = spot::aig::parse_aag(ascii_aig_path, dict);

    // Remove the aiger files
    remove(bin_aig_path.c_str());
    remove(ascii_aig_path.c_str());

    return parsed_aiger;
}

string find_init_latch(string& blif, unsigned init_latch_var) {
    std::regex init_latch_pattern(".latch .* ([a-zA-Z]+"+std::to_string(init_latch_var)+") .*");
    std::smatch match;

    if(std::regex_search(blif, match, init_latch_pattern)) {
        return match[1];
    } else{
        cerr << "Couldn't find init latch in the blif file." << endl;
        return "";
    }
}

void aiger_to_blif(spot::aig_ptr aiger, string& blif_dst, string blif_name) {
    // Create a file of the aiger
    // TODO: extract this constant to a global variable
    string aiger_path = TMP_DIR + blif_name + ".aag";
    ofstream aiger_file_stream(aiger_path);
    spot::print_aiger(aiger_file_stream, aiger);
    aiger_file_stream << endl;
    aiger_file_stream.close();

    // Convert aiger file to blif
    string blif_path = TMP_DIR + blif_name + ".blif";
    string cmd = string(AIGTOBLIF_CMD) + " " + aiger_path;
    exec(cmd.c_str(), blif_dst);

    blif_dst = replaceFirstLine(blif_dst, ".model " + string(blif_name));

    remove(aiger_path.c_str());
}

// Create a middleware in the init latches values.
// In the first input, the init values are 0.
// The latch corresponding to the init state of the independent NBA must to be initialized to 1.
// This function creates a new latch that is initialized to 0 and its next value is always 1.
// When the new latch value is 0, the init value of the latch corresponding to the initialize state will become 1.

void deps_blif_latches_middleware(string& deps_strategy_blif, string& init_latch) {
    // Remove .end
    std::size_t end_ind = deps_strategy_blif.find(".end");
    if(end_ind != std::string::npos) {
        deps_strategy_blif.erase(end_ind, 4);
    }

        // Add const 1 if it is not in the blif
    if(deps_strategy_blif.find(".names c1") == string::npos) {
        deps_strategy_blif += ".names c1\r\n1\r\n\r\n";
    }

    // Define a new latch, init value is 0 and next value is always 1
    string tmp_latch = "lltmpinit";
    size_t tmp_latch_idx = deps_strategy_blif.find(".latch");
    assert(tmp_latch_idx != std::string::npos && "Error: .latch not found in the blif file.");
    deps_strategy_blif.insert(tmp_latch_idx, ".latch c1 " + tmp_latch + " 0\r\n\r\n");

    // Replace the latch with a temp init latch
    string init_latch_tmp = init_latch + "Tmp";
    std::regex init_latch_pattern(".latch (.*) (" + init_latch + ") (.*)");
    assert(std::regex_search(deps_strategy_blif, init_latch_pattern) && "Init latch not found in the blif file.");
    deps_strategy_blif = std::regex_replace(deps_strategy_blif, init_latch_pattern, ".latch $1 " + init_latch_tmp + " $3");

    // Add a new name, where it takes the new latch and the temp init latch and outputs the original init latch
    deps_strategy_blif += ".names " + init_latch_tmp + " " + tmp_latch + " " + init_latch + "\r\n"
                            "-0 1\r\n"
                            "11 1\r\n\r\n";

    // Add .end
    deps_strategy_blif += ".end\r\n\r\n";
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

spot::aig_ptr refine_dependent_strategy(spot::aig_ptr dependent_strategy, string& model_name, spot::bdd_dict_ptr dict) {
    string deps_blif;

    aiger_to_blif(dependent_strategy, deps_blif, model_name);
    string deps_init_latch = find_init_latch(deps_blif, dependent_strategy->latch_var(0));
    deps_blif_latches_middleware(deps_blif, deps_init_latch);

    std::string deps_blif_path = TMP_DIR + model_name + ".blif";
    std::ofstream deps_blif_file(deps_blif_path);
    deps_blif_file << deps_blif << endl;
    deps_blif_file.close();

    return blif_file_to_aiger(deps_blif_path, dict, model_name);
}

spot::aig_ptr merge_strategies(spot::aig_ptr independent_strategy,
                               spot::aig_ptr dependent_strategy,
                               const vector<string>& inputs,
                               const vector<string>& independent_vars,
                               const vector<string>& dependent_vars,
                               spot::bdd_dict_ptr dict, string& model_name) {
    // TODO: validate the commands: aigtoblif, aigtoaig, abc are exists

    if (independent_vars.empty() || independent_strategy == nullptr) {
        return refine_dependent_strategy(dependent_strategy, model_name, dict);
    }
    if (dependent_vars.empty() || dependent_strategy == nullptr) {
        return independent_strategy;
    }

    // Create BLIF for dependent and independent strategies
    string indeps_blif, deps_blif;
    string deps_model_name = model_name + "_deps";
    string indeps_model_name = model_name + "_indeps";
    aiger_to_blif(dependent_strategy, deps_blif, deps_model_name);
    aiger_to_blif(independent_strategy, indeps_blif, indeps_model_name);

    string deps_init_latch = find_init_latch(deps_blif, dependent_strategy->latch_var(0));
    deps_blif_latches_middleware(deps_blif, deps_init_latch);

    // Merge BLIF
    // TODO: extract this constant to a global variable
    std::string merged_blif_path = TMP_DIR + model_name + ".blif";
    ofstream merged_blif_file(merged_blif_path);
    merge_strategies_blifs(merged_blif_file, indeps_blif, deps_blif,
                           indeps_model_name, deps_model_name, inputs,
                           independent_vars, dependent_vars, model_name);
    merged_blif_file.close();

    spot::aig_ptr merged_strategy =
        blif_file_to_aiger(merged_blif_path, dict, model_name);

    remove(merged_blif_path.c_str());

    return merged_strategy;
}

