#include "tim1_sharing.h"

#include "nrf.h"
#include "nrf_drv_timer.h"

#include "nrf_log.h"

enum {
    TIM1_CC_CHANNEL_FOR_SYSTIME = NRF_TIMER_CC_CHANNEL0,
};


static void timer_event_handler(nrf_timer_event_t event_type, void* p_context);

static nrf_drv_timer_t  tim1 = NRF_DRV_TIMER_INSTANCE(1);

extern volatile uint32_t systime_counter;


bool TIM1_SHARING_init(void) {
    const nrf_drv_timer_config_t cfg = {
        .frequency = NRF_TIMER_FREQ_31250Hz,
        .mode = NRF_TIMER_MODE_TIMER,
        .bit_width = NRF_TIMER_BIT_WIDTH_16,
        .interrupt_priority = TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
    };

    uint32_t err = nrf_drv_timer_init(&tim1, &cfg, timer_event_handler);

    if (NRF_SUCCESS == err) { // first time initialized
        // FOR SYSTIME
        // enable extended compare to clear counter and enable interrupt
        uint32_t ticks = nrf_drv_timer_ms_to_ticks(&tim1, 1);
        nrf_drv_timer_extended_compare(&tim1, TIM1_CC_CHANNEL_FOR_SYSTIME, ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

        nrf_drv_timer_enable(&tim1);
    }

    return NRF_SUCCESS == err || NRF_ERROR_INVALID_STATE == err;
}

static void timer_event_handler(nrf_timer_event_t event_type, void* p_context) {
    UNUSED_PARAMETER(p_context);
    switch (event_type) {
    case NRF_TIMER_EVENT_COMPARE0:
        ++systime_counter;
        break;
    default:
        // do nothing
        break;
    }
}
