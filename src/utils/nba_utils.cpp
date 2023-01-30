#include "nba_utils.h"

spot::twa_graph_ptr clone_nba(spot::twa_graph_ptr nba) {
    spot::const_twa_graph_ptr nba_to_clone = nba;

    spot::twa::prop_set props;
    props.state_based = true;
    spot::twa_graph* cloned_nba = new spot::twa_graph(nba_to_clone, props);

    return std::shared_ptr<spot::twa_graph>(cloned_nba);
}
