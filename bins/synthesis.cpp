#include <signal.h>
#include <iostream>
#include <spot/twaalgos/aiger.hh>
#include <vector>
#include <memory>
#include <stdexcept>
#include <future>
#include <chrono>


#include "dependents_synthesiser.h"
#include "find_deps_by_automaton.h"
#include "merge_strategies.h"
#include "handle_unates_by_complement.h"
#include "handle_unates_by_heuristic.h"
#include "nba_utils.h"
#include "synthesis_utils.h"

using namespace std;
using namespace spot;

#define USE_UNATE_COMPLEMENT 0
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

    bool skip_dependencies = options.dependency_timeout <= 0;
    g_synt_measure =
        new SynthesisMeasure(synt_instance, skip_dependencies, options.skip_unates);
    SynthesisMeasure& synt_measure = *g_synt_measure;

    signal(SIGINT, on_sighup);
    signal(SIGTERM, on_sighup);
    signal(SIGHUP, on_sighup);

    try {
        // Get NBA for synthesis
        spot::twa_graph_ptr nba = get_nba_for_synthesis(
            synt_instance.get_formula_parsed(), gi, synt_measure, verbose);

        // Handle Unate
        if(options.skip_unates) {
            verbose << "=> Skipping finding and handling Unates" << endl;
        } else {
            verbose << "=> Finding and handling Unates" << endl;
            unsigned init_state = nba->get_init_state_number();

            // Init find unate code
#if USE_UNATE_COMPLEMENT
            HandleUnatesByComplement find_unates(nba, synt_instance, synt_measure);
#else
            HandleUnatesByHeuristic find_unates(nba, synt_instance, synt_measure);
#endif
            find_unates.run();

            assert(init_state == nba->get_init_state_number() && "Find Unate changed the automaton original state");
        }

        // Handle Dependent variables
        vector<string> dependent_variables, independent_variables;
        twa_graph_ptr nba_without_deps = nullptr, nba_with_deps = nullptr;

        if (skip_dependencies) {
            verbose << "=> Skipping finding and ejecting dependencies" << endl;
        } else {
            FindDepsByAutomaton automaton_dependencies(synt_instance, synt_measure,
                                                       nba, false);

            std::future<void> fut = std::async(std::launch::async, [&] {
                automaton_dependencies.find_dependencies(dependent_variables,
                                                         independent_variables);
            });
            if (fut.wait_for(std::chrono::milliseconds (options.dependency_timeout)) == std::future_status::timeout) {
                automaton_dependencies.stop();
            }
            while (!automaton_dependencies.is_done()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            }

            verbose << "Found " << dependent_variables.size()
                    << " dependent variables" << endl;
        }

        bool found_dependencies = !dependent_variables.empty();
        bool should_clone_nba_with_deps = found_dependencies;
        unordered_map<int, bdd> bdd_to_bdd_without_deps;

        if (should_clone_nba_with_deps) {
            synt_measure.start_clone_nba_with_deps();
            nba_with_deps = clone_nba(nba);
            synt_measure.end_clone_nba_with_deps();
        }

        if (found_dependencies) {
            synt_measure.start_remove_dependent_ap();
            remove_ap_from_automaton(nba, dependent_variables, bdd_to_bdd_without_deps);
            synt_measure.end_remove_dependent_ap();
        }
        nba_without_deps = nba;

        // Synthesis the independent variables
        vector<string>& indep_outs = found_dependencies
                                   ? synt_instance.get_output_vars()
                                   : independent_variables;

        synt_measure.start_independents_synthesis();
        spot::aig_ptr indep_strategy =
            synthesis_nba_to_aiger(gi, nba_without_deps, indep_outs, input_vars, verbose);
        synt_measure.end_independents_synthesis(indep_strategy);


        if (indep_strategy == nullptr) {
            cout << "UNREALIZABLE" << endl;
            synt_measure.completed();

            dump_measures(synt_measure, options);

            return EXIT_SUCCESS;
        }

        // Synthesis the dependent variables
        spot::aig_ptr final_strategy, dependents_strategy;

        if (found_dependencies) {
            synt_measure.start_dependents_synthesis();
            DependentsSynthesiser dependents_synt(nba_without_deps, nba_with_deps,
                                                  input_vars, independent_variables,
                                                  dependent_variables, bdd_to_bdd_without_deps);
            dependents_strategy = dependents_synt.synthesis();
            synt_measure.end_dependents_synthesis(dependents_strategy);

            synt_measure.start_merge_strategies();
            final_strategy = merge_strategies(
                indep_strategy, dependents_strategy, input_vars,
                independent_variables, dependent_variables, gi.dict, options.model_name);
            synt_measure.end_merge_strategies(final_strategy);
        } else {
            final_strategy = indep_strategy;

            if (skip_dependencies) {
                verbose << "=> Skipping synthesis dependent variables" << endl;
            } else {
                verbose << "=> No dependent variables found." << endl;
            }
        }

        spot::print_aiger(std::cout, final_strategy) << '\n';

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
        dump_measures(*g_synt_measure, options);
    } catch (const std::exception& ex) {
        std::cerr << "Error occurred: " << ex.what() << std::endl;
        dump_measures(*g_synt_measure, options);
    } catch (...) {
        std::cerr << "Unknown failure occurred. Possible memory corruption"
                  << std::endl;
        dump_measures(*g_synt_measure, options);
    }
}

void on_sighup(int args) {
    try {
        dump_measures(*g_synt_measure, options);
    } catch (const std::runtime_error& re) {
        std::cout << "Runtime error: " << re.what() << std::endl;
        dump_measures(*g_synt_measure, options);
    } catch (const std::exception& ex) {
        std::cout << "Error occurred: " << ex.what() << std::endl;
        dump_measures(*g_synt_measure, options);
    } catch (...) {
        std::cout << "Unknown failure occurred. Possible memory corruption"
                  << std::endl;
        dump_measures(*g_synt_measure, options);
    }

    exit(EXIT_SUCCESS);
}
