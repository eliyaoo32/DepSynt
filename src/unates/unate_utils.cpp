#include "unate_utils.h"

std::string unateTypeToString(UnateType type) {
    switch (type) {
        case UnateType::Positive:
            return "Positive";
        case UnateType::Negative:
            return "Negative";
        case UnateType::NotUnate:
            return "Not Unate";
        default:
            return "Unknown";
    }
}
