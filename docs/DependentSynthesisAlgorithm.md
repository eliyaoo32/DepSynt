In order to synthesis a strategy for dependent variable we apply the following algorithm:

## Synthesis an Aiger for Dependents Output Variables
**Input:**
- LTL specification $\varphi(I, O \setminus D, D)$, where:
    - $I$ = Input vars
    - $O$ = Output vars
    - $D = \{ d_1,...,d_k \}$ = Dependent Output Vars.
- $A_\varphi'$ - NBA with projected dependent variables.

**Output:**
- AIGER
    - Input = $I \cup O \setminus D$
    - Latches = $2 \cdot \#States(A_\varphi')$
    - Outputs = $\emptyset$

**Algorithm:**
```
aig = new AIG(
    Latches=2 * #States(Aϕ),
    Input=I ∪ O \ D,
    Output={ d_1, ..., d_k }
)

bdd_to_gate = HashMap<BDD, number>

states_dst = HashMap< S \in States, Vector<Pairs<S' \in State, BDD B>> > # i.e., exists transition (S', B, S)

state_to_aig1_latech = (state_num) => state_num
state_to_aig3_latech = (state_num) => #(States) + state_num

for state in States(Aϕ):
    for out in outs(state):
        src, bdd, dst = out

        if not (bdd ∈ bdd_to_gate):
            bdd_to_gate[bdd] = aig.bddToGate( bdd )

        next_latech_to_gate[dst].add( (src, bdd_to_gate[bdd]) )

for dst, (src, condition) ∈ next_latech_to_gate:
    # Possible optimizations: cache the gate_nums vector and use the same gate number if already created

    aig.set_latch_next(
        state_to_aig1_latech(dst),
        aig.AND(aig.OR(gate_nums), src)
    )


```
