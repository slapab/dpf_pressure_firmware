#ifndef DFU_BOOTLOADER_CUSTOM_BOARD_H_
#define DFU_BOOTLOADER_CUSTOM_BOARD_H_ 1

#include <stdint.h>

#define APP_TIMER_PRESCALER     0                               /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE 4                               /**< Size of timer operation queues. */

#define NRF_CLOCK_LFCLKSRC { \
        .source = NRF_CLOCK_LF_SRC_SYNTH, \
        .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_250_PPM \
    };

#define ENTER_BOOTLOADER_REQ_VALUE UINT32_C(0xB1)

// Macros required for example by bootloader
#define LEDS_NUMBER 1
#define LED_START      8
#define LED_1          LED_START
#define BSP_ERROR_LED  LED_1
#define BSP_LED_0 LED_1
#define LEDS_ACTIVE_STATE 0

#define BUTTONS_NUMBER 0

// UART Stuff
#define TX_PIN_NUMBER 16
#define RX_PIN_NUMBER UART_PIN_DISCONNECTED // On PCB is 15
#define RTS_PIN_NUMBER UART_PIN_DISCONNECTED
#define CTS_PIN_NUMBER UART_PIN_DISCONNECTED
#define TX_BUF_SIZE 256
#define RX_BUF_SIZE 4

// Temperature analog pin
#define TEMP_PIN_NUMBER 1
#define TEMP_AIN NRF_ADC_CONFIG_INPUT_2
#define SIG_IN_PIN_NUMBER 2
#define SIG_AIN NRF_ADC_CONFIG_INPUT_3


#endif /* custom_board.h */
