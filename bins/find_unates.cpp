#include <signal.h>
#include <iostream>
#include "utils.h"

#include "synt_measure.h"
#include "handle_unates.h"
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
    verbose_out << "Synthesis Problem: " << endl;
    verbose_out << synt_instance << endl;
    verbose_out << "================================" << endl;

    signal(SIGINT, on_sighup);
    signal(SIGHUP, on_sighup);

    try {
        unate_measures = new FindUnatesMeasures( synt_instance );

        unate_measures->start_automaton_construct();
        auto automaton = construct_automaton(synt_instance);
        unate_measures->end_automaton_construct(automaton);

        unsigned init_state = automaton->get_init_state_number();

        // Init find unate code
        HandleUnates find_unates(automaton, synt_instance, *unate_measures);
        find_unates.run();

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