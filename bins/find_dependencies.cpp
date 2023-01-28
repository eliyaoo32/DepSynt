#include <signal.h>

#include <iostream>
#include <spot/twaalgos/sccfilter.hh>
#include <vector>

#include "find_deps_by_automaton.h"
#include "find_deps_by_formula.h"
#include "synt_instance.h"
#include "synt_measure.h"
#include "utils.h"

namespace Options = boost::program_options;
using namespace std;

static BaseMeasures* synt_measures = nullptr;

// TODO: throw cerr and exit instead of cout
void on_sighup(int args) {
    try {
        cout << *synt_measures << endl;
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

int main(int argc, const char* argv[]) {
    FindDependenciesCLIOptions options;

    int parsed_cli_status = parse_find_dependencies_cli(argc, argv, options);
    if (!parsed_cli_status) {
        return EXIT_FAILURE;
    }

    std::ostream nullout(nullptr);
    ostream& verbose_out = options.verbose ? std::cout : nullout;

    // Build Synthesis synt_instance
    verbose_out << "Initialize Synthesis Instance..." << endl;
    SyntInstance synt_instance(options.inputs, options.outputs, options.formula);
    verbose_out << "Synthesis Problem: " << endl;
    verbose_out << synt_instance << endl;
    verbose_out << "================================" << endl;

    signal(SIGINT, on_sighup);
    signal(SIGHUP, on_sighup);

    try {
        if (options.algorithm == Algorithm::FORMULA) {
            auto* formula_measures = new BaseMeasures(synt_instance);
            synt_measures = formula_measures;

            verbose_out << "Building Synthesis Automaton..." << endl;
            formula_measures->start_automaton_construct();
            auto automaton = construct_automaton(synt_instance);
            string state_based_status =
                automaton->prop_state_acc().is_true()
                    ? "true"
                    : (automaton->prop_state_acc().is_false() ? "false" : "maybe");
            formula_measures->end_automaton_construct(automaton);

            verbose_out << "Searching Dependencies By Formula Definition..." << endl;

            vector<string> formula_dependent_variables,
                formula_independent_variables;
            FindDepsByFormula formula_dependencies(synt_instance, *formula_measures);
            formula_dependencies.find_dependencies(formula_dependent_variables,
                                                   formula_independent_variables);

            verbose_out << "Formula Dependent Variables: "
                        << formula_dependent_variables << endl;
            verbose_out << "Formula Dependency Variables: "
                        << formula_independent_variables << endl;
        } else if (options.algorithm == Algorithm::AUTOMATON) {
            auto* automaton_measures =
                new AutomatonFindDepsMeasure(synt_instance, false);
            synt_measures = automaton_measures;

            verbose_out << "Searching Dependencies By Automaton Definition..."
                        << endl;

            // Building Instance Automaton
            automaton_measures->start_automaton_construct();
            auto automaton = construct_automaton(synt_instance);
            automaton_measures->end_automaton_construct(automaton);

            automaton_measures->start_prune_automaton();
            automaton = spot::scc_filter_states(automaton);  // Prune automaton
            automaton_measures->end_prune_automaton(automaton);

            // Search for depedent variables
            vector<string> automaton_dependent_variables,
                automaton_independent_variables;
            FindDepsByAutomaton automaton_dependencies(
                synt_instance, *automaton_measures, automaton, false);
            if (options.find_input_dependencies) {
                verbose_out << "Searching for input dependent variables..." << endl;
                automaton_dependencies.set_dependent_variable_type(
                    FindDepsByAutomaton::DependentVariableType::Input);
            } else {
                verbose_out << "Searching for output dependent variables..." << endl;
                automaton_dependencies.set_dependent_variable_type(
                    FindDepsByAutomaton::DependentVariableType::Output);
            }
            automaton_dependencies.find_dependencies(
                automaton_dependent_variables, automaton_independent_variables);

            verbose_out << "Automaton Dependent Variables: "
                        << automaton_dependent_variables << endl;
            verbose_out << "Automaton Dependency Variables: "
                        << automaton_independent_variables << endl;
        } else {
            std::cerr << "No algorithm was selected " << std::endl;
            return EXIT_FAILURE;
        }

        synt_measures->completed();
        cout << *synt_measures << endl;
        delete synt_measures;
    } catch (const std::runtime_error& re) {
        std::cout << "Runtime error: " << re.what() << std::endl;
    } catch (const std::exception& ex) {
        std::cout << "Error occurred: " << ex.what() << std::endl;
    } catch (...) {
        std::cout << "Unknown failure occurred. Possible memory corruption"
                  << std::endl;
    }

    return EXIT_SUCCESS;
}
