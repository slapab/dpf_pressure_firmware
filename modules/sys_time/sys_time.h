#ifndef MODULES_SYS_TIME_H_
#define MODULES_SYS_TIME_H_

#include <stdint.h>
#include <stdbool.h>

#define SYS_TIME_IS_TIMEOUT(start_tp, timeout_ms) ((bool)((SYS_TIME_get_tick() - (start_tp)) > (timeout_ms)))

bool SYS_TIME_init(void);

uint32_t SYS_TIME_get_tick(void);


#endif /* MODULES_SYS_TIME_H_ */
