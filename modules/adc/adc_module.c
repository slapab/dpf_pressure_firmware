#include "adc_module.h" 
#include "tim1_sharing.h"
#include "custom_board.h"
#include "nrf_drv_ppi.h"
#include "app_error.h"
#include "nrf.h"
#include "nrf_drv_timer.h"
#include "signal.h"
#include "log_config.h"
#define NRF_LOG_MODULE_NAME "adcm"
#define NRF_LOG_LEVEL LOG_CFG_ADC_LOG_LEVEL
#include "nrf_log.h"

/**
 * @brief ADC interrupt handler.
 */
static void adc_event_handler(nrf_drv_adc_evt_t const* evt);

static ret_code_t init_adc(void);

static void timer_event_handler(nrf_timer_event_t event_type, void* p_context);

static nrf_drv_timer_t  tim2 = NRF_DRV_TIMER_INSTANCE(2);


/// Channel configuration for Temp sensor
/// Temp sernsor output is max 1,75V, and using 2/3 as prescaling and VBG reference, gives max 1,8V of input signal
static nrf_drv_adc_channel_t m_temp_channel = {
        .config.config.resolution = NRF_ADC_CONFIG_RES_10BIT,
        .config.config.reference = NRF_ADC_CONFIG_REF_VBG,
        .config.config.input = NRF_ADC_CONFIG_SCALING_INPUT_TWO_THIRDS,
        .config.config.ain = TEMP_AIN
};

/// Channel configuration for DPF signal input
static nrf_drv_adc_channel_t m_sig_in_channel = {
        .config.config.resolution = NRF_ADC_CONFIG_RES_10BIT,
        .config.config.reference = NRF_ADC_CONFIG_REF_VBG,
        .config.config.input = NRF_ADC_CONFIG_SCALING_INPUT_ONE_THIRD,
        .config.config.ain = SIG_AIN
};

enum {
    ADC_TEMP_CHANNEL_IDX = 0,
    ADC_SIG_IN_CHANNEL_IDX = 1,
    ADC_NBR_OF_CHANNELS = 2
};

#define ADC_SAMPLES_PER_CHANNEL 10
#define ADC_SAMPLES_COUNT_IN_BUFFERS (ADC_NBR_OF_CHANNELS * ADC_SAMPLES_PER_CHANNEL)
static nrf_adc_value_t smpls_buffers_pool[2][ADC_SAMPLES_COUNT_IN_BUFFERS];

static nrf_ppi_channel_t sampling_ppi_ch;

/// Application callback for sending samples
static ADC_samples_callback_t app_callback;

/// Flag that tells interrupt handler about stopped sampling by application
static sig_atomic_t stop_sampling;

bool ADC_init(const ADC_init_t* init) {
    bool ret_val = false;
    stop_sampling = true;

    // configure sampling timer
    const nrf_drv_timer_config_t cfg = {
        .frequency = NRF_TIMER_FREQ_31250Hz,
        .mode = NRF_TIMER_MODE_TIMER,
        .bit_width = NRF_TIMER_BIT_WIDTH_16,
        .interrupt_priority = TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
    };

    ret_code_t err = nrf_drv_timer_init(&tim2, &cfg, timer_event_handler);
    APP_ERROR_CHECK(err);
    uint32_t ticks = nrf_drv_timer_ms_to_ticks(&tim2, 100);
    nrf_drv_timer_extended_compare(&tim2, NRF_TIMER_CC_CHANNEL0, ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);


    // configure PPI to sampling by timer
    nrf_drv_ppi_init();
    err = nrf_drv_ppi_channel_alloc(&sampling_ppi_ch);
    if (NRF_SUCCESS == err) {
        APP_ERROR_CHECK(nrf_drv_ppi_channel_assign(sampling_ppi_ch,
                nrf_drv_timer_compare_event_address_get(&tim2, NRF_TIMER_CC_CHANNEL0),
                nrf_drv_adc_start_task_get()));

        app_callback = init->samples_callback;

        ret_val = true;
    } else {
        APP_ERROR_CHECK(err);
    }

    return ret_val;
}

bool ADC_start_conv(void) {
    stop_sampling = false;
    APP_ERROR_CHECK(init_adc());
    APP_ERROR_CHECK(nrf_drv_adc_buffer_convert(&smpls_buffers_pool[0][0], ADC_SAMPLES_COUNT_IN_BUFFERS));
    // start sampling
    nrf_drv_timer_enable(&tim2);
    APP_ERROR_CHECK(nrf_drv_ppi_channel_enable(sampling_ppi_ch));
    return true;
}

bool ADC_stop_conv(void) {
    stop_sampling = true;
    // stop sampling
    nrf_drv_ppi_channel_disable(sampling_ppi_ch);
    nrf_drv_timer_disable(&tim2);
    // stop ongoing conversion
    nrf_adc_stop();
    nrf_drv_adc_uninit();
    return true;
}

static ret_code_t init_adc(void) {
    nrf_drv_adc_config_t config = {
            .interrupt_priority = ADC_CONFIG_IRQ_PRIORITY
    };

    ret_code_t err = nrf_drv_adc_init(&config, adc_event_handler);
    if (NRF_SUCCESS == err) {
        // configure channel for temp
        nrf_drv_adc_channel_enable(&m_temp_channel);

        // configure channel for DPF pressure input signal
        nrf_drv_adc_channel_enable(&m_sig_in_channel);
    } else {
        NRF_LOG_ERROR("Failed to initialize ADC, err code %u\n", err)
    }
    return err;
}

static void adc_event_handler(nrf_drv_adc_evt_t const* evt) {
    if (NRF_DRV_ADC_EVT_DONE == evt->type) {
        if (NULL != app_callback) {

            // schedule next sampling
            if (false == stop_sampling) { // can continue sampling
                nrf_adc_value_t* next_buff = ((uintptr_t)&smpls_buffers_pool[0][0] == (uintptr_t)evt->data.done.p_buffer) ?
                        &smpls_buffers_pool[1][0] : &smpls_buffers_pool[0][0];
                nrf_drv_adc_buffer_convert(next_buff, ADC_SAMPLES_COUNT_IN_BUFFERS);
            }

            // report samples stored in previous buffer to the application logic
            const ADC_samples_t descr = {
                .buff = evt->data.done.p_buffer,
                .temp_first_idx = 0,
                .temp_smpls_nbr = evt->data.done.size,
                .sig_first_idx = 1,
                .sigin_smpls_nbr = evt->data.done.size
            };
            app_callback(&descr);
        }
    }
}

static void timer_event_handler(nrf_timer_event_t event_type, void* p_context) {
    UNUSED_PARAMETER(event_type);
    UNUSED_PARAMETER(p_context);
}



