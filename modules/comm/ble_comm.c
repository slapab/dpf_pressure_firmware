#include "ble_comm.h"
#include "ble_stack.h"
#include "notification_buffers.h"
#include "nrf_queue.h"
#include "ble_nus.h"
#include "signal.h"

#include "log_config.h"
#define NRF_LOG_MODULE_NAME "ble-comm"
#define NRF_LOG_LEVEL LOG_CFG_BLE_COMM_LOG_LEVEL
#include "nrf_log.h"


extern ble_nus_t m_nus;                                      /**< Structure to identify the Nordic UART Service. */
static volatile sig_atomic_t nus_notificatoin_enabled;


#define BLE_COMM_NBR_OF_TX_NOTIFICATION_BUFFERS 10U
static uint8_t notif_buffers_mem_pool[BLE_COMM_NBR_OF_TX_NOTIFICATION_BUFFERS][NOTIF_BUFFERS_ONE_BUFFER_LEN];
NOTIF_BUFFERS_DEF_QUEUES(BLE_COMM_NBR_OF_TX_NOTIFICATION_BUFFERS, m_notif_free_buffs_queue, m_notif_in_use_buffs_queue);
static NOTIF_BUFFERS_descr_t notif_buffers_descr;

#define BLE_COMM_TX_QUEUE_LEN 256U
NRF_QUEUE_DEF(uint8_t, m_ble_comm_tx_queue, BLE_COMM_TX_QUEUE_LEN, NRF_QUEUE_MODE_NO_OVERFLOW);

static struct {
    uint8_t* buff_ptr;
    uint16_t len;
} ble_comm_pending_tx_buff;


bool BLE_COMM_init(void) {
    nrf_queue_reset(&m_ble_comm_tx_queue);
    ble_comm_pending_tx_buff.buff_ptr = NULL;
    const NOTIF_BUFFERS_init_t init = {
            .free_buffers_queue = &m_notif_free_buffs_queue,
            .in_use_buffers_queue = &m_notif_in_use_buffs_queue,
            .buffers_count = BLE_COMM_NBR_OF_TX_NOTIFICATION_BUFFERS,
            .mem_pool = notif_buffers_mem_pool
    };
    return NOTIF_BUFFERS_init(&init, &notif_buffers_descr);
}

bool BLE_COMM_is_comm_enabled(void) {
    return true == BLE_STACK_is_connected() && true == nus_notificatoin_enabled;
}

uint32_t BLE_COMM_send_data(const void* data, uint16_t len) {
    uint32_t err_code = NRF_SUCCESS;
    if (true == nus_notificatoin_enabled) {
        err_code = nrf_queue_write(&m_ble_comm_tx_queue, data, len);
        if (NRF_SUCCESS != err_code) {
            NRF_LOG_DEBUG("%s (): no free space\n", (uint32_t)__func__);
        }
    } else {
        err_code = NRF_ERROR_INVALID_STATE;
    }
    return err_code;
}

void BLE_COMM_tx_process(void) {
    if (true == nus_notificatoin_enabled) {

        // first, check if there is any pending buffer
        if (NULL != ble_comm_pending_tx_buff.buff_ptr) {
            // try to send pending buffer first
            ret_code_t err = ble_nus_string_send(&m_nus, ble_comm_pending_tx_buff.buff_ptr, ble_comm_pending_tx_buff.len);
            if (NRF_SUCCESS == err) {
                // ok, buffer has been scheduled to SoftDevice
                NOTIF_BUFFERS_set_in_use_buffer(&notif_buffers_descr, ble_comm_pending_tx_buff.buff_ptr);
                // clear pending buffer because it has been handled
                ble_comm_pending_tx_buff.buff_ptr = NULL;
            } else { // pending buffer remains pending, exiting right now, because pending buffer must be send first
                return;
            }
        }


        size_t read_len = nrf_queue_utilization_get(&m_ble_comm_tx_queue);
        if (read_len > 0U) {
            // try to get free buffer for tx via notifications
            uint8_t* buff_ptr = NULL;
            if (true == NOTIF_BUFFERS_get_free_buffer(&notif_buffers_descr, &buff_ptr) &&
                    NULL != buff_ptr) {
                // read data from queue
                if (read_len > NOTIF_BUFFERS_ONE_BUFFER_LEN) {
                    read_len = NOTIF_BUFFERS_ONE_BUFFER_LEN;
                }
                ret_code_t err = nrf_queue_read(&m_ble_comm_tx_queue, buff_ptr, read_len);
                if (NRF_SUCCESS != err) {
                    NRF_LOG_ERROR("%s() failed to read from queue, err %d \n", (uint32_t)__func__, err);
                    // give back notif. buffer
                    NOTIF_BUFFERS_set_free_buffer(&notif_buffers_descr, buff_ptr);
                    return;
                }
                // try to send data over BLE
                err = ble_nus_string_send(&m_nus, buff_ptr, (uint16_t)read_len);
                if (NRF_SUCCESS == err) { //  ok, buffer has been scheduled, set it as in use
                    if (false == NOTIF_BUFFERS_set_in_use_buffer(&notif_buffers_descr, buff_ptr)) {
                        NRF_LOG_ERROR("%s() failed to set in-use\n", (uint32_t)__func__);
                    }
                } else {
                    // set given buffer as pending, because data already has been read from queue, and can't be put put back
                    // at the same position. So need to set as pending, and in the next try this pending buffer will be tried
                    // to send as first.
                    ble_comm_pending_tx_buff.buff_ptr = buff_ptr;
                    ble_comm_pending_tx_buff.len = read_len;
                }
            } else {
                NRF_LOG_DEBUG("%s(): no free notif. buffers\n", (uint32_t)__func__);
            }
        }
    }

}

void BLE_COMM_nus_data_handler(const ble_nus_evt_t* evt) {
    switch (evt->type) {
    case BLE_NUS_EVT_TYPE_RX: {
        NRF_LOG_DEBUG("Got data len %u\r\n", evt->rx_data.len);
//        const char* txt = "Test 1234";
//        BLE_COMM_send_data(txt, strlen(txt));
    }   break;
    case BLE_NUS_EVT_TYPE_TX_COMPLETE: {
        uint8_t* buff_ptr = NULL;
        //  get in-use buffer, and set it as free buffer
        if (true == NOTIF_BUFFERS_get_in_use_buffer(&notif_buffers_descr, &buff_ptr) &&
                NULL != buff_ptr) {
            NOTIF_BUFFERS_set_free_buffer(&notif_buffers_descr, buff_ptr);
        }
        NRF_LOG_DEBUG("HVX COMPLETE\r\n");
    }   break;
    case BLE_NUS_EVT_TYPE_NOTIFICATION_ON:
        nus_notificatoin_enabled = true;
        nrf_queue_reset(&m_ble_comm_tx_queue);
        NOTIF_BUFFERS_reset(&notif_buffers_descr);
        ble_comm_pending_tx_buff.buff_ptr = NULL;
        NRF_LOG_DEBUG("NUS NOTIF ON %u\r\n");
        break;
    case BLE_NUS_EVT_TYPE_NOTIFICATION_OFF:
        nus_notificatoin_enabled = false;
        NRF_LOG_DEBUG("NUS NOTIF OFF %u\r\n");
        break;
    }
}
