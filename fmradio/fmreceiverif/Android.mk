# Modified by Sony Ericsson Mobile Communications AB

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := fmreceiverif
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(call all-java-files-under, src) \
           src/java/com/ti/fm/IFmReceiver.aidl

include $(BUILD_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := com.ti.fm.fmreceiverif.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions
LOCAL_SRC_FILES := $(LOCAL_MODULE)

LOCAL_CERTIFICATE := platform
include $(BUILD_PREBUILT)

