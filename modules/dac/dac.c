#include "custom_board.h"
#include "dac.h"
#include "sys_time.h"
#include "nrf_drv_twi.h"
#include "nrf_delay.h"
#include "nrf_queue.h"
#include "signal.h"


#include "log_config.h"
#define NRF_LOG_MODULE_NAME "dac"
#define NRF_LOG_LEVEL LOG_CFG_DAC_LOG_LEVEL
#include "nrf_log.h"

#define TWI_SLAVE_ADDR MCP47x6A0T_E
#define DAC_MCP47x6_TYPE MCP4726_12BIT_TYPE
#define DAC_MCP47x6_READ_LEN MCP47_1_2__6_TWI_READ_LEN

#if DAC_MCP47x6_TYPE == MCP4726_12BIT_TYPE
#define DAC_CONV_MV_TO_RAW(mv) (uint16_t)(((uint32_t)(mv) * UINT32_C(4095)) / UINT32_C(5000))
#endif

#define DAC_UPDATE_SETTINGS_INITIALIZER { \
        .type = DAC_MCP47x6_TYPE, \
        .vref = MCP47x6_VREF_EXT_UNBUFFERED, \
        .pwr_down = MCP47x6_PD_OFF, \
        .gain = MCP47x6_GAIN_X2 /* external reference ~2,495V */ \
    }

static void twi_evt_handler(const nrf_drv_twi_evt_t* p_event, void* p_context);

static ret_code_t init_twi_blocking(void);
static ret_code_t init_twi_nonblocking(void);
static bool verify_dac_config(void);
static bool compare_dac_config(const MCP47x6_settings_t* sett);
static bool write_nv_dac_config(void);

static const nrf_drv_twi_t twi = NRF_DRV_TWI_INSTANCE(0);

static const nrf_drv_twi_config_t twi_cfg = {
    .scl = DAC_TWI_SCL_PIN_NUMBER,
    .sda = DAC_TWI_SDA_PIN_NUMBER,
    .frequency = NRF_TWI_FREQ_400K,
    .clear_bus_init = true,
    .hold_bus_uninit = false
};

static const MCP47x6_settings_t dac_nv_settings = {
    .type = DAC_MCP47x6_TYPE,
    .vref = MCP47x6_VREF_EXT_UNBUFFERED,
    .pwr_down = MCP47x6_PD_OFF,
    .gain = MCP47x6_GAIN_X2
};
/// Initial DAC voltage in milivolts that must be saved in non-volatile memory.
static const uint16_t dac_nv_init_value_mv = 500U;

#define DAC_VALUES_QUEUE_LEN 50U
NRF_QUEUE_DEF(uint16_t, m_dac_values_queue, DAC_VALUES_QUEUE_LEN, NRF_QUEUE_MODE_NO_OVERFLOW);

#define TWI_XFER_IS_DONE 1
#define TWI_XFER_IN_PROGRESS 0
static volatile sig_atomic_t xfer_done = TWI_XFER_IS_DONE;
static volatile nrf_drv_twi_evt_type_t xfer_err_code;

bool DAC_init(void) {
    nrf_queue_reset(&m_dac_values_queue);
    xfer_done = TWI_XFER_IS_DONE;

    ret_code_t err = init_twi_blocking();
    if (NRF_SUCCESS == err) {
        // compare and if necessary write DAC settings
        if (true == verify_dac_config()) {
            // after all reinit to nonblocking, since now it will update the DAC as soon as possible
            err = init_twi_nonblocking();
            if (NRF_SUCCESS != err) {
                NRF_LOG_ERROR("Init nonblocking failed, err code %d\n", err);
            }
        } else {
            NRF_LOG_ERROR("Unable to write default configuration to DAC\n");
            return false;
        }
    } else {
        NRF_LOG_ERROR("Init blocking failed, err code %d\n", err);
        return false;
    }
    return true;
}

bool DAC_write_vol_sett_blocking(void) {
    const MCP47x6_settings_t sett = {
        .type = DAC_MCP47x6_TYPE,
        .vref = MCP47x6_VREF_EXT_UNBUFFERED,
        .pwr_down = MCP47x6_PD_OFF,
        .gain = MCP47x6_GAIN_X2 // external reference ~2,495V
    };
    uint8_t tx_data = 0;
    MCP47x6_prepare_write_volatile_cfg(&sett, &tx_data);
    NRF_LOG_DEBUG("tx byte 0x%02X\n", tx_data);
    // send
    ret_code_t err = nrf_drv_twi_tx(&twi, TWI_SLAVE_ADDR, &tx_data, 1, false);
    if (NRF_SUCCESS == err) {
        return true;
    } else {
        NRF_LOG_ERROR("error in %s(), err %d\n", (uint32_t)__func__, err);
    }
    return false;
}

