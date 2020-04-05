#include "app_logic.h"
#include "mcp9700t.h"
#include <signal.h>
#include "log_config.h"
#define NRF_LOG_MODULE_NAME "logic"
#define NRF_LOG_LEVEL LOG_CFG_LOGIC_LOG_LEVEL
#include "nrf_log.h"


typedef struct {
    nrf_adc_value_t buff[50];
    uint16_t samples_nbr;
} APP_LOGIC_adc_data_t;

static APP_LOGIC_adc_data_t temp_adc;
static APP_LOGIC_adc_data_t sigin_adc;
static volatile sig_atomic_t got_adc = 0;

void APP_LOGIC_pool(void) {
    if (0 != got_adc) {
        got_adc = 0;
        NRF_LOG_INFO("Got ADC samples, temp smpls len: %d, sig_in smpls len %d\n",
                temp_adc.samples_nbr, sigin_adc.samples_nbr);

        NRF_LOG_INFO("Temp meas\n");
        for (uint16_t i = 0; i < temp_adc.samples_nbr; ++i) {
            temp_adc.buff[i] = ADC_temp_raw_to_millivolt(temp_adc.buff[i]);
            int8_t deg;
            MCP9700T_convert(&temp_adc.buff[i], &deg, 1);
            NRF_LOG_RAW_INFO("%d (%d °C)", temp_adc.buff[i], deg);
        }
        NRF_LOG_RAW_INFO("\n");

        NRF_LOG_INFO("Sigin meas\n");
        for (uint16_t i = 0; i < sigin_adc.samples_nbr; ++i) {
            sigin_adc.buff[i] = ADC_sigin_raw_to_millivolt(sigin_adc.buff[i]);
            NRF_LOG_RAW_INFO("%d ", sigin_adc.buff[i]);
        }
        NRF_LOG_RAW_INFO("\n");
    }
}


void APP_LOGIC_adc_samples_callback(const ADC_samples_t* descr) {
    for (uint16_t ridx = descr->temp_first_idx, widx = 0; ridx < descr->temp_smpls_nbr; ridx += 2, ++widx) {
        temp_adc.buff[widx] = descr->buff[ridx];
    }
    temp_adc.samples_nbr = descr->temp_smpls_nbr / 2;

    for (uint16_t ridx = descr->sig_first_idx, widx = 0; ridx < descr->sigin_smpls_nbr; ridx += 2, ++widx) {
        sigin_adc.buff[widx] = descr->buff[ridx];
    }
    sigin_adc.samples_nbr = descr->sigin_smpls_nbr / 2;

    got_adc = 1;
}
