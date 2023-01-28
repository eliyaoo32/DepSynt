#include <iostream>
#include <spot/twaalgos/aiger.hh>
#include <spot/twaalgos/mealy_machine.hh>
#include <string>
#include <vector>

#include "dependents_synthesiser.h"
#include "find_deps_by_automaton.h"
#include "nba_utils.h"
#include "synt_instance.h"
#include "synthesis_utils.h"
#include "utils.h"

using namespace std;
using namespace spot;

#define AIGER_MODE "ite"

int main(int argc, const char* argv[]) {
    SynthesisCLIOptions options;
    int parsed_cli_status = parse_synthesis_cli(argc, argv, options);
    if (!parsed_cli_status) {
        return EXIT_FAILURE;
    }
    ostream nullout(nullptr);
    ostream& verbose = options.verbose ? std::cout : nullout;

    verbose << "=> Loaded Options: " << endl;
    verbose << options << endl;

    // Prepare synthesis info
    spot::synthesis_info gi;
    gi.s = spot::synthesis_info::algo::SPLIT_DET;
    gi.minimize_lvl = 2;  // i.e, simplication level
    SyntInstance synt_instance(options.inputs, options.outputs, options.formula);
    vector<string> input_vars(synt_instance.get_input_vars());
    SynthesisMeasure
        synt_measure;  // TODO: fix it and create custom automaton synt measures

    // Get NBA for synthesis
    spot::twa_graph_ptr nba = get_nba_for_synthesis(
        synt_instance.get_formula_parsed(), gi, synt_measure, verbose);

    // Handle Dependent variables
    vector<string> dependent_variables, independent_variables;
    twa_graph_ptr nba_without_deps = nullptr, nba_with_deps = nullptr;

    if (options.skip_eject_dependencies) {
        verbose << "=> Skipping finding and ejecting dependencies" << endl;
    } else {
        FindDepsByAutomaton automaton_dependencies(synt_instance, synt_measure, nba,
                                                   false);
        automaton_dependencies.find_dependencies(dependent_variables,
                                                 independent_variables);

        verbose << "Found " << dependent_variables.size() << " dependent variables"
                << endl;
    }

    bool found_depedencies =
        !options.skip_eject_dependencies && !dependent_variables.empty();
    bool should_clone_nba_with_deps = found_depedencies;
    bool should_clone_nba_without_deps = found_depedencies;

    if (should_clone_nba_with_deps) {
        nba_with_deps = clone_nba(nba);
    }

    if (found_depedencies) {
        synt_measure.start_remove_dependent_ap();
        remove_ap_from_automaton(nba, dependent_variables);
        synt_measure.end_remove_dependent_ap();
    }

    if (should_clone_nba_without_deps) {
        nba_without_deps = clone_nba(nba);
    }

    // Synthesis the independent variables
    spot::aig_ptr indep_strategy = synthesis_nba_to_aiger(
        gi, synt_measure, nba, independent_variables, input_vars, verbose);

    if (indep_strategy == nullptr) {
        cout << "UNREALIZABLE" << endl;
        // TODO: handle UNREALIZABLE
    }

    // Synthesis the dependent variables
    if (found_depedencies) {
        DependentsSynthesiser dependents_synt(nba_without_deps, nba_with_deps,
                                              input_vars, independent_variables,
                                              dependent_variables);
        spot::aig_ptr dependents_strategy = dependents_synt.synthesis();
        spot::print_aiger(std::cout, dependents_strategy) << '\n';
    } else if (options.skip_synt_dependencies) {
        verbose << "=> Skipping synthesis dependent variables" << endl;
    } else {
        cout << "No dependent variables found." << endl;
    }

    // TODO: merge the dependents strategy and independent strategy
    // TODO: add model checking
}