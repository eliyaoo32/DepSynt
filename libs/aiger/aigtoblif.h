#ifndef REACTIVE_SYNTHESIS_BFSS_AIGTOBLIF_H
#define REACTIVE_SYNTHESIS_BFSS_AIGTOBLIF_H

#ifdef __cplusplus
extern "C"
{
#endif

int aigtoblif(FILE* aig_src, FILE* blif_dst, const char* model_name);

#ifdef __cplusplus
}
#endif

#endif //REACTIVE_SYNTHESIS_BFSS_AIGTOBLIF_H
