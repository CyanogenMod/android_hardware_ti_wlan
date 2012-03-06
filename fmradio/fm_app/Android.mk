BUILD_FMAPP:=1

FM_STACK_PATH:= $(call my-dir)/../fm_stack/
LOCAL_PATH := $(call my-dir)
ALSA_PATH := external/alsa-lib/

include $(CLEAR_VARS)

ifeq ($(BUILD_FMAPP),1)

#
# FM Application
#


LOCAL_C_INCLUDES:=\
	$(FM_STACK_PATH)/MCP_Common/Platform/fmhal/inc/int \
	$(FM_STACK_PATH)/MCP_Common/Platform/fmhal/inc 	\
	$(FM_STACK_PATH)/MCP_Common/Platform/os/Linux 	\
	$(FM_STACK_PATH)/MCP_Common/Platform/inc 		\
	$(FM_STACK_PATH)/MCP_Common/tran 	\
	$(FM_STACK_PATH)/MCP_Common/inc 	\
	$(FM_STACK_PATH)/HSW_FMStack/stack/inc/int \
	$(FM_STACK_PATH)/HSW_FMStack/stack/inc 	\
	external/bluez/libs/include 	\
	$(FM_STACK_PATH)/fm_app		\
	$(ALSA_PATH)/include 


LOCAL_CFLAGS:= -g -c -W -Wall -O2 -D_POSIX_SOURCE 

LOCAL_SRC_FILES:= \
fm_app.c fm_trace.c						

LOCAL_SHARED_LIBRARIES := \
	libbluetooth libaudio libfm_stack

LOCAL_STATIC_LIBRARIES := 

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng

LOCAL_MODULE:=fmapp

include $(BUILD_EXECUTABLE)

endif # BUILD_FMAPP_
