**Input:**
1. φ(I, O, D) LTL specification.
2. A = NBA of φ
3. A_proj = NBA of φ with projected dependent variables

**Output:**
* AIGER H(Input = I ∪ O, Output = D, Latch = |States(A)|)

**Algorithm:**
```
1. H = AIGER(Input = I ∪ O, Output = D, Latch = |States(A_proj)|)

# Define next latches
2. TransitionsOfDst: HashMap< State, Vector<State, BDD> >
3. For (s, B, s') ∊ Transitions(A_proj):
    3.1. TransitionsOfDst[s'].add( (s, B) )
4. For s ∊ States(A_proj):
    4.1. s_next_cond = H.or({
        H.and(H.latch(q), H.BDDToVar(BDD)) 
        for (q, BDD) ∊ TransitionsOfDst[s]
    })
    4.2. H.set_next_latch(s, s_next_cond)

# Define output of dependent variables
5. for d ∊ D:   # For each dependent variable
    5.1. d_cond = H.or(
        H.and(
            H.latch(s),
            PartialImpl(B, d)   # PartialImpl(B, d) is cached
        )
        for (s, B, s') ∊ Transitions(A):
    )
    5.2. H.set_output(d, d_cond) 

/*
Optional Optimization:
For the same BDD the process Ckt(BDD, d1) and Ckt(BDD, d2) shares the same construction except assignment of neg_n_v

Optional Optimization:
BDD1 may be included in BDD2, so in the process BDD2 we can conver BDD1.
*/
PartialImpl(BDD, d, BDD2Gate):
    If BDD == True:
        return AIG_True
    If BDD == False:
        Return AIG_False
    if BDD2Gate has BDD.Id():
        Return BDD2Gate[BDD.Id()]

    # Post-Order travels
    n0_child = PartialImpl(BDD.Low)
    n1_child = PartialImpl(BDD.High)

    if Var(BDD) ∊ I ∪ O:
        n_v = H.input_var(BDD.Var())
        neg_n_v = H.not(n_v)
    else:
        n_v = AIG_True
        if Var(BDD) == d:
            neg_n_v = 0
        else:
            neg_n_v = 1

    BDD2Gate[BDD.Id()] = H.or(
        H.and(neg_n_v, n0_child),
        H.and(n_v, n1_child)
    )
    
    return BDD2Gate[BDD.Id()]
```

