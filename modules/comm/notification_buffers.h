#ifndef MODULES_COMM_NOTIFICATION_BUFFERS_H_
#define MODULES_COMM_NOTIFICATION_BUFFERS_H_

#include <stdint.h>
#include <stdbool.h>
#include "nrf_queue.h"


/// Maximum length of one buffer using for transmitting data using BLE NOTIFICATION
#define NOTIF_BUFFERS_ONE_BUFFER_LEN 20U
/// Type of element that must used for creating NRF QUEUE
typedef uint8_t* NOTIF_BUFFERS_buffer_type_t;

typedef struct {
    const nrf_queue_t* free_buffers_queue;
    const nrf_queue_t* in_use_buffers_queue;
    uint16_t buffers_count;
    uint8_t (*mem_pool)[NOTIF_BUFFERS_ONE_BUFFER_LEN];
} NOTIF_BUFFERS_descr_t;

typedef NOTIF_BUFFERS_descr_t NOTIF_BUFFERS_init_t;

#define NOTIF_BUFFERS_DEF_QUEUES(buffers_nbr, free_queue_name, in_use_queue_name) \
        NRF_QUEUE_DEF(NOTIF_BUFFERS_buffer_type_t, free_queue_name, buffers_nbr, NRF_QUEUE_MODE_NO_OVERFLOW); \
        NRF_QUEUE_DEF(NOTIF_BUFFERS_buffer_type_t, in_use_queue_name, buffers_nbr, NRF_QUEUE_MODE_NO_OVERFLOW);

/**
 *
 * @param[in] init
 * @param[in,out] descr
 * @return
 */
bool NOTIF_BUFFERS_init(const NOTIF_BUFFERS_init_t* init, NOTIF_BUFFERS_descr_t* descr);

bool NOTIF_BUFFERS_reset(const NOTIF_BUFFERS_descr_t* descr);

bool NOTIF_BUFFERS_get_free_buffer(const NOTIF_BUFFERS_descr_t* descr, uint8_t** buff);
bool NOTIF_BUFFERS_set_free_buffer(const NOTIF_BUFFERS_descr_t* descr, const uint8_t* buff);
bool NOTIF_BUFFERS_get_in_use_buffer(const NOTIF_BUFFERS_descr_t* descr, uint8_t** buff);
bool NOTIF_BUFFERS_set_in_use_buffer(const NOTIF_BUFFERS_descr_t* descr, const uint8_t* buff);



#endif /* MODULES_COMM_NOTIFICATION_BUFFERS_H_ */
