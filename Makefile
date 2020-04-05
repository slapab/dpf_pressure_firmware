PROJECT_NAME     := ble_app_s130
TARGETS          := nrf51822_xxaa
OUTPUT_DIRECTORY := _build
# a DFU settings hex file path with name
APP_SETTINGS_HEX := $(OUTPUT_DIRECTORY)/app_settings.hex


NRFUTIL_TOOL := nrfutil
MERGEHEX_TOOL := /opt/mergehex/mergehex

OPENOCD_PATH := /opt/openocd/0.10.0-201610281609-dev/bin/
OPENOCD_CMD_COMMON := $(OPENOCD_PATH)/openocd -f interface/ftdi/jtag-lock-pick_tiny_2.cfg -c "transport select swd;" -c "set WORKAREASIZE 0;" -f "target/nrf51.cfg" -c "adapter_khz 1000;"
STDERR_TO_STDOUT := 2>&1

SDK_ROOT := ./nRF5_SDK_12.3.0_d7731ad
PROJ_DIR := .
DFU_DIR := $(PROJ_DIR)/dfu

# [in] path to the bootloader hex
BOOTLOADER_HEX := ./hex_files/nrf51822_xxaa_s130_bootloader.hex
# [in] path to the softdevice hex
SOFTDEVICE_HEX := $(SDK_ROOT)/components/softdevice/s130/hex/s130_nrf51_2.0.1_softdevice.hex


$(OUTPUT_DIRECTORY)/nrf51822_xxaa.out: \
  LINKER_SCRIPT  := ble_app_gcc_nrf51.ld

# List of supported BSPs
# 10 - v1_0
# Select compilation BSP version (valid values are from suported BSP list) 
BSP_USE_VER := 10
ifeq ($(BSP_USE_VER),10)
	BSP_DIR := $(PROJ_DIR)/bsp/v1_0
else
	$(error Please selct valid BSP version!)
endif

# Source files common to all targets
SRC_FILES += \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_backend_serial.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_frontend.c \
  $(SDK_ROOT)/components/libraries/button/app_button.c \
  $(SDK_ROOT)/components/libraries/util/app_error.c \
  $(SDK_ROOT)/components/libraries/util/app_error_weak.c \
  $(SDK_ROOT)/components/libraries/fifo/app_fifo.c \
  $(SDK_ROOT)/components/libraries/timer/app_timer.c \
  $(SDK_ROOT)/components/libraries/uart/app_uart_fifo.c \
  $(SDK_ROOT)/components/libraries/util/app_util_platform.c \
  $(SDK_ROOT)/components/libraries/hardfault/hardfault_implementation.c \
  $(SDK_ROOT)/components/libraries/hardfault/nrf51/handler/hardfault_handler_gcc.c \
  $(SDK_ROOT)/components/libraries/util/nrf_assert.c \
  $(SDK_ROOT)/components/libraries/uart/retarget.c \
  $(SDK_ROOT)/components/libraries/util/sdk_errors.c \
  $(SDK_ROOT)/components/libraries/queue/nrf_queue.c \
  $(SDK_ROOT)/components/drivers_nrf/clock/nrf_drv_clock.c \
  $(SDK_ROOT)/components/drivers_nrf/common/nrf_drv_common.c \
  $(SDK_ROOT)/components/drivers_nrf/timer/nrf_drv_timer.c \
  $(SDK_ROOT)/components/drivers_nrf/gpiote/nrf_drv_gpiote.c \
  $(SDK_ROOT)/components/drivers_nrf/uart/nrf_drv_uart.c \
  $(SDK_ROOT)/components/drivers_nrf/adc/nrf_drv_adc.c \
  $(SDK_ROOT)/components/drivers_nrf/ppi/nrf_drv_ppi.c \
  $(SDK_ROOT)/external/segger_rtt/RTT_Syscalls_GCC.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT_printf.c \
  $(SDK_ROOT)/components/ble/common/ble_advdata.c \
  $(SDK_ROOT)/components/ble/common/ble_conn_params.c \
  $(SDK_ROOT)/components/ble/ble_db_discovery/ble_db_discovery.c \
  $(SDK_ROOT)/components/ble/common/ble_srv_common.c \
  $(SDK_ROOT)/components/toolchain/gcc/gcc_startup_nrf51.S \
  $(SDK_ROOT)/components/toolchain/system_nrf51.c \
  $(SDK_ROOT)/components/softdevice/common/softdevice_handler/softdevice_handler.c \
  $(SDK_ROOT)/components/libraries/scheduler/app_scheduler.c \
  $(SDK_ROOT)/components/ble/common/ble_advdata.c \
  $(SDK_ROOT)/components/ble/ble_advertising/ble_advertising.c \
  $(SDK_ROOT)/components/libraries/fstorage/fstorage.c \
  $(PROJ_DIR)/modules/modified_ble_nus/ble_nus.c \
  $(PROJ_DIR)/modules/comm/ble_comm.c \
  $(PROJ_DIR)/modules/comm/notification_buffers.c \
  $(PROJ_DIR)/modules/ble_stack/ble_stack.c \
  $(PROJ_DIR)/modules/sys_time/sys_time.c \
  $(PROJ_DIR)/modules/adc/adc_module.c \
  $(PROJ_DIR)/modules/tim_sharing/tim1_sharing.c \
  $(PROJ_DIR)/modules/app_logic/app_logic.c \
  $(PROJ_DIR)/drivers/mcp9700t/mcp9700t.c \
  $(PROJ_DIR)/main.c \
  $(BSP_DIR)/bsp.c \

