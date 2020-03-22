
#include "sys_time.h"
#include "nrf.h"
#include "nrf_drv_timer.h"

static void timer_event_handler(nrf_timer_event_t event_type, void* p_context);

static nrf_drv_timer_t  tim1 = NRF_DRV_TIMER_INSTANCE(1);

static volatile uint32_t tim1_counter;


bool SYS_TIME_init(void) {

    const nrf_drv_timer_config_t cfg = {
        .frequency = NRF_TIMER_FREQ_31250Hz,
        .mode = NRF_TIMER_MODE_TIMER,
        .bit_width = NRF_TIMER_BIT_WIDTH_32,
        .interrupt_priority = TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
    };

    tim1_counter = 0U;
    uint32_t err = nrf_drv_timer_init(&tim1, &cfg, timer_event_handler);

    if (NRF_SUCCESS == err) { // first time initialized
        // enable extended compare to clear counter and enable interrupt
        uint32_t ticks = nrf_drv_timer_ms_to_ticks(&tim1, 1);
        nrf_drv_timer_extended_compare(&tim1, NRF_TIMER_CC_CHANNEL0, ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
        nrf_drv_timer_enable(&tim1);
    }

    return NRF_SUCCESS == err || NRF_ERROR_INVALID_STATE == err;
}

uint32_t SYS_TIME_get_tick(void) {
    return tim1_counter;
}


static void timer_event_handler(nrf_timer_event_t event_type, void* p_context) {
    UNUSED_PARAMETER(p_context);
    switch (event_type) {
    case NRF_TIMER_EVENT_COMPARE0:
        ++tim1_counter;
        break;
    default:
        // do nothing
        break;
    }
}



