#include <iostream>
#include "utils.h"

#include "find_unates.h"
#include "synt_instance.h"

using namespace std;

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
    auto automaton = construct_automaton(synt_instance);


    // Init find unate code
    FindUnates find_unates(automaton);
    for(auto& candidate_variable : synt_instance.get_output_vars()) {
        for(unsigned state = 0; state < automaton->num_states(); state++) {
            bool is_unate = find_unates.is_unate_by_state(state, candidate_variable);
            if(is_unate) {
                cout << "[V] State " << state << " is Unate for variable " << candidate_variable << endl;
                cout << "    Removable edges: " << find_unates.removable_edges_by_state(state, candidate_variable) << endl;
            } else {
                cout << "[X] State " << state << " is NOT Unate for variable " << candidate_variable << endl;
            }
        }
    }

    return EXIT_SUCCESS;
}