#  $(PROJ_DIR)/remote_control.c \



# Include folders common to all targets
INC_FOLDERS += \
  $(SDK_ROOT)/components/drivers_nrf/comp \
  $(SDK_ROOT)/components/drivers_nrf/twi_master \
  $(SDK_ROOT)/components/ble/ble_services/ble_ancs_c \
  $(SDK_ROOT)/components/ble/ble_services/ble_ias_c \
  $(SDK_ROOT)/components/softdevice/s130/headers \
  $(SDK_ROOT)/components/libraries/pwm \
  $(SDK_ROOT)/components/libraries/usbd/class/cdc/acm \
  $(SDK_ROOT)/components/libraries/usbd/class/hid/generic \
  $(SDK_ROOT)/components/libraries/usbd/class/msc \
  $(SDK_ROOT)/components/libraries/usbd/class/hid \
  $(SDK_ROOT)/components/libraries/log \
  $(SDK_ROOT)/components/ble/ble_services/ble_gls \
  $(SDK_ROOT)/components/libraries/fstorage \
  $(SDK_ROOT)/components/drivers_nrf/i2s \
  $(SDK_ROOT)/components/libraries/gpiote \
  $(SDK_ROOT)/components/drivers_nrf/gpiote \
  $(SDK_ROOT)/components/libraries/fifo \
  $(SDK_ROOT)/components/drivers_nrf/common \
  $(SDK_ROOT)/components/ble/ble_advertising \
  $(SDK_ROOT)/components/drivers_nrf/adc \
  $(SDK_ROOT)/components/softdevice/s130/headers/nrf51 \
  $(SDK_ROOT)/components/ble/ble_services/ble_bas_c \
  $(SDK_ROOT)/components/ble/ble_services/ble_hrs_c \
  $(SDK_ROOT)/components/libraries/queue \
  $(SDK_ROOT)/components/ble/ble_dtm \
  $(SDK_ROOT)/components/toolchain/cmsis/include \
  $(SDK_ROOT)/components/ble/ble_services/ble_rscs_c \
  $(SDK_ROOT)/components/drivers_nrf/uart \
  $(SDK_ROOT)/components/ble/common \
  $(SDK_ROOT)/components/ble/ble_services/ble_lls \
  $(SDK_ROOT)/components/drivers_nrf/wdt \
  $(SDK_ROOT)/components/ble/ble_db_discovery \
  $(SDK_ROOT)/components/ble/ble_services/ble_bas \
  $(SDK_ROOT)/components/libraries/experimental_section_vars \
  $(SDK_ROOT)/components/ble/ble_services/ble_ans_c \
  $(SDK_ROOT)/components/libraries/slip \
  $(SDK_ROOT)/components/libraries/mem_manager \
  $(SDK_ROOT)/external/segger_rtt \
  $(SDK_ROOT)/components/libraries/usbd/class/cdc \
  $(SDK_ROOT)/components/drivers_nrf/hal \
  $(SDK_ROOT)/components/drivers_nrf/rtc \
  $(SDK_ROOT)/components/ble/ble_services/ble_ias \
  $(SDK_ROOT)/components/libraries/usbd/class/hid/mouse \
  $(SDK_ROOT)/components/drivers_nrf/ppi \
  $(SDK_ROOT)/components/ble/ble_services/ble_dfu \
  $(SDK_ROOT)/components/drivers_nrf/twis_slave \
  $(SDK_ROOT)/components \
  $(SDK_ROOT)/components/libraries/scheduler \
  $(SDK_ROOT)/components/ble/ble_services/ble_lbs \
  $(SDK_ROOT)/components/ble/ble_services/ble_hts \
  $(SDK_ROOT)/components/drivers_nrf/delay \
  $(SDK_ROOT)/components/libraries/crc16 \
  $(SDK_ROOT)/components/drivers_nrf/timer \
  $(SDK_ROOT)/components/libraries/util \
  $(SDK_ROOT)/components/drivers_nrf/pwm \
  $(SDK_ROOT)/components/libraries/csense_drv \
  $(SDK_ROOT)/components/libraries/csense \
  $(SDK_ROOT)/components/drivers_nrf/rng \
  $(SDK_ROOT)/components/libraries/low_power_pwm \
  $(SDK_ROOT)/components/libraries/hardfault \
  $(SDK_ROOT)/components/ble/ble_services/ble_cscs \
  $(SDK_ROOT)/components/libraries/uart \
  $(SDK_ROOT)/components/libraries/hci \
  $(SDK_ROOT)/components/libraries/usbd/class/hid/kbd \
  $(SDK_ROOT)/components/drivers_nrf/spi_slave \
  $(SDK_ROOT)/components/drivers_nrf/lpcomp \
  $(SDK_ROOT)/components/libraries/timer \
  $(SDK_ROOT)/components/drivers_nrf/power \
  $(SDK_ROOT)/components/libraries/usbd/config \
  $(SDK_ROOT)/components/toolchain \
  $(SDK_ROOT)/components/libraries/led_softblink \
  $(SDK_ROOT)/components/drivers_nrf/qdec \
  $(SDK_ROOT)/components/ble/ble_services/ble_cts_c \
  $(SDK_ROOT)/components/drivers_nrf/spi_master \
  $(SDK_ROOT)/components/ble/ble_services/ble_hids \
  $(SDK_ROOT)/components/drivers_nrf/pdm \
  $(SDK_ROOT)/components/libraries/crc32 \
  $(SDK_ROOT)/components/libraries/usbd/class/audio \
  $(SDK_ROOT)/components/ble/peer_manager \
  $(SDK_ROOT)/components/drivers_nrf/swi \
  $(SDK_ROOT)/components/ble/ble_services/ble_tps \
  $(SDK_ROOT)/components/ble/ble_services/ble_dis \
  $(SDK_ROOT)/components/device \
  $(SDK_ROOT)/components/ble/nrf_ble_qwr \
  $(SDK_ROOT)/components/libraries/button \
  $(SDK_ROOT)/components/libraries/usbd \
  $(SDK_ROOT)/components/ble/ble_services/ble_lbs_c \
  $(SDK_ROOT)/components/ble/ble_racp \
  $(SDK_ROOT)/components/toolchain/gcc \
  $(SDK_ROOT)/components/libraries/fds \
  $(SDK_ROOT)/components/libraries/twi \
  $(SDK_ROOT)/components/drivers_nrf/clock \
  $(SDK_ROOT)/components/ble/ble_services/ble_rscs \
  $(SDK_ROOT)/components/drivers_nrf/usbd \
  $(SDK_ROOT)/components/softdevice/common/softdevice_handler \
  $(SDK_ROOT)/components/ble/ble_services/ble_hrs \
  $(SDK_ROOT)/components/libraries/log/src \
  $(SDK_ROOT)/components/boards \
  $(SDK_ROOT)/external/protothreads \
  $(SDK_ROOT)/external/protothreads/pt-1.4 \
  $(PROJ_DIR)/config \
  $(PROJ_DIR)/ \
  $(PROJ_DIR)/bsp \
  $(PROJ_DIR)/modules/modified_ble_nus \
  $(PROJ_DIR)/modules/comm \
  $(PROJ_DIR)/modules/ble_stack \
  $(PROJ_DIR)/modules/sys_time \
  $(PROJ_DIR)/modules/adc \
  $(PROJ_DIR)/modules/tim_sharing \
  $(PROJ_DIR)/modules/app_logic \
  $(PROJ_DIR)/drivers/mcp9700t \
  $(BSP_DIR) \
  


