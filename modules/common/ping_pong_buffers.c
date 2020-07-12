#include "ping_pong_buffers.h"


bool PP_BUFFERS_init(ping_pong_buffs_descr_t* descr) {
    bool ret_val = false;
    if (NULL != descr) {
        if (NULL != descr->free_buffers_queue && NULL != descr->in_use_buffers_queue
                && NULL != descr->mempool && descr->mempool_rows > 0 && descr->mempool_cols > 0) {
            ret_val = PP_BUFFERS_reset(descr);
        }
    }
    return ret_val;
}

bool PP_BUFFERS_reset(const ping_pong_buffs_descr_t* descr) {
    // reset queues
    nrf_queue_reset(descr->free_buffers_queue);
    nrf_queue_reset(descr->in_use_buffers_queue);

    // initialize free buffers queue (all buffers are set as free to use)
    uint8_t (*mempool_ptr)[descr->mempool_cols] = descr->mempool;
    for (uint16_t i = 0; i < descr->mempool_rows; ++i) {
        if (false == PP_BUFFERS_set_free_buffer(descr, &mempool_ptr[i * descr->item_size * descr->mempool_cols][0])) {
            return false; // critical error, shouldn't happen
        }
    }
    return true;
}

bool PP_BUFFERS_get_free_buffer(const ping_pong_buffs_descr_t* descr, void* ptr_to_dst_buff_addr) {
    bool ret_val = false;
    if (NULL != descr && NULL != ptr_to_dst_buff_addr) {
        ret_val = NRF_SUCCESS == nrf_queue_generic_pop(descr->free_buffers_queue, ptr_to_dst_buff_addr, false);
    }
    return ret_val;
}

bool PP_BUFFERS_set_free_buffer(const ping_pong_buffs_descr_t* descr, void* buff_addr) {
    bool ret_val = false;
    if (NULL != descr && NULL != buff_addr) {
        ret_val = NRF_SUCCESS == nrf_queue_push(descr->free_buffers_queue, &buff_addr);
    }
    return ret_val;
}

bool PP_BUFFERS_get_in_use_buffer(const ping_pong_buffs_descr_t* descr, void* ptr_to_dst_buff_addr) {
    bool ret_val = false;
    if (NULL != descr && NULL != ptr_to_dst_buff_addr) {
        ret_val = NRF_SUCCESS == nrf_queue_generic_pop(descr->in_use_buffers_queue, ptr_to_dst_buff_addr, false);
    }
    return ret_val;
}

bool PP_BUFFERS_set_in_use_buffer(const ping_pong_buffs_descr_t* descr, void* buff_addr) {
    bool ret_val = false;
    if (NULL != descr && NULL != buff_addr) {
        ret_val = NRF_SUCCESS == nrf_queue_push(descr->in_use_buffers_queue, &buff_addr);
    }
    return ret_val;
}

