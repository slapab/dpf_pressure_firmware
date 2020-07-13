#include "app_logic.h"
#include "mcp9700t.h"
#include "dac.h"
#include "adc_module.h"
#include "ping_pong_buffers.h"
#include "sys_time.h"
#include "nrf_queue.h"
#include "nrf_pt.h"
#include <signal.h>

#include "log_config.h"
#define NRF_LOG_MODULE_NAME "logic"
#define NRF_LOG_LEVEL LOG_CFG_LOGIC_LOG_LEVEL
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

typedef struct __attribute__((packed)) {
    nrf_adc_value_t sigin_buff[10];
    nrf_adc_value_t temp_buff[10];
    uint16_t samples_nbr;
} APP_LOGIC_adc_data_t;

static PT_THREAD(handle_adc_samples(pt_t* pt));

//static APP_LOGIC_adc_data_t temp_adc;
//static APP_LOGIC_adc_data_t sigin_adc;
static volatile sig_atomic_t got_adc = 0;

static pt_t handle_dc_pt;

#define ADC_SAMPLES_BUFF_DESCR_COUNT 5
PP_BUFFERS_DEF_QUEUES(APP_LOGIC_adc_data_t*, ADC_SAMPLES_BUFF_DESCR_COUNT, adc_buff_free_queue, adc_buff_in_use_queue);
static APP_LOGIC_adc_data_t adc_samples_buff_descr_pool[ADC_SAMPLES_BUFF_DESCR_COUNT][1];
static ping_pong_buffs_descr_t adc_pp_buffs_descr;

bool APP_LOGIC_init(void) {
    // Init ping pong buffers for ADC samples
    adc_pp_buffs_descr.free_buffers_queue = &adc_buff_free_queue;
    adc_pp_buffs_descr.in_use_buffers_queue = &adc_buff_in_use_queue;
    adc_pp_buffs_descr.item_size = sizeof(APP_LOGIC_adc_data_t);
    adc_pp_buffs_descr.mempool_rows = ADC_SAMPLES_BUFF_DESCR_COUNT;
    adc_pp_buffs_descr.mempool_cols = 1;
    adc_pp_buffs_descr.mempool = (uint8_t (*)[])adc_samples_buff_descr_pool; // cast to comply with uint8_t as mempool base type

    if (false == PP_BUFFERS_init(&adc_pp_buffs_descr)) {
        NRF_LOG_ERROR("Failed to init PP buffers for ADC\n");
        NRF_LOG_PROCESS();
        return false;
    }

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
//    if (0 != got_adc) {
//        NRF_LOG_ERROR("(adc irq) Overflow!!!\n");
//    }
//
//    for (uint16_t ridx = descr->temp_first_idx, widx = 0; ridx < descr->temp_smpls_nbr; ridx += 2, ++widx) {
//        temp_adc.buff[widx] = descr->buff[ridx];
//    }
//    temp_adc.samples_nbr = descr->temp_smpls_nbr / 2;
//
//    for (uint16_t ridx = descr->sig_first_idx, widx = 0; ridx < descr->sigin_smpls_nbr; ridx += 2, ++widx) {
//        sigin_adc.buff[widx] = descr->buff[ridx];
//        NRF_LOG_RAW_INFO("1. %d ", ADC_sigin_raw_to_millivolt(descr->buff[ridx]));
//        NRF_LOG_PROCESS();
//    }
//    NRF_LOG_RAW_INFO("\n");
//    sigin_adc.samples_nbr = descr->sigin_smpls_nbr / 2;
//
//    got_adc = 1;


    // new implementation
    APP_LOGIC_adc_data_t* buff = NULL;
    if (true == PP_BUFFERS_get_free_buffer(&adc_pp_buffs_descr, &buff)) {

        for (uint16_t sigin_ridx = descr->sig_first_idx, temp_ridx = descr->temp_first_idx, widx = 0;
                sigin_ridx < descr->sigin_smpls_nbr;
                sigin_ridx += 2, temp_ridx += 2, ++widx) {
            buff->sigin_buff[widx] = descr->buff[sigin_ridx];
            buff->temp_buff[widx] = descr->buff[temp_ridx];
//            NRF_LOG_RAW_INFO("2. %d ", ADC_sigin_raw_to_millivolt(descr->buff[sigin_ridx]));
//            NRF_LOG_PROCESS();
        }
        buff->samples_nbr = descr->sigin_smpls_nbr / 2;

//        NRF_LOG_RAW_INFO("\n");

        if (false == PP_BUFFERS_set_in_use_buffer(&adc_pp_buffs_descr, buff)) {
            NRF_LOG_ERROR("(adc irq) failed to add to in use queue\n");
        }
        got_adc = 1;
    } else {
        NRF_LOG_ERROR("(adc irq) no free buffers for ADC samples\n");
    }
}

