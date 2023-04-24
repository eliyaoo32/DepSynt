#ifndef NBA_UTILS_H
#define NBA_UTILS_H

#include <iostream>
#include <spot/twa/twa.hh>
#include <spot/twa/twagraph.hh>

spot::twa_graph_ptr clone_nba(const spot::twa_graph_ptr& nba);

int count_edges(const spot::twa_graph_ptr& nba);

void custom_print(std::ostream& out, spot::twa_graph_ptr& aut);

#endif