# Libraries common to all targets
LIB_FILES += \

# C flags common to all targets
CFLAGS += -DBOARD_CUSTOM
CFLAGS += -DSOFTDEVICE_PRESENT
CFLAGS += -DNRF51
CFLAGS += -D__HEAP_SIZE=0
CFLAGS += -DS130
CFLAGS += -DBLE_STACK_SUPPORT_REQD
CFLAGS += -DSWI_DISABLE0
CFLAGS += -DBSP_UART_SUPPORT
CFLAGS += -DNRF51822
CFLAGS += -DNRF_SD_BLE_API_VERSION=2
CFLAGS += -DAPP_SCHEDULER_ENABLED=1
CFLAGS += -mcpu=cortex-m0
CFLAGS += -mthumb -mabi=aapcs
CFLAGS +=  -Wall -O3 -g3 # -Werror
CFLAGS += -mfloat-abi=soft
# keep every function in separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin --short-enums

# C++ flags common to all targets
CXXFLAGS += \

# Assembler flags common to all targets
ASMFLAGS += -x assembler-with-cpp
ASMFLAGS += -DBOARD_CUSTOM
ASMFLAGS += -DSOFTDEVICE_PRESENT
ASMFLAGS += -DNRF51
ASMFLAGS += -D__HEAP_SIZE=0
ASMFLAGS += -DS130
ASMFLAGS += -DBLE_STACK_SUPPORT_REQD
ASMFLAGS += -DSWI_DISABLE0
ASMFLAGS += -DBSP_UART_SUPPORT
ASMFLAGS += -DNRF51822
ASMFLAGS += -DNRF_SD_BLE_API_VERSION=2
ASMFLAGS += -DAPP_SCHEDULER_ENABLED=1

