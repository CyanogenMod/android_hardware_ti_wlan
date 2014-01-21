ifeq ($(BOARD_WLAN_CHIPSET),wl127x)

local_target_dir := $(TARGET_OUT)/etc/firmware/ti-connectivity
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := wl127x-fw-5-sr.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := wl127x-fw-5-mr.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := wl127x-fw-5-plt.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := wl1271-nvs.bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := wl1271-nvs_127x.bin
LOCAL_MODULE_CLASS := FIRMWARE
include $(BUILD_PREBUILT)

endif
