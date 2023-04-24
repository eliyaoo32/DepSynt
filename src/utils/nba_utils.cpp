#include "nba_utils.h"
#include <spot/twa/bddprint.hh>

spot::twa_graph_ptr clone_nba(const spot::twa_graph_ptr& nba) {
    spot::twa::prop_set props;
    props.state_based = true;
    auto* cloned_nba = new spot::twa_graph(nba, props);

    return std::shared_ptr<spot::twa_graph>(cloned_nba);
}

int count_edges(const spot::twa_graph_ptr& nba) {
    int counter = 0;

    for(auto& edge : nba->edges()) {
        if(edge.cond != bddfalse) {
            counter += 1;
        }
    }

    return counter;
}


void custom_print(std::ostream& out, spot::twa_graph_ptr& aut)
{
    // We need the dictionary to print the BDDs that label the edges
    const spot::bdd_dict_ptr& dict = aut->get_dict();

    // Some meta-data...
    out << "Acceptance: " << aut->get_acceptance() << '\n';
    out << "Number of sets: " << aut->num_sets() << '\n';
    out << "Number of states: " << aut->num_states() << '\n';
    out << "Number of edges: " << aut->num_edges() << '\n';
    out << "Initial state: " << aut->get_init_state_number() << '\n';
    out << "Atomic propositions:";
    for (spot::formula ap: aut->ap())
        out << ' ' << ap << " (=" << dict->varnum(ap) << ')';
    out << '\n';

    // Arbitrary data can be attached to automata, by giving them
    // a type and a name.  The HOA parser and printer both use the
    // "automaton-name" to name the automaton.
    if (auto name = aut->get_named_prop<std::string>("automaton-name"))
        out << "Name: " << *name << '\n';

    // For the following prop_*() methods, the return value is an
    // instance of the spot::trival class that can represent
    // yes/maybe/no.  These properties correspond to bits stored in the
    // automaton, so they can be queried in constant time.  They are
    // only set whenever they can be determined at a cheap cost: for
    // instance an algorithm that always produces deterministic automata
    // would set the deterministic property on its output.  In this
    // example, the properties that are set come from the "properties:"
    // line of the input file.
    out << "Complete: " << aut->prop_complete() << '\n';
    out << "Deterministic: " << (aut->prop_universal()
                                 && aut->is_existential()) << '\n';
    out << "Unambiguous: " << aut->prop_unambiguous() << '\n';
    out << "State-Based Acc: " << aut->prop_state_acc() << '\n';
    out << "Terminal: " << aut->prop_terminal() << '\n';
    out << "Weak: " << aut->prop_weak() << '\n';
    out << "Inherently Weak: " << aut->prop_inherently_weak() << '\n';
    out << "Stutter Invariant: " << aut->prop_stutter_invariant() << '\n';

    // States are numbered from 0 to n-1
    unsigned n = aut->num_states();
    for (unsigned s = 0; s < n; ++s)
    {
        out << "State " << s << ":\n";

        // The out(s) method returns a fake container that can be
        // iterated over as if the contents was the edges going
        // out of s.  Each of these edges is a quadruplet
        // (src,dst,cond,acc).  Note that because this returns
        // a reference, the edge can also be modified.
        for (auto& t: aut->out(s))
        {
            out << "  edge(" << t.src << " -> " << t.dst << ")\n    label = ";
            spot::bdd_print_formula(out, dict, t.cond);
            out << "\n    acc sets = " << t.acc << '\n';
        }
    }
}

