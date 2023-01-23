#ifndef FORMULA_DEPENDENCIES_H
#define FORMULA_DEPENDENCIES_H

#include <spot/twa/twa.hh>
#include <string>
#include <vector>

#include "synt_instance.h"
#include "synt_measure.h"

using namespace std;

// We make a prime variable by adding the suffix "_pp" to variable's name
inline std::string get_prime_variable(const std::string &var) { return var + "_pp"; }

void equal_to_primes_formula(spot::formula &formula, vector<string> &vars);

class FindDepsByFormula {
   private:
    SyntInstance &m_synt_instance;
    SyntInstance *m_prime_synt_instance;
    SyntMeasures &m_measures;

    bool is_variable_dependent(string &dependent_var, vector<string> &dependency_vars);

    spot::formula *get_dependency_formula(string &dependent_var,
                                          vector<string> &dependency_vars);

    void build_prime_synt_instance();

   public:
    explicit FindDepsByFormula(SyntInstance &synt_instance, SyntMeasures &measure)
        : m_synt_instance(synt_instance), m_measures(measure) {
        build_prime_synt_instance();
    }

    ~FindDepsByFormula() { delete m_prime_synt_instance; }

    void find_dependencies(std::vector<std::string> &dependent_variables,
                           std::vector<std::string> &independent_variables);
};

#endif
