LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_SIMULATOR),true)
  $(error This makefile must not be included when building the simulator)
endif

ifneq ($(WPA_SUPPLICANT_VERSION),VER_0_6_X)
  $(error wpa_supplicant_6 is required for this driver)
endif

WPA_SUPPL_DIR = external/wpa_supplicant_6/wpa_supplicant

include $(WPA_SUPPL_DIR)/.config

L_CFLAGS = -DCONFIG_DRIVER_CUSTOM -DWPA_SUPPLICANT_$(WPA_SUPPLICANT_VERSION)

ifdef CONFIG_NO_STDOUT_DEBUG
L_CFLAGS += -DCONFIG_NO_STDOUT_DEBUG
endif

ifdef CONFIG_DEBUG_FILE
L_CFLAGS += -DCONFIG_DEBUG_FILE
endif

ifdef CONFIG_ANDROID_LOG
L_CFLAGS += -DCONFIG_ANDROID_LOG
endif

ifdef CONFIG_IEEE8021X_EAPOL
L_CFLAGS += -DIEEE8021X_EAPOL
endif

ifdef CONFIG_WPS
L_CFLAGS += -DCONFIG_WPS
endif

INCLUDES = $(WPA_SUPPL_DIR) \
    $(WPA_SUPPL_DIR)/src \
    $(WPA_SUPPL_DIR)/src/common \
    $(WPA_SUPPL_DIR)/src/drivers \
    $(WPA_SUPPL_DIR)/src/l2_packet \
    $(WPA_SUPPL_DIR)/src/utils \
    $(WPA_SUPPL_DIR)/src/wps \
    system/core/libnl_2/include

include $(CLEAR_VARS)
LOCAL_MODULE := libCustomWifi
LOCAL_MODULE_TAGS := eng
LOCAL_SHARED_LIBRARIES := libc libcutils libnl
LOCAL_CFLAGS := $(L_CFLAGS)
LOCAL_SRC_FILES := driver_mac80211.c
LOCAL_C_INCLUDES := $(INCLUDES)
include $(BUILD_STATIC_LIBRARY)
