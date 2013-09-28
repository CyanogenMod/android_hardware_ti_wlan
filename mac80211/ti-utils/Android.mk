#
# Copyright (C) 2011 The Android Open Source Project
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
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        nvs.c \
        misc_cmds.c \
        calibrator.c \
        plt.c \
        ini.c

LOCAL_CFLAGS := -DCONFIG_LIBNL20
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    external/libnl-headers

LOCAL_STATIC_LIBRARIES := libnl_2
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := calibrator

include $(BUILD_EXECUTABLE)

# Build wlconf
include $(LOCAL_PATH)/wlconf/Android.mk


include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	wl_logproxy.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	external/libnl-headers

LOCAL_NO_DEFAULT_COMPILER_FLAGS := true
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libnl-headers \
	$(TARGET_PROJECT_INCLUDES) $(TARGET_C_INCLUDES)
LOCAL_CFLAGS := $(TARGET_GLOBAL_CFLAGS) $(PRIVATE_ARM_CFLAGS)

LOCAL_CFLAGS += -DCONFIG_LIBNL20=y

LOCAL_MODULE := wl_logproxy
LOCAL_LDFLAGS := -Wl,--no-gc-sections
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES := libnl_2
LOCAL_MODULE := wl_logproxy

include $(BUILD_EXECUTABLE)


include $(LOCAL_PATH)/hw/Android.mk