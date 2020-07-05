#ifndef DRIVERS_H_
#define DRIVERS_H_

#include <stdint.h>
#include <stdbool.h>

/// Devices I2C addresses
#define MCP47x6A0_E 0b1100000
#define MCP47x6A0T_E MCP47x6A0_E

#define MCP47x6A1_E 0b1100001
#define MCP47x6A1T_E MCP47x6A1_E

#define MCP47x6A2_E 0b1100010
#define MCP47x6A2T_E MCP47x6A2_E

#define MCP47x6A3_E 0b1100011
#define MCP47x6A3T_E MCP47x6A3_E

#define MCP47x6A4_E 0b1100100
#define MCP47x6A4T_E MCP47x6A4_E

#define MCP47x6A5_E 0b1100101
#define MCP47x6A5T_E MCP47x6A5_E

#define MCP47x6A6_E 0b1100110
#define MCP47x6A6T_E MCP47x6A6_E

#define MCP47x6A7_E 0b1100111
#define MCP47x6A7T_E MCP47x6A7_E

#define MCP47_1_2__6_TWI_READ_LEN 6
#define MCP4706_TWI_READ_LEN 4

#define MCP47x6_EEPROM_IN_PROGRAMMING_CYCLE 0

typedef enum __attribute__((packed)) {
    MCP4706_8BIT_TYPE = 0x00FF,
    MCP4716_10BIT_TYPE = 0x03FF,
    MCP4726_12BIT_TYPE = 0x0FFF
} MCP47x6_type_e;

typedef enum __attribute__((packed)) {
    MCP47x6_PD_OFF = 0,
    MCP47x6_PD_ENTER = 1,
    // TODO add rest of powerdown modes
} MCP47x6_pd_e;

typedef enum __attribute__((packed)) {
    MCP47x6_GAIN_X1 = 0,
    MCP47x6_GAIN_X2 = 1,
} MCP47x6_gain_e;

typedef enum __attribute__((packed)) {
    MCP47x6_VREF_VDD_UNBUFFERED = 0,
    MCP47x6_VREF_EXT_UNBUFFERED = 0x02,
    MCP47x6_VREF_EXT_BUFFERED = 0x03,
} MCP47x6_vref_e;

typedef struct {
    MCP47x6_type_e type;
    MCP47x6_vref_e vref;
    MCP47x6_pd_e pwr_down;
    MCP47x6_gain_e gain;
} MCP47x6_settings_t;

typedef struct {
    MCP47x6_settings_t nv_sett;
    MCP47x6_settings_t vol_sett;
    uint16_t nv_data;
    uint16_t vol_data;
    uint8_t vol_rdy;
    uint8_t nv_rdy;
} MCP47x6_read_t;

void MCP47x6_prepare_write_volatile_DAC(const MCP47x6_settings_t* sett, uint16_t data, uint8_t buff[2]);

void MCP47x6_prepare_write_volatile_mem(const MCP47x6_settings_t* sett, uint16_t data, uint8_t buff[3]);

void MCP47x6_prepare_write_all_mem(const MCP47x6_settings_t* sett, uint16_t data, uint8_t buff[3]);

void MCP47x6_prepare_write_volatile_cfg(const MCP47x6_settings_t* sett, uint8_t* cmd_data);

void MCP47x6_decode_read_data(const MCP47x6_type_e type, const uint8_t* raw, const uint8_t len, MCP47x6_read_t* read_data);

#endif /* DRIVERS_H_ */
