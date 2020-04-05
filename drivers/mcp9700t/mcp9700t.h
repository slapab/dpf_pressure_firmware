#ifndef DRIVERS_MCP9700T_H_
#define DRIVERS_MCP9700T_H_

#include <stdint.h>

void MCP9700T_convert(const int16_t* mv_buff, int8_t* out_deg, const uint16_t nbr);

#endif /* DRIVERS_MCP9700T_H_ */
