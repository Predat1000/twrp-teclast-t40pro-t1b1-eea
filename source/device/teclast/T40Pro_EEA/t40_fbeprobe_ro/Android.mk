LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := t40_fbeprobe_ro
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 64
LOCAL_SRC_FILES := t40_fbeprobe_ro.cpp
LOCAL_CPPFLAGS := -std=c++17 -Wall -Wextra -Werror
LOCAL_STATIC_LIBRARIES := libdm libext2_uuid
LOCAL_SHARED_LIBRARIES := android.hardware.keymaster@4.1 android.hardware.keymaster@4.0 libhidlbase libutils liblog libcrypto libc++ libbase
include $(BUILD_EXECUTABLE)
