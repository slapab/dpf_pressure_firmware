#ifndef MODULES_COMM_BLE_COMM_H_
#define MODULES_COMM_BLE_COMM_H_

#include "ble_nus.h"
#include <stdint.h>
#include <stdbool.h>


bool BLE_COMM_init(void);
bool BLE_COMM_is_comm_enabled(void);

void BLE_COMM_nus_data_handler(const ble_nus_evt_t* evt);

uint32_t BLE_COMM_send_data(const void* data, uint16_t len);

void BLE_COMM_tx_process(void);

#endif /* MODULES_COMM_BLE_COMM_H_ */
