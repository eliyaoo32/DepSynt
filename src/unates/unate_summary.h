#ifndef REACTIVE_SYNTHESIS_BFSS_UNATE_SUMMARY_H
#define REACTIVE_SYNTHESIS_BFSS_UNATE_SUMMARY_H

#include <vector>
#include <string>

enum class UnateType {
    Positive,
    Negative,
};

struct UnateStateSummary {
    unsigned state;
    std::string variable;
    vector<std::string> tested_variable;
    UnateType type;

    TimeMeasure totalDuration;
    TimeMeasure complementDuration;

    int removedEdges;
    int impactedEdges;
};

struct UnatesSummary {
    TimeMeasure totalDuration;
    TimeMeasure automatonConstructionDuration;
    TimeMeasure automatonCloneDuration;

    std::string benchmarkName;
    std::vector<std::string> outputVariables;
    std::vector<std::string> inputVariables;
    std::string formula;

    std::vector<UnateStateSummary> unateStates;

    int automatonSizeBefore;
    int automatonSizeAfter;
};

#endif //REACTIVE_SYNTHESIS_BFSS_UNATE_SUMMARY_H
