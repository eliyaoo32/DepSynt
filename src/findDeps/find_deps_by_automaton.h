#ifndef FIND_DEPS_BY_AUTOMATON_H
#define FIND_DEPS_BY_AUTOMATON_H

#include <map>
#include <spot/tl/parse.hh>
#include <spot/twaalgos/sccfilter.hh>
#include <spot/twaalgos/translate.hh>
#include <string>
#include <vector>
#include <atomic>

#include "bdd_utils.h"
#include "synt_instance.h"
#include "synt_measure.h"

using PairState = std::pair<unsigned, unsigned>;

using PairEdges =
    std::pair<spot::twa_graph::edge_storage_t, spot::twa_graph::edge_storage_t>;


bool are_edges_shares_variable(spot::twa_graph::edge_storage_t& e1,
                               spot::twa_graph::edge_storage_t& e2);
bool are_edges_shares_assignment(spot::twa_graph::edge_storage_t& e1,
                                 spot::twa_graph::edge_storage_t& e2);
bool are_states_collides_by_edges(spot::twa_graph_ptr& automaton, unsigned state1, unsigned state2, int dependent_var_num);

struct VarIndexer {
    int var_index{};
    int prime_var_index{};
};

class FindDepsByAutomaton {
   public:
    /**
     * @brief This class is used to determinate the type of dependent variable to
     * search.
     */
    enum DependentVariableType { Output, Input };

   private:
    SyntInstance& m_synt_instance;
    AutomatonFindDepsMeasure& m_measures;
    spot::twa_graph_ptr m_automaton;
    BDDVarsCacher* m_bdd_cacher;
    DependentVariableType m_dependent_variable_type;
    std::atomic<bool> m_stop_flag;
    std::atomic<bool> m_is_done;


    bool is_variable_dependent(std::string dependent_var,
                               std::vector<std::string>& dependency_vars,
                               std::vector<PairState>& pairStates,
                               bool use_shared_edges);

    bool is_dependent_by_pair_edges(int dependent_var,
                                    std::vector<int>& dependency_vars,
                                    vector<VarIndexer>& reset_vars,
                                    const PairEdges& edges);

    void init_automaton();

    void find_dependencies_candidates(std::vector<std::string>& candidates_dst);

    /**
     * @brief In a validation of a dependent variables, we need to calculate what's
     * the dependency set of the dependent variable. This function extracts the
     * dependency set.
     */
    void extract_dependency_set(std::vector<std::string>& dependency_set_dst,
                                std::vector<std::string>& current_candidates,
                                std::vector<std::string>& current_independents);

    /**
     * @brief Extract all the states in automaton which are reachable by the same prefix.
     * Return true if finished successfully and didn't stopped by the stop flag.
     */
    bool get_all_compatible_states(std::vector<PairState>& pairStates,
                                   const spot::twa_graph_ptr& aut);

   public:
    explicit FindDepsByAutomaton(SyntInstance& synt_instance,
                                 AutomatonFindDepsMeasure& measure,
                                 spot::twa_graph_ptr aut, bool should_prune)
        : m_synt_instance(synt_instance),
          m_measures(measure),
          m_stop_flag(false),
          m_is_done(false),
          m_dependent_variable_type(DependentVariableType::Output) {
        m_automaton = aut;

        // TODO: remove should prune and making sure the caller is pruning by himself
        if (should_prune) {
            m_measures.start_prune_automaton();
            m_automaton = spot::scc_filter_states(m_automaton);  // Prune m_automaton
            m_measures.end_prune_automaton(m_automaton);
        }

        m_bdd_cacher = new BDDVarsCacher(m_automaton);
    }

    ~FindDepsByAutomaton() {
        if(m_bdd_cacher != nullptr) {
            delete m_bdd_cacher;
        }
    }

    void set_dependent_variable_type(DependentVariableType type) {
        m_dependent_variable_type = type;
    }

    void find_dependencies(std::vector<std::string>& dependent_variables,
                           std::vector<std::string>& independent_variables,
                           bool use_shared_edges);

    void stop() {
        return m_stop_flag.store(true);
    }

    bool is_done() {
        return m_is_done.load();
    }
};

#endif
