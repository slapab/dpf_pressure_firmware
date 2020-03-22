#include "notification_buffers.h"


bool NOTIF_BUFFERS_init(const NOTIF_BUFFERS_init_t* init, NOTIF_BUFFERS_descr_t* descr) {
    bool ret_val = false;
    if (NULL != init && NULL != descr) {
        if (NULL != init->free_buffers_queue && NULL != init->in_use_buffers_queue
                && NULL != init->mem_pool && init->buffers_count > 0) {
            descr->free_buffers_queue = init->free_buffers_queue;
            descr->in_use_buffers_queue = init->in_use_buffers_queue;
            descr->buffers_count = init->buffers_count;
            descr->mem_pool = init->mem_pool;

            ret_val = NOTIF_BUFFERS_reset(descr);
        }
    }
    return ret_val;
}

bool NOTIF_BUFFERS_reset(const NOTIF_BUFFERS_descr_t* descr) {
    // reset queues
    nrf_queue_reset(descr->free_buffers_queue);
    nrf_queue_reset(descr->in_use_buffers_queue);

    // initialize free buffers queue (all buffers are set as free to use)
    for (uint16_t i = 0; i < descr->buffers_count; ++i) {
        if (false == NOTIF_BUFFERS_set_free_buffer(descr, &descr->mem_pool[i][0])) {
            return false; // critical error, shouldn't happen
        }
    }
    return true;
}

bool NOTIF_BUFFERS_get_free_buffer(const NOTIF_BUFFERS_descr_t* descr, uint8_t** buff) {
    bool ret_val = false;
    if (NULL != descr && NULL != buff) {
        ret_val = NRF_SUCCESS == nrf_queue_generic_pop(descr->free_buffers_queue, buff, false);
    }
    return ret_val;
}

bool NOTIF_BUFFERS_set_free_buffer(const NOTIF_BUFFERS_descr_t* descr, const uint8_t* buff) {
    bool ret_val = false;
    if (NULL != descr && NULL != buff) {
        ret_val = NRF_SUCCESS == nrf_queue_push(descr->free_buffers_queue, &buff);
    }
    return ret_val;
}

bool NOTIF_BUFFERS_get_in_use_buffer(const NOTIF_BUFFERS_descr_t* descr, uint8_t** buff) {
    bool ret_val = false;
    if (NULL != descr && NULL != buff) {
        ret_val = NRF_SUCCESS == nrf_queue_generic_pop(descr->in_use_buffers_queue, buff, false);
    }
    return ret_val;
}

bool NOTIF_BUFFERS_set_in_use_buffer(const NOTIF_BUFFERS_descr_t* descr, const uint8_t* buff) {
    bool ret_val = false;
    if (NULL != descr && NULL != buff) {
        ret_val = NRF_SUCCESS == nrf_queue_push(descr->in_use_buffers_queue, &buff);
    }
    return ret_val;
}