# Linker flags
LDFLAGS += -mthumb -mabi=aapcs -L $(TEMPLATE_PATH) -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m0
# let linker to dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs -lc -lnosys


.PHONY: $(TARGETS) default all clean help flash flash_softdevice

# Default target - first one defined
default: nrf51822_xxaa

# Print all targets that can be built
help:
	@echo following targets are available:
	@echo 	nrf51822_xxaa

TEMPLATE_PATH := $(SDK_ROOT)/components/toolchain/gcc

include $(TEMPLATE_PATH)/Makefile.common

$(foreach target, $(TARGETS), $(call define_target, $(target)))

# Flash the program
flash: $(OUTPUT_DIRECTORY)/$(TARGETS).hex
	@echo Flashing: $<
#	$(OPENOCD_CMD_COMMON) -c "program $< verify reset; reset run; exit;" $(STDERR_TO_STDOUT)
	nrfjprog --program $< -f nrf51 --sectorerase --reset
#	nrfjprog --reset -f nrf51

# Flash softdevice
flash_softdevice:
	@echo "Flashing: $(SOFTDEVICE_HEX)"
#	$(OPENOCD_CMD_COMMON) -c "init; reset halt;" -c "program $(SOFTDEVICE_HEX) verify reset; reset run; exit;" $(STDERR_TO_STDOUT)
	nrfjprog --program $(SOFTDEVICE_HEX) -f nrf51 --sectorerase
	nrfjprog --reset -f nrf51

