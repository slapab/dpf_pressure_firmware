#ifndef MODULES_COMMON_PING_PONG_BUFFERS_H_
#define MODULES_COMMON_PING_PONG_BUFFERS_H_

#include "nrf_queue.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const nrf_queue_t* free_buffers_queue;
    const nrf_queue_t* in_use_buffers_queue;
    uint8_t item_size;
    uint16_t mempool_rows;
    uint16_t mempool_cols;
    uint8_t (*mempool)[];
} ping_pong_buffs_descr_t;

#define PP_BUFFERS_DEF_QUEUES(item_type, buffers_nbr, free_queue_name, in_use_queue_name) \
        NRF_QUEUE_DEF(item_type, free_queue_name, buffers_nbr, NRF_QUEUE_MODE_NO_OVERFLOW); \
        NRF_QUEUE_DEF(item_type, in_use_queue_name, buffers_nbr, NRF_QUEUE_MODE_NO_OVERFLOW);

/**
 *
 * @param[in] init
 * @return
 */
bool PP_BUFFERS_init(ping_pong_buffs_descr_t* descr);

bool PP_BUFFERS_reset(const ping_pong_buffs_descr_t* descr);

bool PP_BUFFERS_get_free_buffer(const ping_pong_buffs_descr_t* descr, void* ptr_to_dst_buff_addr);
bool PP_BUFFERS_set_free_buffer(const ping_pong_buffs_descr_t* descr, void* buff_addr);
bool PP_BUFFERS_get_in_use_buffer(const ping_pong_buffs_descr_t* descr, void* ptr_to_dst_buff_addr);
bool PP_BUFFERS_set_in_use_buffer(const ping_pong_buffs_descr_t* descr, void* buff_addr);

#endif /* MODULES_COMMON_PING_PONG_BUFFERS_H_ */