bool DAC_write_vol_dac_blocking(uint16_t dac) {
    const MCP47x6_settings_t sett = {
        .type = DAC_MCP47x6_TYPE,
        .vref = MCP47x6_VREF_EXT_UNBUFFERED,
        .pwr_down = MCP47x6_PD_OFF,
        .gain = MCP47x6_GAIN_X2 // external reference ~2,495V
    };

    uint8_t tx_data[2] = {0};
    MCP47x6_prepare_write_volatile_DAC(&sett, dac, tx_data);
    NRF_LOG_DEBUG("writing [0] : 0x%02X [1] : 0x%02X\n", tx_data[0], tx_data[1]);
    ret_code_t err = nrf_drv_twi_tx(&twi, TWI_SLAVE_ADDR, &tx_data[0], sizeof(tx_data), false);
    if (NRF_SUCCESS == err) {
        return true;
    } else {
        NRF_LOG_ERROR("error in %s(), err %d\n", (uint32_t)__func__, err);
    }
    return false;
}

bool DAC_update_dac(const uint16_t voltage_mv) {
    bool ret_val = false;

    // raw write value to queue
    uint16_t raw = DAC_CONV_MV_TO_RAW(voltage_mv);
    // check if xfer is done fist, if so, then no need to add to queue, just send,
    // but if xfer is not done, then add to queue and then do recheck xfer status to verify if xfer hasn't finished before put to queue
    if (TWI_XFER_IS_DONE == xfer_done) {
        // start transfer
        start_xfer:
        {
            const MCP47x6_settings_t sett = DAC_UPDATE_SETTINGS_INITIALIZER;
            static uint8_t tx_data[2] = {0};
            MCP47x6_prepare_write_volatile_DAC(&sett, raw, &tx_data[0]);
            nrf_drv_twi_xfer_desc_t xfer_desc = NRF_DRV_TWI_XFER_DESC_TX(TWI_SLAVE_ADDR, &tx_data[0], sizeof(tx_data));
            ret_code_t err = nrf_drv_twi_xfer(&twi, &xfer_desc, 0);
            if (NRF_SUCCESS == err) {
                xfer_done = TWI_XFER_IN_PROGRESS;
                ret_val = true;
            } else {
                NRF_LOG_ERROR("error in %s(), err %d\n", (uint32_t)__func__, err);
            }
        }
    } else { // twi is busy right now
        if (NRF_ERROR_NO_MEM == nrf_queue_write(&m_dac_values_queue, &raw, 1)) {
            NRF_LOG_ERROR("%s(): queue is full\n", (uint32_t)__func__);
        } else { // success
            // test if TWI is idle, then start sending
            if (TWI_XFER_IS_DONE == xfer_done) {
                // done, so start transfer, but read from queue
                nrf_queue_read(&m_dac_values_queue, &raw, 1);
                goto start_xfer;
            } else {
                ret_val = true;
            }
        }
    }
    return ret_val;
}


bool DAC_read_blocking(MCP47x6_read_t* read_data) {
    bool ret_val = false;

    uint8_t buff[MCP47_1_2__6_TWI_READ_LEN] = {0};
    ret_code_t err = nrf_drv_twi_rx(&twi, TWI_SLAVE_ADDR, buff, DAC_MCP47x6_READ_LEN);
    if (NRF_SUCCESS == err) {
        MCP47x6_decode_read_data(DAC_MCP47x6_TYPE, buff, DAC_MCP47x6_READ_LEN, read_data);
        ret_val = true;
    } else {
        NRF_LOG_ERROR("Failed read dac data, err %d\n", err);
    }

    return ret_val;
}