static PT_THREAD(handle_adc_samples(pt_t* pt)) {
    PT_BEGIN(pt);
    UNUSED_VARIABLE(PT_YIELD_FLAG);


//    PT_WAIT_UNTIL(pt, 1 == got_adc);

    // TODO calculate average and schedule to DAC

//    got_adc = 0;
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
//    int32_t sigin_avg_mv = 0;
//    for (uint16_t i = 0; i < sigin_adc.samples_nbr; ++i) {
//        sigin_adc.buff[i] = ADC_sigin_raw_to_millivolt(sigin_adc.buff[i]);
//        sigin_avg_mv += ADC_sigin_raw_to_millivolt(sigin_adc.buff[i]);
//        NRF_LOG_RAW_INFO("%d ", sigin_adc.buff[i]);
//    }
//    sigin_avg_mv /= sigin_adc.samples_nbr;
//    NRF_LOG_RAW_INFO("\n");

    static APP_LOGIC_adc_data_t* smpls_buff = NULL;
    static int32_t sigin_avg = 0;
    static int32_t temp_avg = 0;
//    static uint32_t stp = 0;

    PT_WAIT_UNTIL(pt, 1 == got_adc);
    got_adc = 0;

    if (true == PP_BUFFERS_get_in_use_buffer(&adc_pp_buffs_descr, &smpls_buff)) {
        sigin_avg = 0;
        temp_avg = 0;
        for (uint16_t idx = 0; idx < smpls_buff->samples_nbr; ++idx) {
            sigin_avg += smpls_buff->sigin_buff[idx];
            temp_avg += smpls_buff->temp_buff[idx];
//            NRF_LOG_RAW_INFO("%d ", ADC_sigin_raw_to_millivolt(smpls_buff->sigin_buff[idx]));
//            int8_t deg = 0;
//            deg = 0;
//            int16_t deg_volt = ADC_temp_raw_to_millivolt(smpls_buff->temp_buff[idx]);
//            MCP9700T_convert(&deg_volt, &deg, 1);
//            NRF_LOG_RAW_INFO("(t:%d) ", deg);
//            NRF_LOG_PROCESS();
        }
        sigin_avg /= smpls_buff->samples_nbr;
        temp_avg /= smpls_buff->samples_nbr;
//        NRF_LOG_RAW_INFO("avg = %d, dt = %u, smpls len = %d\n", ADC_sigin_raw_to_millivolt(sigin_avg), SYS_TIME_get_tick() - stp, smpls_buff->samples_nbr);
//        stp = SYS_TIME_get_tick();

        // release buffer when is not necessary anymore
        if (false == PP_BUFFERS_set_free_buffer(&adc_pp_buffs_descr, smpls_buff)) {
            NRF_LOG_ERROR("(adc pthr) failed to set buffer free\n");
        }

        // update DAC
        if (false == DAC_update_dac(ADC_sigin_raw_to_millivolt(sigin_avg))) {
            NRF_LOG_ERROR("Failed to update dac\n");
            ADC_stop_conv();
            PT_EXIT(pt);
        }
    } else {
        NRF_LOG_ERROR("(adc pthr) false positive adc flag");
    }

    PT_RESTART(pt);

    PT_END(pt);
}

