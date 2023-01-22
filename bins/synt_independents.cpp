#include <iostream>
#include <set>
#include <spot/twaalgos/aiger.hh>
#include <spot/twaalgos/mealy_machine.hh>
#include <string>
#include <vector>

#include "find_deps_by_automaton.h"
#include "synt_instance.h"
#include "synthesis_utils.h"
#include "utils.h"

#define AIGER_MODE "ite"

/**
 * This CLI tool synthesizes a AIGER machine from a given LTL formula.
 * The tool supports decomposing the formula into sub-formulas.
 * The tool also supports finding dependent variables and removing them.
 * The tool DOES NOT synthesising the dependent variables.
 */
int main(int argc, const char* argv[]) {
    /**
     * Process the LTL specfication
     */
    SynthesisCLIOptions options;
    int parsed_cli_status = parse_synthesis_cli(argc, argv, options);
    if (!parsed_cli_status) {
        return EXIT_FAILURE;
    }
    ostream nullout(nullptr);
    ostream& verbose = options.verbose ? std::cout : nullout;

    verbose << "=> Loaded Options: " << endl;
    verbose << options << endl;

    vector<string> synt_output_vars = {};
    vector<string> synt_input_vars = {};
    extract_variables(options.outputs, synt_output_vars);
    extract_variables(options.inputs, synt_input_vars);

    spot::synthesis_info gi;
    gi.s = spot::synthesis_info::algo::SPLIT_DET;
    gi.minimize_lvl = 2;  // i.e, simplication level

    /**
     * Decompose the formula into sub-formulas
     */
    vector<SyntInstance> synt_instances;

    if (options.decompose_formula) {
        auto splitted_formulas =
            spot::split_independant_formulas(options.formula, synt_output_vars);

        assert(splitted_formulas.first.size() == splitted_formulas.second.size() &&
               "Invalid decomposed formula");

        if (splitted_formulas.first.size() > 1) {
            for (size_t i = 0; i < splitted_formulas.first.size(); i++) {
                auto& sub_formula = splitted_formulas.first[i];
                auto& sub_out_aps = splitted_formulas.second[i];

                vector<string> sub_out_vars;
                std::transform(sub_out_aps.begin(), sub_out_aps.end(),
                               std::back_inserter(sub_out_vars),
                               [](auto& ap) { return ap.ap_name(); });

                synt_instances.emplace_back(synt_input_vars, sub_out_vars,
                                            sub_formula);
            }
        }
    }
    if (synt_instances.empty()) {
        synt_instances.emplace_back(synt_input_vars, synt_output_vars,
                                    options.formula);
    }
    if (options.verbose) {
        verbose << "=> Found " << synt_instances.size()
                << " sub-formulas. (Apply Decompose = " << options.decompose_formula
                << ")" << endl;
    }

    /**
     * Synthesis each sub-formula
     */
    vector<shared_ptr<AutomatonSyntMeasure>> synt_measures;
    vector<spot::mealy_like> mealy_machines(synt_instances.size());

    for (size_t i = 0; i < synt_instances.size(); i++) {
        auto& synt_instance = synt_instances[i];
        auto& mealy = mealy_machines[i];

        synt_instance.order_output_vars(synt_output_vars);

        AutomatonSyntMeasure* synt_meas =
            new AutomatonSyntMeasure(synt_instance, options.skip_dependencies);

        // Step 1: Convert synthesis formula to NBA
        spot::twa_graph_ptr nba =
            get_nba_for_synthesis(synt_instance, gi, *synt_meas, verbose);

        // Step 2: Find & Remove dependent variables
        vector<string> dependent_variables, independent_variables;
        if (options.skip_dependencies) {
            verbose << "=> Skip finding dependent variables..." << endl;
        } else {
            find_and_remove_dependents(nba, synt_instance, *synt_meas,
                                       dependent_variables, independent_variables,
                                       verbose);
        }

        // Step 3: Synthesis the NBA
        bool should_split = true;  // Because it's an AIGER
        bool is_realizable = synthesis_nba_to_mealy(
            gi, *synt_meas, nba, synt_instance, verbose, should_split, mealy);

        if (!is_realizable) {
            cout << "UNREALIZABLE" << endl;
            return EXIT_SUCCESS;
        }
    }

    /**
     * Generates AIGER format
     */
    vector<vector<string>> sub_output_vars;
    sub_output_vars.resize(synt_instances.size());
    std::transform(
        synt_instances.begin(), synt_instances.end(), sub_output_vars.begin(),
        [](auto& synt_instance) { return synt_instance.get_output_vars(); });

    spot::aig_ptr saig = spot::mealy_machines_to_aig(
        mealy_machines, AIGER_MODE, synt_input_vars, sub_output_vars);
    spot::print_aiger(std::cout, saig) << '\n';
}
