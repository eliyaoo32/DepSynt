#include <cstdio>
#include <sstream>
#include <regex>
#include <boost/algorithm/string/join.hpp>
#include <fstream>

#include "aigtoblif.h"
#include "aigtoaig.h"
#include "abc_utils.h"
#include "BLIF.h"


void BLIF::load_aig(spot::aig_ptr& aig) {
    // AIG to String
    std::stringstream aiger_stream;
    spot::print_aiger(aiger_stream, aig) << endl;
    string aiger_str = aiger_stream.str();

    // Create In-Memory file of the AIG
    char* aiger_buff = (char*)aiger_str.c_str();
    FILE* aiger_file = fmemopen(aiger_buff, aiger_str.size(), "r+");

    // Create In-Memory file to BLIF
    char* blif_buff;
    size_t blif_buff_size;
    FILE* blif_file = open_memstream (&blif_buff, &blif_buff_size);

    // Convert AIG to BLIF
    int err = aigtoblif(aiger_file, blif_file, m_model_name.c_str());
    if(err != 0) {
        cerr << "Error converting aiger to blif" << endl;
        exit(1);
    }

    m_blif_content = new std::string(blif_buff, blif_buff_size);

    fclose(aiger_file);
    fclose(blif_file);
    free(blif_buff);
}

void BLIF::init_latch_to_one(string& latch_name) {
    string& blif = *m_blif_content;

    // Remove .end
    std::size_t end_ind = blif.find(".end");
    if (end_ind != std::string::npos) {
        blif.erase(end_ind, 4);
    }

    // Add const 1 if it is not in the blif
    if (blif.find(".names c1") == string::npos) {
        blif += ".names c1\r\n1\r\n\r\n";
    }

    // Define a new latch, init value is 0 and next value is always 1
    string tmp_latch = "lltmpinit" + latch_name;
    size_t tmp_latch_idx = blif.find(".latch");
    assert(tmp_latch_idx != std::string::npos &&
           "Error: .latch not found in the BLIF file.");
    blif.insert(tmp_latch_idx,
                              ".latch c1 " + tmp_latch + " 0\r\n\r\n");

    // Replace the latch with a temp init latch
    string init_latch_tmp = latch_name + "Tmp";
    std::regex init_latch_pattern(".latch (.*) (" + latch_name + ") (.*)");
    assert(std::regex_search(blif, init_latch_pattern) &&
           "Init latch not found in the blif file.");
    blif = std::regex_replace(blif, init_latch_pattern,
                                            ".latch $1 " + init_latch_tmp + " $3");

    // Add a new name, where it takes the new latch and the temp init latch and
    // outputs the original init latch
    blif += ".names " + init_latch_tmp + " " + tmp_latch + " " +
            latch_name +
                          "\r\n"
                          "-0 1\r\n"
                          "11 1\r\n\r\n";

    // Add .end
    blif += ".end\r\n\r\n";
}

string BLIF::find_latch_name_by_num(unsigned int latch_num) {
    string& blif = *m_blif_content;
    std::regex init_latch_pattern(".latch .* ([a-zA-Z]+" +
                                  std::to_string(latch_num) + ") .*");
    std::smatch match;

    if (std::regex_search(blif, match, init_latch_pattern)) {
        return match[1];
    } else {
        cerr << "Couldn't find init latch in the blif file." << endl;
        return "";
    }
}

BLIF_ptr BLIF::merge_dependency_strategies(BLIF& indep_blif, BLIF& dep_blif,
                                     const vector<string>& inputs,
                                     const vector<string>& independent_vars,
                                     const vector<string>& dependent_vars,
                                     string& model_name) {
    auto merged_blif = std::make_shared<BLIF>(model_name);
    std::stringstream out;

    string blif_inputs = boost::algorithm::join(inputs, " ");
    string blif_independent_vars = boost::algorithm::join(independent_vars, " ");
    string blif_dependent_vars = boost::algorithm::join(dependent_vars, " ");

    // Base of the blif
    out << ".model " << model_name << endl;
    out << ".inputs " << blif_inputs << endl;
    out << ".outputs " << blif_independent_vars << " " << blif_dependent_vars
        << endl;

    // Declare Wired Variables
    for (const auto & independent_var : independent_vars) {
        out << ".names " << blif_wired_var(independent_var) << " "
            << independent_var << endl;
        out << "1 1" << endl;
    }
    for (const auto & dependent_var : dependent_vars) {
        out << ".names " << blif_wired_var(dependent_var) << " "
            << dependent_var << endl;
        out << "1 1" << endl;
    }

    // Create Circuits Connectors
    string input_wires = "";
    string indeps_wires = "";
    string deps_wires = "";

    for (const auto & input : inputs) {
        input_wires += input + "=" + input + " ";
    }
    for (const auto & independent_var : independent_vars) {
        indeps_wires +=
                independent_var + "=" + blif_wired_var(independent_var) + " ";
    }
    for (const auto & dependent_var : dependent_vars) {
        deps_wires +=
                dependent_var + "=" + blif_wired_var(dependent_var) + " ";
    }

    // Create subsckts
    out << ".subckt " << indep_blif.m_model_name << " " << input_wires << " "
        << indeps_wires << endl;
    out << ".subckt " << dep_blif.m_model_name << " " << input_wires << " " << indeps_wires
        << " " << deps_wires << endl;
    out << ".end" << endl;

    // Attach subckts to the base
    out << indep_blif << endl;
    out << dep_blif << endl;

    string merged_blif_content = out.str();
    merged_blif->load_string(merged_blif_content);

    return merged_blif;
}

ostream& operator<<(ostream& os, const BLIF& sm) {
    os << *(sm.m_blif_content);
    return os;
}

spot::aig_ptr BLIF::to_aig(spot::bdd_dict_ptr& dict) {
    // Write the blif to a file
    string blif_file_path = "./tmp_blif.blif";
    ofstream blif_file(blif_file_path);
    blif_file << *this;
    blif_file.close();

    // Convert BLIF to Binary AIGER
    string binary_aig_file_name = "./tmp_aig.aig";
    blif_file_to_binary_aig_file(blif_file_path, binary_aig_file_name);

    // Binary Aiger To ASCII Aiger
    FILE* binary_aig_file = fopen(binary_aig_file_name.c_str(), "rb");

    char* ascii_aig_buff;
    size_t ascii_aig_size;
    FILE* ascii_aig_file = open_memstream (&ascii_aig_buff, &ascii_aig_size);

    aigtoaig(binary_aig_file, ascii_aig_file, 1);

    fclose(binary_aig_file);
    fclose(ascii_aig_file);

    // ASCII Aiger to Spot AIG
    spot::aig_ptr aig = spot::aig::parse_aag(ascii_aig_buff, "tmp_aig", dict);

    free(ascii_aig_buff);
    return aig;
}
