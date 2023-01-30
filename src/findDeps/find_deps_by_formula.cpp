#include "find_deps_by_formula.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <spot/tl/formula.hh>
#include <spot/tl/parse.hh>
#include <spot/twaalgos/contains.hh>
#include <spot/twaalgos/translate.hh>

void FindDepsByFormula::find_dependencies(
    std::vector<std::string>& dependent_variables,
    std::vector<std::string>& independent_variables) {
    std::vector<std::string> candidates(m_synt_instance.get_output_vars());
    const std::vector<std::string>& input_vars = m_synt_instance.get_input_vars();

    while (!candidates.empty()) {
        std::string dependent_var = candidates.back();
        candidates.pop_back();

        m_measures.start_testing_variable(dependent_var);

        // Build dependency set: (Input + Candidates + Independents)
        vector<string> dependency_set = input_vars;
        std::copy(candidates.begin(), candidates.end(),
                  std::back_inserter(dependency_set));
        std::copy(independent_variables.begin(), independent_variables.end(),
                  std::back_inserter(dependency_set));

        // Check if candidates variable is dependent
        if (this->is_variable_dependent(dependent_var, dependency_set)) {
            dependent_variables.push_back(dependent_var);
            m_measures.end_testing_variable(true, dependency_set);
        } else {
            independent_variables.push_back(dependent_var);
            m_measures.end_testing_variable(false, dependency_set);
        }
    }
}

bool FindDepsByFormula::is_variable_dependent(string& dependent_var,
                                              vector<string>& dependency_vars) {
    spot::formula* dependency_formula =
        get_dependency_formula(dependent_var, dependency_vars);

    spot::translator trans;  // TODO: find best options for this translator
    spot::twa_graph_ptr automaton = trans.run(dependency_formula);
    bool is_empty = automaton->is_empty();

    delete dependency_formula;
    return is_empty;
}

spot::formula* FindDepsByFormula::get_dependency_formula(
    string& dependent_var, vector<string>& dependency_vars) {
    if (dependent_var.empty() || dependency_vars.empty()) {
        throw std::invalid_argument(
            "Dependent and dependency are required to have at least 1 item to "
            "check for dependency");
    }

    vector<string> dependent_vars = {dependent_var};

    spot::formula dependencies_equals_to_prime_formula,
        dependents_equals_to_prime_formula;
    equal_to_primes_formula(dependencies_equals_to_prime_formula, dependency_vars);
    equal_to_primes_formula(dependents_equals_to_prime_formula, dependent_vars);

    auto* dependency_formula = new spot::formula(spot::formula::And(
        {m_synt_instance.get_formula_parsed(),
         m_prime_synt_instance->get_formula_parsed(),
         spot::formula::M(spot::formula::Not(dependents_equals_to_prime_formula),
                          dependencies_equals_to_prime_formula)}));

    return dependency_formula;
}

void FindDepsByFormula::build_prime_synt_instance() {
    const vector<string>& input_vars = m_synt_instance.get_input_vars();
    const vector<string>& output_vars = m_synt_instance.get_output_vars();

    // Build Prime Synt Variables
    vector<string> prime_input_vars;
    vector<string> prime_output_vars;
    prime_input_vars.resize(input_vars.size());
    prime_output_vars.resize(m_synt_instance.get_output_vars().size());

    std::transform(input_vars.begin(), input_vars.end(), prime_input_vars.begin(),
                   get_prime_variable);
    std::transform(m_synt_instance.get_output_vars().begin(),
                   m_synt_instance.get_output_vars().end(), prime_output_vars.begin(),
                   get_prime_variable);

    // Build formula
    string prime_formula = std::string(m_synt_instance.get_formula_str());
    for (auto& var : input_vars) {
        boost::replace_all(prime_formula, var, get_prime_variable(var));
    }
    for (auto& var : output_vars) {
        boost::replace_all(prime_formula, var, get_prime_variable(var));
    }

    this->m_prime_synt_instance =
        new SyntInstance(prime_input_vars, prime_output_vars, prime_formula);
}

void equal_to_primes_formula(spot::formula& dst, vector<string>& vars) {
    vector<string> var_equal_to_prime;
    var_equal_to_prime.resize(vars.size());

    auto var_equal_to_prime_op = [](std::string& var) {
        return "(" + var + " <-> " + get_prime_variable(var) + ")";
    };
    std::transform(vars.begin(), vars.end(), var_equal_to_prime.begin(),
                   var_equal_to_prime_op);

    std::string equal_to_prime_str = boost::algorithm::join(var_equal_to_prime, " & ");
    spot::parsed_formula pf = spot::parse_infix_psl(equal_to_prime_str);

    if (pf.format_errors(std::cerr)) {
        throw std::runtime_error("Error parsing formula: " + equal_to_prime_str);
    }

    dst = pf.f;
}
