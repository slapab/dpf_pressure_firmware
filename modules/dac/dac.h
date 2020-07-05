#ifndef MODULES_DAC_H_
#define MODULES_DAC_H_

#include "mcp47x6.h"
#include <stdint.h>
#include <stdbool.h>

bool DAC_init(void);

bool DAC_read_blocking(MCP47x6_read_t* read_data);

bool DAC_write_vol_sett_blocking(void);

bool DAC_write_vol_dac_blocking(uint16_t dac);

#endif /* MODULES_DAC_H_ */
