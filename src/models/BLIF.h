#ifndef REACTIVE_SYNTHESIS_BFSS_BLIF_H
#define REACTIVE_SYNTHESIS_BFSS_BLIF_H

#include <string>
#include <spot/twaalgos/aiger.hh>

using namespace std;

class BLIF {
private:
    string* m_blif_content;
    string m_model_name;
public:
    BLIF(string model_name): m_model_name(model_name) {}
    ~BLIF() { delete m_blif_content; }

    void load_aig(spot::aig_ptr& aig);

    void init_latch_to_one(string& latch_name);

    string find_latch_name_by_num(unsigned latch_num);

    BLIF* merge
};


#endif //REACTIVE_SYNTHESIS_BFSS_BLIF_H
