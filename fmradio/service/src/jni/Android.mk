# Modified by Sony Ericsson Mobile Communications AB

LOCAL_PATH := $(call my-dir)
FM_STACK_PATH:= hardware/ti/wlan/fmradio/fm_stack/

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    JFmRxNative.cpp

LOCAL_SHARED_LIBRARIES := libcutils libhardware libfm_stack

LOCAL_C_INCLUDES += $(JNI_H_INCLUDE) \
	$(FM_STACK_PATH)/MCP_Common/Platform/fmhal/inc/int \
	$(FM_STACK_PATH)/MCP_Common/Platform/fmhal/inc 	\
	$(FM_STACK_PATH)/MCP_Common/Platform/os/Linux 	\
	$(FM_STACK_PATH)/MCP_Common/Platform/inc 		\
	$(FM_STACK_PATH)/MCP_Common/tran 	\
	$(FM_STACK_PATH)/MCP_Common/inc 	\
	$(FM_STACK_PATH)/HSW_FMStack/stack/inc/int \
	$(FM_STACK_PATH)/HSW_FMStack/stack/inc 	\
    $(FM_STACK_PATH)/FM_Trace/

LOCAL_CFLAGS +=

ifeq ($(BOARD_HAVE_BLUETOOTH),true)
LOCAL_C_INCLUDES += \
    system/bluetooth/bluez-clean-headers \
    system/bluetooth/bluedroid/include \

LOCAL_CFLAGS += -DHAVE_BLUETOOTH
LOCAL_SHARED_LIBRARIES += libbluedroid libdbus
endif

LOCAL_SHARED_LIBRARIES += liblog
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libfmrx
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

