#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

# This makefile is only included if BOARD_WLAN_TI_STA_DK_ROOT is set,
# and if we're not building for the simulator.
ifndef BOARD_WLAN_TI_STA_DK_ROOT
  $(error BOARD_WLAN_TI_STA_DK_ROOT must be defined when including this makefile)
endif
ifeq ($(TARGET_SIMULATOR),true)
  $(error This makefile must not be included when building the simulator)
endif

DK_ROOT = $(BOARD_WLAN_TI_STA_DK_ROOT)
OS_ROOT = $(DK_ROOT)/platforms
STAD	= $(DK_ROOT)/stad
UTILS	= $(DK_ROOT)/utils
TWD	= $(DK_ROOT)/TWD
COMMON  = $(DK_ROOT)/common
TXN	= $(DK_ROOT)/Txn
CUDK	= $(DK_ROOT)/CUDK
LIB	= ../../lib

include external/wpa_supplicant/.config

# To force sizeof(enum) = 4
ifneq ($(TARGET_SIMULATOR),true)
L_CFLAGS += -mabi=aapcs-linux
endif

INCLUDES = $(STAD)/Export_Inc \
	$(STAD)/src/Application \
	$(UTILS) \
	$(OS_ROOT)/os/linux/inc \
	$(OS_ROOT)/os/common/inc \
	$(TWD)/TWDriver \
	$(TWD)/FirmwareApi \
	$(TWD)/TwIf \
	$(TWD)/FW_Transfer/Export_Inc \
	$(TXN) \
	$(CUDK)/configurationutility/inc \
	$(CUDK)/os/common/inc \
	external/openssl/include \
	external/wpa_supplicant \
	$(DK_ROOT)/../lib
  
L_CFLAGS += -DCONFIG_DRIVER_CUSTOM -DHOST_COMPILE -D__BYTE_ORDER_LITTLE_ENDIAN
OBJS = driver_ti.c $(LIB)/scanmerge.c $(LIB)/shlist.c

ifdef CONFIG_NO_STDOUT_DEBUG
L_CFLAGS += -DCONFIG_NO_STDOUT_DEBUG
endif

ifdef CONFIG_DEBUG_FILE
L_CFLAGS += -DCONFIG_DEBUG_FILE
endif

ifdef CONFIG_IEEE8021X_EAPOL
L_CFLAGS += -DIEEE8021X_EAPOL
endif

L_CFLAGS += -DCONFIG_WPS

########################
 
include $(CLEAR_VARS)
LOCAL_MODULE := libCustomWifi
LOCAL_SHARED_LIBRARIES := libc libcutils
LOCAL_CFLAGS := $(L_CFLAGS)
LOCAL_SRC_FILES := $(OBJS)
LOCAL_C_INCLUDES := $(INCLUDES)
include $(BUILD_STATIC_LIBRARY)

########################
