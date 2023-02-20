#include <signal.h>

#include <iostream>
#include <spot/twaalgos/aiger.hh>
#include <spot/twaalgos/mealy_machine.hh>
#include <string>
#include <vector>
#include <cstdio>
#include <memory>
#include <stdexcept>

#include "dependents_synthesiser.h"
#include "find_deps_by_automaton.h"
#include "merge_strategies.h"
#include "nba_utils.h"
#include "synt_instance.h"
#include "synthesis_utils.h"
#include "utils.h"

using namespace std;
using namespace spot;

#define AIGER_MODE "ite"

static SynthesisMeasure* g_synt_measure = nullptr;
static SynthesisCLIOptions options;

void on_sighup(int args);

int main(int argc, const char* argv[]) {
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

    g_synt_measure =
        new SynthesisMeasure(synt_instance, options.skip_eject_dependencies);
    SynthesisMeasure& synt_measure = *g_synt_measure;

//    string dst2;
//    exec2("/Users/eliyahub/Thesis/reactive-synthesis-bfss/tools/aiger-utilities/aigtoblif /tmp/ltl2dpa16_deps.aag", dst2);
//    cout << dst2 << endl;
//
//
//    cout << "Results: " << endl << dst2 << endl;
//    return 0;
//
//    string dst;
//    char buffer[128];
//    auto pipe = popen("/Users/eliyahub/Thesis/reactive-synthesis-bfss/tools/aiger-utilities/aigtoblif /tmp/ltl2dpa16_deps.aag", "r");
//    if (!pipe) throw runtime_error("popen() failed!");
//
//    while (!feof(pipe)) {
//        if (fgets(buffer, 128, pipe) != NULL) {
//            dst += buffer;
//        }
//    }
//
//    pclose(pipe);
//
//    cout << "Results: " << endl << dst << endl;
//
//    return 0;

    signal(SIGINT, on_sighup);
    signal(SIGHUP, on_sighup);

    try {
        // Get NBA for synthesis
        spot::twa_graph_ptr nba = get_nba_for_synthesis(
            synt_instance.get_formula_parsed(), gi, synt_measure, verbose);

        // Handle Dependent variables
        vector<string> dependent_variables, independent_variables;
        twa_graph_ptr nba_without_deps = nullptr, nba_with_deps = nullptr;

        if (options.skip_eject_dependencies) {
            verbose << "=> Skipping finding and ejecting dependencies" << endl;
        } else {
            FindDepsByAutomaton automaton_dependencies(synt_instance, synt_measure,
                                                       nba, false);
            automaton_dependencies.find_dependencies(dependent_variables,
                                                     independent_variables);

            verbose << "Found " << dependent_variables.size()
                    << " dependent variables" << endl;
        }

        bool found_depedencies =
            !options.skip_eject_dependencies && !dependent_variables.empty();
        bool should_clone_nba_with_deps = found_depedencies;
        bool should_clone_nba_without_deps = found_depedencies;

        if (should_clone_nba_with_deps) {
            synt_measure.start_clone_nba_with_deps();
            nba_with_deps = clone_nba(nba);
            synt_measure.end_clone_nba_with_deps();
        }

        if (found_depedencies) {
            synt_measure.start_remove_dependent_ap();
            remove_ap_from_automaton(nba, dependent_variables);
            synt_measure.end_remove_dependent_ap();
        }

        if (should_clone_nba_without_deps) {
            synt_measure.start_clone_nba_without_deps();
            nba_without_deps = clone_nba(nba);
            synt_measure.end_clone_nba_without_deps();
        }

        // Synthesis the independent variables
        vector<string>& outs = options.skip_synt_dependencies
                                   ? synt_instance.get_output_vars()
                                   : independent_variables;

        synt_measure.start_independents_synthesis();
        spot::aig_ptr indep_strategy =
            synthesis_nba_to_aiger(gi, nba, outs, input_vars, verbose);
        synt_measure.end_independents_synthesis(indep_strategy);

        cout << "Indepedent strategy: " << endl;
        spot::print_aiger(std::cout, indep_strategy) << '\n';

        if (indep_strategy == nullptr) {
            cout << "UNREALIZABLE" << endl;
            synt_measure.completed();

            dump_measures(synt_measure, options);

            return EXIT_SUCCESS;
        }

        // Synthesis the dependent variables
        spot::aig_ptr final_strategy, dependents_strategy;

        if (found_depedencies && !options.skip_synt_dependencies) {
            synt_measure.start_dependents_synthesis();
            DependentsSynthesiser dependents_synt(nba_without_deps, nba_with_deps,
                                                  input_vars, independent_variables,
                                                  dependent_variables);
            dependents_strategy = dependents_synt.synthesis();
            synt_measure.end_dependents_synthesis(dependents_strategy);

            // TODO: print only final strategy
            cout << "Dependents strategy: " << endl;
            spot::print_aiger(std::cout, dependents_strategy) << '\n';

            string model_name = "ltl2dpa16";  // TODO: fetch from options

            // TODO: calculate duration of merge_strategies
            final_strategy = merge_strategies(
                indep_strategy, dependents_strategy, input_vars,
                independent_variables, dependent_variables, gi.dict, model_name);
        } else {
            final_strategy = indep_strategy;

            if (options.skip_synt_dependencies) {
                verbose << "=> Skipping synthesis dependent variables" << endl;
            } else {
                cout << "No dependent variables found." << endl;
            }
        }

        if (options.apply_model_checking) {
            synt_measure.start_model_checking();

            spot::translator trans(gi.dict, &gi.opt);
            auto neg_spec =
                trans.run(spot::formula::Not(synt_instance.get_formula_parsed()));

            auto strategy_aut = final_strategy->as_automaton(false);
            bool model_checking_ok = !neg_spec->intersects(strategy_aut);

            if (model_checking_ok) {
                verbose << "=> Model checking: OK" << endl;
            } else {
                cerr << "=> Model checking: Error - Strategy intersects with "
                        "negation of specificaiton"
                     << endl;
            }

            synt_measure.end_model_checking(model_checking_ok ? "OK" : "Error");
        }

        // Print Measures
        synt_measure.completed();
        dump_measures(synt_measure, options);

        return EXIT_SUCCESS;
    } catch (const std::runtime_error& re) {
        std::cerr << "Runtime error: " << re.what() << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Error occurred: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown failure occurred. Possible memory corruption"
                  << std::endl;
    }
}

void on_sighup(int args) {
    try {
        dump_measures(*g_synt_measure, options);
    } catch (const std::runtime_error& re) {
        std::cout << "Runtime error: " << re.what() << std::endl;
    } catch (const std::exception& ex) {
        std::cout << "Error occurred: " << ex.what() << std::endl;
    } catch (...) {
        std::cout << "Unknown failure occurred. Possible memory corruption"
                  << std::endl;
    }

    exit(EXIT_SUCCESS);
}
