
#include "synt_instance.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "utils.h"

void SyntInstance::build_all_vars() {
    for (std::string& var : boost::join(m_input_vars, m_output_vars)) {
        m_all_vars.push_back(var);
    }
}

SyntInstance::SyntInstance(const std::string& input_str,
                           const std::string& output_str,
                           const std::string& formula_str)
    : m_formula(formula_str) {
    extract_variables(input_str, m_input_vars);
    extract_variables(output_str, m_output_vars);

    build_all_vars();
    construct_formula();
}

SyntInstance::SyntInstance(std::vector<std::string>& inputs,
                           std::vector<std::string>& outputs, std::string& formula)
    : m_input_vars(inputs), m_output_vars(outputs), m_formula(formula) {
    build_all_vars();
    construct_formula();
};

SyntInstance::SyntInstance(std::vector<std::string>& inputs,
                           std::vector<std::string>& outputs, spot::formula& formula)
    : m_input_vars(inputs), m_output_vars(outputs), m_formula_parsed(formula) {
    build_all_vars();

    // Convert spot formula to string
    std::stringstream ss;
    ss << formula;
    m_formula = ss.str();
};

void SyntInstance::all_vars_excluded(std::vector<std::string>& dst,
                                     const std::vector<std::string>& excluded) {
    dst.clear();

    for (const std::string& var : m_all_vars) {
        if (std::find(excluded.begin(), excluded.end(), var) == excluded.end()) {
            dst.push_back(var);
        }
    }
}

void SyntInstance::order_output_vars(std::vector<std::string>& expected_order) {
    auto pred = [&expected_order](const std::string& a, const std::string& b) {
        auto it_a = std::find(expected_order.begin(), expected_order.end(), a);
        auto it_b = std::find(expected_order.begin(), expected_order.end(), b);

        if (it_a == expected_order.end() || it_b == expected_order.end()) {
            throw std::runtime_error("Output variable not found in expected order");
        }

        return it_a < it_b;
    };

    std::sort(m_output_vars.begin(), m_output_vars.end(), pred);
}

void SyntInstance::construct_formula() {
    spot::parsed_formula pf = spot::parse_infix_psl(this->m_formula);

    if (pf.format_errors(std::cerr)) {
        throw std::runtime_error("Error parsing formula_str: " + this->m_formula);
    }
    if (pf.f == nullptr) {
        throw std::runtime_error("Formula is not built yet");
    }

    this->m_formula_parsed = spot::formula(pf.f);
}

spot::twa_graph_ptr construct_automaton(SyntInstance& synt_instance) {
    spot::translator trans;
    trans.set_type(spot::postprocessor::Buchi);
    trans.set_pref(spot::postprocessor::SBAcc);
    auto automaton = trans.run(synt_instance.get_formula_parsed());

    return automaton;
}

std::ostream& operator<<(std::ostream& out, SyntInstance& instance) {
    out << "formula: " << instance.get_formula_str() << std::endl;
    out << "input vars: " << instance.get_input_vars() << std::endl;
    out << "output vars: " << instance.get_output_vars() << std::endl;
    return out;
}