static void twi_evt_handler(const nrf_drv_twi_evt_t* p_event, void* p_context) {
    UNUSED_PARAMETER(p_context);
    switch (p_event->type) {
    case NRF_DRV_TWI_EVT_DONE:
        break;
    case NRF_DRV_TWI_EVT_ADDRESS_NACK:
        // fall through
    case NRF_DRV_TWI_EVT_DATA_NACK:
        NRF_LOG_ERROR("(irq): err: %d\n", p_event->type);
        break;
    }

    if (false == nrf_queue_is_empty(&m_dac_values_queue)) {
        uint16_t raw_dac = UINT16_C(0);
        if (NRF_SUCCESS == nrf_queue_read(&m_dac_values_queue, &raw_dac, 1)) {
            const MCP47x6_settings_t sett = DAC_UPDATE_SETTINGS_INITIALIZER;
            static uint8_t tx_data[2] = {0};
            MCP47x6_prepare_write_volatile_DAC(&sett, raw_dac, &tx_data[0]);
            nrf_drv_twi_xfer_desc_t xfer_desc = NRF_DRV_TWI_XFER_DESC_TX(TWI_SLAVE_ADDR, &tx_data[0], sizeof(tx_data));
            ret_code_t err = nrf_drv_twi_xfer(&twi, &xfer_desc, 0);
            if (NRF_SUCCESS != err) {
                NRF_LOG_ERROR("(irq) failed to update DAC, err %d\n", err);
                goto xfer_done;
            }
        } else {
            NRF_LOG_ERROR("(irq) queue inconsistency, reseting queue");
            nrf_queue_reset(&m_dac_values_queue);
            goto xfer_done;
        }
    } else {
        xfer_done:
        xfer_err_code = p_event->type;
        xfer_done = TWI_XFER_IS_DONE;
    }
}

static ret_code_t init_twi_blocking(void) {
    nrf_drv_twi_disable(&twi);
    nrf_drv_twi_uninit(&twi);
    ret_code_t err = nrf_drv_twi_init(&twi, &twi_cfg, NULL, NULL);
    if (NRF_SUCCESS == err) {
        nrf_drv_twi_enable(&twi);
    }
    return err;
}

static ret_code_t init_twi_nonblocking(void) {
    nrf_drv_twi_disable(&twi);
    nrf_drv_twi_uninit(&twi);
    ret_code_t err = nrf_drv_twi_init(&twi, &twi_cfg, twi_evt_handler, NULL);
    if (NRF_SUCCESS == err) {
        nrf_drv_twi_enable(&twi);
    }
    return err;
}

static bool verify_dac_config(void) {
    bool ret_val = false;
    MCP47x6_read_t read_data = {{0}};
    // read current DAC memory, verify if has proper configuration
    for (int i = 0; i < 2 && false == ret_val; ++i) {
        if (true == DAC_read_blocking(&read_data)) {
            // verify non-volatile and volatile settings
            if (false == compare_dac_config(&read_data.nv_sett)) {
                if (false == write_nv_dac_config()) {
                    NRF_LOG_ERROR("Failed to write default config\n");
                }
            } else { // configuration in nv memory is ok
                ret_val = true;
            }
        }
    }

    return ret_val;
}

static bool compare_dac_config(const MCP47x6_settings_t* sett) {
    bool test = true;
    test &= dac_nv_settings.vref == sett->vref;
    test &= dac_nv_settings.gain == sett->gain;
    test &= dac_nv_settings.pwr_down == sett->pwr_down;
    return test;
}

static bool write_nv_dac_config(void) {
    bool ret_val = false;
    uint8_t tx_data[3] = {0};
    MCP47x6_prepare_write_all_mem(&dac_nv_settings, DAC_CONV_MV_TO_RAW(dac_nv_init_value_mv), &tx_data[0]);
    ret_code_t err = nrf_drv_twi_tx(&twi, TWI_SLAVE_ADDR, &tx_data[0], sizeof(tx_data), false);
    if (NRF_SUCCESS == err) {
        // wait for finish
        MCP47x6_read_t read_data = {{0}};
        const uint32_t stp = SYS_TIME_get_tick();
        do {
            nrf_delay_ms(5U);
            if (true == DAC_read_blocking(&read_data)) {
                if (MCP47x6_EEPROM_IN_PROGRAMMING_CYCLE != read_data.nv_rdy) {
                    NRF_LOG_INFO("Written default config to nv memory\n");
                    ret_val = true;
                    break;
                }
            } else {
                NRF_LOG_ERROR("Failed to verify non-volatile write\n");
                break;
            }
        } while(false == SYS_TIME_IS_TIMEOUT(stp, UINT32_C(100)));
    } else {
        NRF_LOG_ERROR("Failed to write default config to non-volatile");
    }
    return ret_val;
}
