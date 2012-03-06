#
# Linux FM StandAlone Stack & Sample Application
#
# Copyright (c) 2008 Texas Instruments
#

# for cross compilation, make sure CROSS_COMPILE is set, and
# set PREFIX to your usr dir in your target fs

#installation directory
#will look for headers in $(PREFIX)/include and install stack in $(PREFIX)/lib
#ROOTFS = /
#ROOTFS=/home/ohad/embedded/18.12/rootfs/
#ROOTFS=/home/ohad/embedded/23.8/rootfs/
ROOTFS=$(BT_HOME)/
PREFIX = $(ROOTFS)usr
LIBPLACE=$(ROOTFS)lib
BLUEZ_HEADERS = $(PREFIX)/include
BLUEZ_LIB = $(PREFIX)/lib
INSTALL_BIN = $(PREFIX)/local/bin

CC	= $(CROSS_COMPILE)gcc
AR	= $(CROSS_COMPILE)ar

INC_FLAGS = \
-I ./MCP_Common/Platform/fmhal/inc/int \
-I ./MCP_Common/Platform/fmhal/inc \
-I ./MCP_Common/Platform/os/Linux \
-I ./MCP_Common/Platform/inc \
-I ./MCP_Common/tran \
-I ./MCP_Common/inc \
-I ./HSW_FMStack/stack/inc/int \
-I ./HSW_FMStack/stack/inc \
-I $(BLUEZ_HEADERS) \
-I ./fm_app


CFLAGS	= -g -c -W -Wall -O2 $(INC_FLAGS)

STACK_SOURCES = mcp_hal_pm.c fmc_os.c mcp_hal_fs.c mcp_win_unicode.c mcp_hal_log.c mcp_hal_memory.c mcp_hal_string.c mcp_win_line_parser.c mcp_hal_os.c mcp_hal_misc.c mcp_hci_sequencer.c mcp_pool.c mcpf_queue.c mcp_endian.c mcpf_main.c mcp_config_reader.c mcp_utils_dl_list.c mcp_rom_scripts_db.c mcp_bts_script_processor.c mcpf_report.c mcp_load_manager.c mcp_gensm.c mcp_config_parser.c mcp_rom_scripts.c fmc_debug.c fmc_common.c fmc_pool.c fmc_utils.c fmc_core.c fm_rx.c fm_rx_sm.c fm_tx_sm.c fm_tx.c fm_trace.c ccm_hal_pwr_up_dwn.c ccm_vac.c ccm.c ccm_imi_bt_tran_sm.c ccm_imi_bt_tran_mngr.c ccm_im.c ccm_imi_bt_tran_off_sm.c ccm_imi_bt_tran_on_sm.c ccm_vaci_chip_abstration.c bt_hci_if.c

#CCM_FILES: ccm_hal_pwr_up_dwn.c cm_vaci_configuration_engine.c ccm_vac.c ccm_vaci_debug.c cm_vaci_allocation_engine.c cm_vaci_mapping_engine.c ccm.c cm_vaci_cal_chip_6350.c cm_vaci_cal_chip_6450_1_0.c cm_vaci_chip_abstration.c cm_vaci_cal_chip_1273.c ccm_imi_bt_tran_sm.c ccm_imi_bt_tran_mngr.c ccm_im.c ccm_imi_bt_tran_off_sm.c ccm_imi_bt_tran_on_sm.c 

ALL_SOURCES = $(STACK_SOURCES) fm_app.c
STACK_OBJECTS = $(subst .c,.o,$(STACK_SOURCES))

SRC_FOLDERS = ./MCP_Common/Platform/hw/linux ./MCP_Common/Platform/fmhal/os ./MCP_Common/Platform/os/linux ./MCP_Common/frame ./MCP_Common/ccm/vac ./MCP_Common/ccm/ccm ./MCP_Common/ccm/cal ./MCP_Common/ccm/im ./MCP_Common/ccm/im/test ./MCP_Common/init_script ./HSW_FMStack/stack/common ./HSW_FMStack/stack/rx ./HSW_FMStack/stack/tx ./fm_app


vpath %.c $(SRC_FOLDERS)

.PHONY: all
all: fmapp

stack: libfm.a

fmapp: libfm.a

fmapp: fm_app.o
	$(CC) $^ -o $@ -L $(BLUEZ_LIB) -L $(LIBPLACE) -lbluetooth -lm -lpthread -lrt
	
libfm.a: $(STACK_OBJECTS)
	$(AR) $(ARFLAGS) $@ $?

.PHONY: clean
clean:
	rm -f fmapp libfm.a *.o

.PHONY: install
install:
	cp fmapp $(INSTALL_BIN)

install-strip:
	arm-linux-strip fmapp; cp fmapp $(INSTALL_BIN)
