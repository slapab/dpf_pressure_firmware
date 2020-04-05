#ifndef MODULES_APP_LOGIC_H_
#define MODULES_APP_LOGIC_H_

#include "adc_module.h"
#include <stdint.h>
#include <stdbool.h>

void APP_LOGIC_pool(void);

void APP_LOGIC_adc_samples_callback(const ADC_samples_t* descr);

#endif /* MODULES_APP_LOGIC_H_ */
