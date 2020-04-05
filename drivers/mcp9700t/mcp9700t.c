#include "mcp9700t.h"

#define MCP9700_T0C_MV INT16_C(500)   // mV
#define MCP9700_TC_MV   INT16_C(10)    // mV/C deg

static inline int8_t MCP9700T_one_convert(const int16_t mv);

void MCP9700T_convert(const int16_t* mv_buff, int8_t* out_deg, const uint16_t nbr) {
    for (uint16_t i = 0; i < nbr; ++i) {
        out_deg[i] = MCP9700T_one_convert(mv_buff[i]);
    }
}


static inline int8_t MCP9700T_one_convert(const int16_t mv) {
    return (mv - MCP9700_T0C_MV) / MCP9700_TC_MV;
}




