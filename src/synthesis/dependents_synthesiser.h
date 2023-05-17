#ifndef DEPENDENTS_SYNTHESISER_H
#define DEPENDENTS_SYNTHESISER_H

#include <algorithm>
#include <iostream>
#include <spot/tl/parse.hh>
#include <spot/twa/fwd.hh>
#include <spot/twa/twa.hh>
#include <spot/twa/twagraph.hh>
#include <spot/twaalgos/aiger.hh>
#include <string>
#include <utility>
#include <vector>

using State = unsigned;
using Gate = unsigned;
using BDDVar = int;
using StateGatePair = std::pair<State, Gate>;

using namespace spot;
using namespace std;

class DependentsSynthesiser {
   private:
    spot::twa_graph_ptr m_nba_without_deps;
    spot::twa_graph_ptr m_nba_with_deps;
    std::vector<std::string> m_input_vars;
    std::vector<std::string> m_indep_vars;
    std::vector<std::string> m_dep_vars;

    aig_ptr m_aiger;

    unordered_set<BDDVar> deps_bdd_vars;
    unordered_map<string, Gate> partial_impl_cache;

    void init_aiger();

    void define_next_latches();

    void define_output_gates();

    Gate get_partial_impl(const bdd& cond, string& dep_var);

    Gate generate_partial_impl(const bdd& cond, string& dep_var,
                               unordered_map<int, Gate>& bdd_partial_impl);

    BDDVar ap_to_bdd_varnum(string& ap) {
        return m_nba_with_deps->register_ap(ap);
    }

   public:
    DependentsSynthesiser(spot::twa_graph_ptr& nba_without_deps,
                          spot::twa_graph_ptr& nba_with_deps,
                          std::vector<std::string>& input_vars,
                          std::vector<std::string>& indep_vars,
                          std::vector<std::string>& dep_vars)
        : m_nba_without_deps(nba_without_deps),
          m_nba_with_deps(nba_with_deps),
          m_input_vars(input_vars),
          m_indep_vars(indep_vars),
          m_dep_vars(dep_vars){};

    spot::aig_ptr synthesis() {
        assert(m_nba_with_deps->num_states() == m_nba_without_deps->num_states());
        assert(m_nba_with_deps->get_dict() == m_nba_without_deps->get_dict());
        assert(!m_dep_vars.empty() && "Dep Vars must be non-empty");

        init_aiger();
        define_next_latches();
        define_output_gates();

        return m_aiger;
    }
};

#endif