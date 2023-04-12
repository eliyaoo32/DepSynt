#include <signal.h>
#include <iostream>
#include "utils.h"

#include "synt_measure.h"
#include "find_unates.h"
#include "synt_instance.h"

using namespace std;

static FindUnatesMeasures* measures = nullptr;

void on_sighup(int args) {
    try {
        cout << *measures << endl;
    } catch (const std::runtime_error& re) {
        std::cerr << "Runtime error: " << re.what() << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Error occurred: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown failure occurred. Possible memory corruption"
                  << std::endl;
    }

    exit(EXIT_SUCCESS);
}

int main(int argc, const char* argv[]) {
    FindUnatesCLIOptions options;

    int parsed_cli_status = parse_find_unates_cli(argc, argv, options);
    if (!parsed_cli_status) {
        return EXIT_FAILURE;
    }

    std::ostream nullout(nullptr);
    ostream& verbose_out = options.verbose ? std::cout : nullout;

    // Init LTL formula
    verbose_out << "Initialize Synthesis Instance..." << endl;
    SyntInstance synt_instance(options.inputs, options.outputs, options.formula);
    verbose_out << "Synthesis Problem: " << endl;
    verbose_out << synt_instance << endl;
    verbose_out << "================================" << endl;

    signal(SIGINT, on_sighup);
    signal(SIGHUP, on_sighup);

    try {
        measures = new FindUnatesMeasures(synt_instance);
        measures->start_automaton_construct();
        auto automaton = construct_automaton(synt_instance);
        measures->end_automaton_construct(automaton);

        unsigned init_state = automaton->get_init_state_number();

        // Init find unate code
        FindUnates find_unates(automaton);
        for(auto& candidate_variable : synt_instance.get_output_vars()) {
            for(unsigned state = 0; state < automaton->num_states(); state++) {
                measures->start_testing_variable(candidate_variable, state);

                bool is_unate = find_unates.is_unate_by_state(state, candidate_variable);
                auto removable_edges = is_unate ? find_unates.removable_edges_by_state(state, candidate_variable) : -1;

                measures->end_testing_variable(is_unate, removable_edges);
            }
        }

        assert(init_state == automaton->get_init_state_number() && "Find Unate changed the automaton original state");

        measures->completed();
        cout << *measures << endl;
        delete measures;
    } catch (const std::runtime_error& re) {
        std::cerr << "Runtime error: " << re.what() << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Error occurred: " << ex.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown failure occurred. Possible memory corruption"
                  << std::endl;
    }

    return EXIT_SUCCESS;
}