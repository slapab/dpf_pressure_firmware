#include "app_logic.h"
#include "mcp9700t.h"
#include "dac.h"
#include "adc_module.h"
#include "nrf_pt.h"
#include <signal.h>

#include "log_config.h"
#define NRF_LOG_MODULE_NAME "logic"
#define NRF_LOG_LEVEL LOG_CFG_LOGIC_LOG_LEVEL
#include "nrf_log.h"


typedef struct {
    nrf_adc_value_t buff[50];
    uint16_t samples_nbr;
} APP_LOGIC_adc_data_t;

static PT_THREAD(handle_adc_samples(pt_t* pt));

static APP_LOGIC_adc_data_t temp_adc;
static APP_LOGIC_adc_data_t sigin_adc;
static volatile sig_atomic_t got_adc = 0;

static pt_t handle_dc_pt;

bool APP_LOGIC_init(void) {
    PT_INIT(&handle_dc_pt);
    // start ADC conversion
    ADC_start_conv();

    return true;
}

void APP_LOGIC_pool(void) {
    (void)PT_SCHEDULE(handle_adc_samples(&handle_dc_pt));

//    if (0 != got_adc) {
//        got_adc = 0;
//        NRF_LOG_INFO("Got ADC samples, temp smpls len: %d, sig_in smpls len %d\n",
//                temp_adc.samples_nbr, sigin_adc.samples_nbr);
//
//        NRF_LOG_INFO("Temp meas\n");
//        for (uint16_t i = 0; i < temp_adc.samples_nbr; ++i) {
//            temp_adc.buff[i] = ADC_temp_raw_to_millivolt(temp_adc.buff[i]);
//            int8_t deg;
//            MCP9700T_convert(&temp_adc.buff[i], &deg, 1);
//            NRF_LOG_RAW_INFO("%d (%d °C)", temp_adc.buff[i], deg);
//        }
//        NRF_LOG_RAW_INFO("\n");
//
//        NRF_LOG_INFO("Sigin meas\n");
//        for (uint16_t i = 0; i < sigin_adc.samples_nbr; ++i) {
//            sigin_adc.buff[i] = ADC_sigin_raw_to_millivolt(sigin_adc.buff[i]);
//            NRF_LOG_RAW_INFO("%d ", sigin_adc.buff[i]);
//        }
//        NRF_LOG_RAW_INFO("\n");
//
//        // test DAC
//        static uint32_t dac_mv = 0U;
//        if (false == DAC_update_dac(dac_mv)) {
//            NRF_LOG_ERROR("Failed to update dac\n");
//        } else {
//            NRF_LOG_INFO("DAC mv: %u\n", dac_mv);
//            dac_mv += 50;
//            if (dac_mv > 4000) {
//                dac_mv = 0;
//            }
//        }
//    }
}


void APP_LOGIC_adc_samples_callback(const ADC_samples_t* descr) {
    if (0 != got_adc) {
        NRF_LOG_ERROR("(adc irq) Overflow!!!\n");
    }

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

static PT_THREAD(handle_adc_samples(pt_t* pt)) {
    PT_BEGIN(pt);
    UNUSED_VARIABLE(PT_YIELD_FLAG);


    PT_WAIT_UNTIL(pt, 1 == got_adc);

    // TODO calculate average and schedule to DAC

    got_adc = 0;
//    NRF_LOG_INFO("Got ADC samples, temp smpls len: %d, sig_in smpls len %d\n",
//            temp_adc.samples_nbr, sigin_adc.samples_nbr);

//    NRF_LOG_INFO("Temp meas\n");
//    for (uint16_t i = 0; i < temp_adc.samples_nbr; ++i) {
//        temp_adc.buff[i] = ADC_temp_raw_to_millivolt(temp_adc.buff[i]);
//        int8_t deg;
//        MCP9700T_convert(&temp_adc.buff[i], &deg, 1);
//        NRF_LOG_RAW_INFO("%d (%d °C)", temp_adc.buff[i], deg);
//    }
//    NRF_LOG_RAW_INFO("\n");

//    NRF_LOG_INFO("Sigin meas\n");
    int32_t sigin_avg_mv = 0;
    for (uint16_t i = 0; i < sigin_adc.samples_nbr; ++i) {
//        sigin_adc.buff[i] = ADC_sigin_raw_to_millivolt(sigin_adc.buff[i]);
        sigin_avg_mv += ADC_sigin_raw_to_millivolt(sigin_adc.buff[i]);
//        NRF_LOG_RAW_INFO("%d ", sigin_adc.buff[i]);
    }
    sigin_avg_mv /= sigin_adc.samples_nbr;
//    NRF_LOG_RAW_INFO("\n");

    // test DAC
//    static uint32_t dac_mv = 0U;
//    if (false == DAC_update_dac(dac_mv)) {
//        NRF_LOG_ERROR("Failed to update dac\n");
//    } else {
//        NRF_LOG_INFO("DAC mv: %u\n", dac_mv);
//        dac_mv += 50;
//        if (dac_mv > 4000) {
//            dac_mv = 0;
//        }
//    }

    if (false == DAC_update_dac(sigin_avg_mv)) {
        NRF_LOG_ERROR("Failed to update dac\n");
        ADC_stop_conv();
        PT_EXIT(pt);
    }

    PT_RESTART(pt);

    PT_END(pt);
}

