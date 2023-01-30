#ifndef BDD_VARS_CACHER_H
#define BDD_VARS_CACHER_H

#include <map>
#include <spot/tl/parse.hh>
#include <spot/twaalgos/translate.hh>
#include <string>
#include <vector>

class BDDVarsCacher {
   private:
    int m_total_vars;
    int m_prime_var_start;  // At what index of variable, the prime var starts
    int m_prime_var_coefficient;
    std::unordered_map<std::string, int> m_variable_index;
    std::unordered_map<std::string, int> m_prime_variable_index;
    spot::twa_graph_ptr m_automaton;

   public:
    BDDVarsCacher(spot::twa_graph_ptr automaton);

    int get_variable_index(std::string& variable_name);

    int get_prime_variable_index(std::string& variable_name);
};

#endif