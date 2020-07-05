#include "mcp47x6.h"

// Supported commands
// This bits are MSB...MSB-3 bits in 1B word
#define MCP47x6_CMD_WRITE_VOLATILE_DAC_REG 0b00  // third bit doesn't care
#define MCP47x6_CMD_WRITE_VOLATILE_MEM     0b010
#define MCP47x6_CMD_WRITE_ALL_MEM          0b011
#define MCP47x6_CMD_WRITE_VOLATILE_CONF    0b100

#define MCP47x6_CMD_BITS_SHIFT 5

#define GET_BIT(word, shift, mask) (((word) >> (shift)) & (mask))

static void MCP47x6_prepare_write_mem(const uint8_t cmd, const MCP47x6_settings_t* sett, uint16_t data, uint8_t buff[2]);
static inline void MCP4706_decode_read(const uint8_t* raw, const uint8_t len, MCP47x6_read_t* read_data);
static inline void MCP47_1_2_6_decode_read(const uint8_t* raw, const uint8_t len, MCP47x6_read_t* read_data);

void MCP47x6_prepare_write_volatile_DAC(const MCP47x6_settings_t* sett, uint16_t data, uint8_t buff[2]) {
    data &= (uint16_t)sett->type; // type value is a valid data bits mask for given device

    // store command and power down bits
    buff[0] = ((MCP47x6_CMD_WRITE_VOLATILE_DAC_REG << MCP47x6_CMD_BITS_SHIFT) |
            ((uint8_t)sett->pwr_down << 4)) & 0xF0;

    // write DAC value in 1st byte
    switch (sett->type) {
    case MCP4706_8BIT_TYPE:
        // noting in first byte
        buff[1] = (uint8_t)data;
        break;
    case MCP4716_10BIT_TYPE:
        buff[0] |= data >> 6; // save data bits [9:6] at position [3:0]
        buff[1] = (uint8_t)(data << 2); // save data bits [5:0] at position [7:2], two remaining doesn't care
        break;
    case MCP4726_12BIT_TYPE:
        buff[0] |= data >> 8; // save data bits [11:8] at position [3:0]
        buff[1] = (uint8_t)data; // save data bits [8:0] at position [8:0]
        break;
    }
}

void MCP47x6_prepare_write_volatile_mem(const MCP47x6_settings_t* sett, uint16_t data, uint8_t buff[3]){
    MCP47x6_prepare_write_mem(MCP47x6_CMD_WRITE_VOLATILE_MEM, sett, data, buff);
}

void MCP47x6_prepare_write_all_mem(const MCP47x6_settings_t* sett, uint16_t data, uint8_t buff[3]) {
    MCP47x6_prepare_write_mem(MCP47x6_CMD_WRITE_ALL_MEM, sett, data, buff);
}

void MCP47x6_prepare_write_volatile_cfg(const MCP47x6_settings_t* sett, uint8_t* cmd_data) {
    *cmd_data = (MCP47x6_CMD_WRITE_VOLATILE_CONF << MCP47x6_CMD_BITS_SHIFT) |
            (((uint8_t)sett->vref) & 0x03) << 3 |
            (((uint8_t)sett->pwr_down & 0x03) << 1) |
            (sett->gain & 0x01);
}

void MCP47x6_decode_read_data(const MCP47x6_type_e type, const uint8_t* raw, const uint8_t len, MCP47x6_read_t* read_data) {
    switch(type) {
    case MCP4706_8BIT_TYPE:
        read_data->nv_sett.type = MCP4706_8BIT_TYPE;
        read_data->vol_sett.type = MCP4706_8BIT_TYPE;
        MCP4706_decode_read(raw, len, read_data);
        break;
    case MCP4716_10BIT_TYPE:
        read_data->nv_sett.type = MCP4716_10BIT_TYPE;
        read_data->vol_sett.type = MCP4716_10BIT_TYPE;
        // fall through
    case MCP4726_12BIT_TYPE:
        read_data->nv_sett.type = MCP4726_12BIT_TYPE;
        read_data->vol_sett.type = MCP4726_12BIT_TYPE;
        MCP47_1_2_6_decode_read(raw, len, read_data);
        break;
    }
}

static void MCP47x6_prepare_write_mem(const uint8_t cmd, const MCP47x6_settings_t* sett, uint16_t data, uint8_t buff[2]) {
    // store command and power down bits and gain
    buff[0] = (cmd << MCP47x6_CMD_BITS_SHIFT) |
            (((uint8_t)sett->vref) & 0x02) << 3 |
            (((uint8_t)sett->pwr_down & 0x02) << 1) |
            (sett->gain & 0x01);

    data &= (uint16_t)sett->type; // type value is a valid data bits mask for given device
    switch (sett->type) {
    case MCP4706_8BIT_TYPE:
        buff[1] = (uint8_t)data; // save data bits [7:0] at position [7:0]
        // last byte bits doesn't care
        break;
    case MCP4716_10BIT_TYPE:
        buff[1] = (uint8_t)(data >> 2); // save data bits [9:2] at position [7:0]
        buff[2] = (uint8_t)(data << 6); // save data bits [1:0] at position [7:6]
        break;
    case MCP4726_12BIT_TYPE:
        buff[1] = (uint8_t)(data >> 4); // save data bits [11:4] at position [7:0]
        buff[2] = (uint8_t)(data << 4); // save data bits [3:0] at position [7:4]
        break;
    }
}

static inline void MCP4706_decode_read(const uint8_t* raw, const uint8_t len, MCP47x6_read_t* read_data) {
    if (MCP4706_TWI_READ_LEN != len) {
        return;
    }

    //todo
}

static inline void MCP47_1_2_6_decode_read(const uint8_t* raw, const uint8_t len, MCP47x6_read_t* read_data) {
    if (MCP47_1_2__6_TWI_READ_LEN != len) {
        return;
    }

    // decode volatile settings
    read_data->vol_rdy = GET_BIT(raw[0], 7, 0x01);
    read_data->vol_sett.pwr_down = GET_BIT(raw[0], 6, 0x01);
    read_data->vol_sett.vref = GET_BIT(raw[0], 3, 0x03);
    read_data->vol_sett.pwr_down = GET_BIT(raw[0], 1, 0x03);
    read_data->vol_sett.gain = GET_BIT(raw[0], 0, 0x01);

    // decode volatile data
    read_data->vol_data = (uint16_t)(raw[1] << 8) | (uint16_t)(raw[2]);

    // decode non-volatile settings
    read_data->nv_rdy = GET_BIT(raw[3], 7, 0x01);
    read_data->nv_sett.pwr_down = GET_BIT(raw[3], 6, 0x01);
    read_data->nv_sett.vref = GET_BIT(raw[3], 3, 0x03);
    read_data->nv_sett.pwr_down = GET_BIT(raw[3], 1, 0x03);
    read_data->nv_sett.gain = GET_BIT(raw[3], 0, 0x01);

    // decode non-volatile data
    read_data->vol_data = (uint16_t)(raw[4] << 8) | (uint16_t)(raw[5]);

    if (MCP4726_12BIT_TYPE == read_data->nv_sett.type) {
        read_data->vol_data >>= 4;
        read_data->nv_data >>= 4;
    } else if (MCP4716_10BIT_TYPE == read_data->nv_sett.type) {
        read_data->vol_data >>= 6;
        read_data->nv_data >>= 6;
    }
}
