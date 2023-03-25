#ifndef REACTIVE_SYNTHESIS_BFSS_BLIF_H
#define REACTIVE_SYNTHESIS_BFSS_BLIF_H

#include <string>
#include <spot/twaalgos/aiger.hh>
#include <memory>

using namespace std;

class BLIF;
using BLIF_ptr = std::shared_ptr<BLIF>;

class BLIF {
private:
    string* m_blif_content;
    string m_model_name;
public:
    BLIF(string model_name): m_model_name(model_name) {}
    ~BLIF() { delete m_blif_content; }

    void load_aig(spot::aig_ptr& aig);

    void load_string(string& content);

    void init_latch_to_one(string& latch_name);

    string find_latch_name_by_num(unsigned latch_num);

    spot::aig_ptr to_aig(spot::bdd_dict_ptr& dict);
public:
    friend ostream& operator<<(ostream& os, const BLIF& sm);

    /**
     * Merge independent and dependent strategies into a single BLIF.
     * The format of the merged BLIF:
     *
     * .model <model_name>
     * .inputs <inputs>
     * .outputs <outputs> // outputs = indep_vars + dep_vars
     * .names <output>  // Create wired variables for output variables
     *  1 1
     *
     * .subckt <indep_blif> <inputs> <indep_wired_vars>
     * .subckt <dep_blif> <inputs> <indep_wired_vars> <dep_wired_vars>
     *  .end
     *
     *  <indep_blif>
     *  <dep_blif>
     *
     * @param indep_blif
     * @param dep_blif
     * @param inputs
     * @param indep_vars
     * @param dep_vars
     * @param model_name
     * @return BLIF of merged strategies
     */
    static BLIF_ptr merge_dependency_strategies(BLIF& indep_blif, BLIF& dep_blif,
                                             const vector<string>& inputs,
                                             const vector<string>& indep_vars,
                                             const vector<string>& dep_vars,
                                             string& model_name);
};

inline string blif_wired_var(const string& var) { return "In" + var; }

#endif //REACTIVE_SYNTHESIS_BFSS_BLIF_H
