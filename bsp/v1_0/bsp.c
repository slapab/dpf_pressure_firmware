#include "custom_board.h"

#include "nrf_gpio.h"

void bsp_board_led_on(uint32_t led) {
    nrf_gpio_pin_clear(led);
}

void bsp_board_led_off(uint32_t led) {
    nrf_gpio_pin_set(led);
}

void bsp_board_leds_init(void) {
    nrf_gpio_cfg_output(BSP_LED_0);

    bsp_board_led_off(BSP_LED_0);
}
