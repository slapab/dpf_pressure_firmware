#ifndef MODULES_SYS_TIME_H_
#define MODULES_SYS_TIME_H_

#include <stdint.h>
#include <stdbool.h>

bool SYS_TIME_init(void);

uint32_t SYS_TIME_get_tick(void);


#endif /* MODULES_SYS_TIME_H_ */
