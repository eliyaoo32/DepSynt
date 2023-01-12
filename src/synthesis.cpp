#include <iostream>
#include <spot/twaalgos/aiger.hh>
#include <spot/twaalgos/mealy_machine.hh>
#include <string>
#include <vector>

#include "dependents_synthesiser.h"
#include "find_deps_by_automaton.h"
#include "synt_instance.h"
#include "synthesis_utils.h"
#include "utils.h"

using namespace std;
using namespace spot;

#define AIGER_MODE "ite"

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

    if (options.decompose_formula) {
        cerr << "Synthesis Dependents Vars doesn't support decomposing formulas"
             << endl;
        return EXIT_FAILURE;
    }

    spot::synthesis_info gi;
    gi.s = spot::synthesis_info::algo::SPLIT_DET;
    gi.minimize_lvl = 2;  // i.e, simplication level

    SyntInstance synt_instance(options.inputs, options.outputs, options.formula);
    AutomatonSyntMeasure synt_measure(synt_instance, options.skip_dependencies);

    /**
     * Synthesising
     */
    // Step 1: Convert synthesis formula to NBA
    spot::twa_graph_ptr nba =
        get_nba_for_synthesis(synt_instance, gi, synt_measure, verbose);

    // Step 2: Find & Remove dependent variables
    vector<string> dependent_variables, independent_variables;
    bool found_depedencies = false;
    if (options.skip_dependencies) {
        verbose << "=> Skip finding dependent variables..." << endl;
    } else {
        find_and_remove_dependents(nba, synt_instance, synt_measure,
                                   dependent_variables, independent_variables,
                                   verbose);
        found_depedencies = dependent_variables.size() > 0;
    }

    // Step 2.1: Store NBA without dependencies, will be used to synthesis
    // dependecies strategy
    twa_graph_ptr nba_without_deps;
    if (found_depedencies) {
        const_twa_graph_ptr nba_to_clone = nba;
        twa_graph* nba_without_deps_ =
            new twa_graph(*nba_to_clone);  // TODO: make sure it clones correctly}
        nba_without_deps = shared_ptr<twa_graph>(nba_without_deps_);
    }

    // Step 3: Synthesis the NBA
    mealy_like mealy;
    bool should_split = true;  // Because it's an AIGER
    bool is_realizable = synthesis_nba_to_mealy(gi, synt_measure, nba, synt_instance,
                                                verbose, should_split, mealy);

    if (!is_realizable) {
        cout << "UNREALIZABLE" << endl;
        return EXIT_SUCCESS;
    }

    // Step 4: Convert the Mealy machine to AIGER
    spot::aig_ptr independent_aig =
        mealy_machines_to_aig({mealy}, AIGER_MODE, synt_instance.get_input_vars(),
                              {independent_variables});

    // Output results
    cout << "Synthesis Measures: " << endl;
    cout << synt_measure << endl;

    cout << "=========================" << endl;

    cout << "Independents Aiger: " << endl;
    spot::print_aiger(std::cout, independent_aig) << '\n';

    // Step 5: Synthesis Dependent vars
    if (found_depedencies) {
        vector<string> input_vars(synt_instance.get_input_vars());
        DependentsSynthesiser dependents_synt(nba_without_deps, input_vars,
                                              independent_variables,
                                              dependent_variables);
        spot::aig_ptr dependents_strategy = dependents_synt.synthesis();

        cout << "Dependent Variables Strategy: " << endl;
        spot::print_aiger(std::cout, dependents_strategy) << '\n';
    }
}
