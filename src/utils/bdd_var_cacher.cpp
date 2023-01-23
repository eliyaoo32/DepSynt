#include "bdd_var_cacher.h"

#include <spot/tl/parse.hh>
#include <spot/twaalgos/translate.hh>

BDDVarsCacher::BDDVarsCacher(spot::twa_graph_ptr automaton)
    : m_automaton(automaton), m_prime_var_coefficient(0) {
    m_total_vars = automaton->ap().size();

    // Register prime variables
    m_prime_var_start =
        automaton->get_dict()->register_anonymous_variables(m_total_vars, automaton);
}

int BDDVarsCacher::get_variable_index(std::string& variable_name) {
    if (m_variable_index.find(variable_name) == m_variable_index.end()) {
        m_variable_index[variable_name] = m_automaton->register_ap(variable_name);
    }

    return m_variable_index[variable_name];
}

int BDDVarsCacher::get_prime_variable_index(std::string& variable_name) {
    if (m_prime_variable_index.find(variable_name) == m_prime_variable_index.end()) {
        m_prime_variable_index[variable_name] =
            m_prime_var_start + m_prime_var_coefficient;

        m_prime_var_coefficient++;
    }

    return m_prime_variable_index[variable_name];
}