erase:
	@echo Erasing
#	$(OPENOCD_CMD_COMMON) -c "init; reset; halt; nrf51 mass_erase; reset halt; exit;" $(STDERR_TO_STDOUT)
	nrfjprog --eraseall -f nrf51

reset:
	@echo Resetting
#	$(OPENOCD_CMD_COMMON) -c "init; reset run; shutdown;" $(STDERR_TO_STDOUT)
	#$(OPENOCD_PATH)/openocd -f interface/ftdi/jtag-lock-pick_tiny_2.cfg -c "transport select swd;" -c "set WORKAREASIZE 0;" -f "target/nrf51.cfg" -c "adapter_khz 1000;" -c "init; reset run; shutdown;"
	nrfjprog --reset -f nrf51


generate_dfu_settings: $(TARGETS)
	@echo Generating DFU settings
	$(NRFUTIL_TOOL) settings generate --family NRF51 --application $(OUTPUT_DIRECTORY)/$(TARGETS).hex --application-version 0 --bootloader-version 0 --bl-settings-version 1 $(APP_SETTINGS_HEX)

merge: generate_dfu_settings
	@echo Merging to $(OUTPUT_DIRECTORY)/merged_app_dfu.hex from:
	@echo	$(SOFTDEVICE_HEX)
	@echo	$(OUTPUT_DIRECTORY)/$(TARGETS).hex
	@echo	$(BOOTLOADER_HEX)
	@echo	$(APP_SETTINGS_HEX)
	$(MERGEHEX_TOOL) -m $(SOFTDEVICE_HEX) $(OUTPUT_DIRECTORY)/$(TARGETS).hex $(BOOTLOADER_HEX) -o $(OUTPUT_DIRECTORY)/temp.hex
	$(MERGEHEX_TOOL) -m $(OUTPUT_DIRECTORY)/temp.hex $(APP_SETTINGS_HEX) -o $(OUTPUT_DIRECTORY)/merged_app_dfu.hex

flash_app_as_dfu: generate_dfu_settings
	@echo Flashing APP as DFU
	# merge app with dfu settings
	$(MERGEHEX_TOOL) -m $(APP_SETTINGS_HEX) $(OUTPUT_DIRECTORY)/$(TARGETS).hex -o $(OUTPUT_DIRECTORY)/temp.hex
	# flash 
	nrfjprog --program $(OUTPUT_DIRECTORY)/temp.hex -f nrf51 --sectorerase --reset

flash_image: erase merge
	@echo flashing image
#	$(OPENOCD_CMD_COMMON) -c "program $(OUTPUT_DIRECTORY)/merged_app_dfu.hex verify reset; reset run; exit;" $(STDERR_TO_STDOUT)
	nrfjprog --program $(OUTPUT_DIRECTORY)/merged_app_dfu.hex --sectorerase -f nrf51 --reset


gen_dfu_zip:
	@echo Generating DFU zip from file:
	@ls -l $(OUTPUT_DIRECTORY)/$(TARGETS).hex
	@echo
	$(NRFUTIL_TOOL) pkg generate --sd-req 0x87 --hw-version 51 --application-version 1 --application $(OUTPUT_DIRECTORY)/$(TARGETS).hex --key-file $(DFU_DIR)/private.key $(DFU_DIR)/app_dfu.zip
