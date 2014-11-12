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
    $(LOCAL_PATH)

ifneq ($(wildcard external/libnl),)
LOCAL_C_INCLUDES += external/libnl/include
LOCAL_SHARED_LIBRARIES := libnl
else
LOCAL_C_INCLUDES += external/libnl-headers
LOCAL_STATIC_LIBRARIES := libnl_2
endif
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := calibrator

include $(BUILD_EXECUTABLE)

# Build wlconf
include $(LOCAL_PATH)/wlconf/Android.mk
