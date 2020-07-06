#ifndef MODULES_ADC_ADC_MOD_H_
#define MODULES_ADC_ADC_MOD_H_

#include "nrf_drv_adc.h"
#include <stdint.h>
#include <stdbool.h>

/// 10-bit resolution for each sampling channel
#define ADC_MODULE_CHANNELS_RESOLUTION INT32_C(1024)
/// Voltage reference for sampling channels
#define ADC_MODULE_CHANNELS_VREF_MV INT32_C(1200)


typedef struct {
    nrf_adc_value_t* buff;
    uint16_t temp_first_idx;
    uint16_t temp_smpls_nbr;
    uint16_t sig_first_idx;
    uint16_t sigin_smpls_nbr;
} ADC_samples_t;

typedef void (*ADC_samples_callback_t)(const ADC_samples_t* descr);

typedef struct {
    ADC_samples_callback_t samples_callback;
    uint32_t sampling_period_us;
    uint16_t samples_nbr_per_channel;
} ADC_init_t;

bool ADC_init(const ADC_init_t* init);

bool ADC_start_conv(void);

bool ADC_stop_conv(void);

inline nrf_adc_value_t ADC_temp_raw_to_millivolt(const nrf_adc_value_t raw) {
    // for temp sensor using 2/3 internal divider of input voltage, so use
    enum {
        DIVIDER_INV_MULTIPLY = INT32_C(3),
        DIVIDER_INV_DIVIDE = INT32_C(2)
    };
    return (raw * ADC_MODULE_CHANNELS_VREF_MV * DIVIDER_INV_MULTIPLY) / ADC_MODULE_CHANNELS_RESOLUTION / DIVIDER_INV_DIVIDE;
}

inline nrf_adc_value_t ADC_sigin_raw_to_millivolt(const nrf_adc_value_t raw) {
    /// Inverse of Signal-In channel prescaling (input voltage divider)
    enum {
        ADC_MODULE_SIGIN_INVERSE_OF_PRESCALLING = INT32_C(3)
    };
    return (raw * ADC_MODULE_CHANNELS_VREF_MV * ADC_MODULE_SIGIN_INVERSE_OF_PRESCALLING) / ADC_MODULE_CHANNELS_RESOLUTION;
}

#endif /* MODULES_ADC_ADC_MOD_H_ */
