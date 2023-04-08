#include "nba_utils.h"

spot::twa_graph_ptr clone_nba(const spot::twa_graph_ptr& nba) {
    spot::twa::prop_set props;
    props.state_based = true;
    auto* cloned_nba = new spot::twa_graph(nba, props);

    return std::shared_ptr<spot::twa_graph>(cloned_nba);
}
