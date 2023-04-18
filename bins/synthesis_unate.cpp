#include <signal.h>
#include <spot/twaalgos/aiger.hh>
#include <spot/twaalgos/aiger.hh>
#include <memory>
#include <stdexcept>
#include <iostream>
#include "utils.h"

#include "synt_measure.h"
#include "find_unates.h"
#include "synthesis_utils.h"
#include "unate_utils.h"

using namespace std;


static FindUnatesMeasures* unate_measures = nullptr;

void on_sighup(int args) {
    try {
        if(unate_measures != nullptr) {
            cout << *unate_measures << endl;
        } else {
            cerr << "No unate summary available" << endl;
        }
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
    spot::synthesis_info gi;
    gi.s = spot::synthesis_info::algo::SPLIT_DET;
    gi.minimize_lvl = 2;  // i.e, simplication level
    vector<string> input_vars(synt_instance.get_input_vars());

    verbose_out << "Synthesis Problem: " << endl;
    verbose_out << synt_instance << endl;
    verbose_out << "================================" << endl;

    signal(SIGINT, on_sighup);
    signal(SIGHUP, on_sighup);

    try {
        unate_measures = new FindUnatesMeasures( synt_instance );

        unate_measures->start_automaton_construct();
        spot::twa_graph_ptr automaton = get_nba_for_synthesis(
                synt_instance.get_formula_parsed(), gi, *unate_measures, verbose_out);
        unate_measures->end_automaton_construct(automaton);

        /**************************************************************************
         * Find Unate & Remove from Automaton
         **************************************************************************/
        unsigned init_state = automaton->get_init_state_number();

        // Init find unate code
        FindUnates find_unates(automaton, synt_instance, *unate_measures);
        for(unsigned state = 0; state < automaton->num_states(); state++) {
            find_unates.resolve_unates_in_state(state);
        }

        assert(init_state == automaton->get_init_state_number() && "Find Unate changed the automaton original state");

        /**************************************************************************
         * Synthesis the reduced Unate
         **************************************************************************/
        spot::aig_ptr strategy = synthesis_nba_to_aiger(gi, automaton, synt_instance.get_output_vars(), input_vars, verbose_out);
        if (strategy == nullptr) {
            cout << "UNREALIZABLE" << endl;
        }

        unate_measures->completed();
        cout << *unate_measures << endl;
        delete unate_measures;
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