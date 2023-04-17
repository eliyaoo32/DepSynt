#ifndef REACTIVE_SYNTHESIS_BFSS_UNATE_UTILS_H
#define REACTIVE_SYNTHESIS_BFSS_UNATE_UTILS_H

#include <vector>
#include <string>

enum class UnateType {
    Positive,
    Negative,
};

std::string unateTypeToString(UnateType type);


#endif //REACTIVE_SYNTHESIS_BFSS_UNATE_UTILS_H
