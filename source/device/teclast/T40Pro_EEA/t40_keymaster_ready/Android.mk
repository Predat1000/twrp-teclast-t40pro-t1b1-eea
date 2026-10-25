LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := t40_keymaster_ready
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/system/bin
LOCAL_SRC_FILES := t40_keymaster_ready.cpp
LOCAL_SHARED_LIBRARIES := libc libdl libm libc++ liblog libutils libhidlbase android.hardware.keymaster@4.0 android.hardware.keymaster@4.1
LOCAL_CFLAGS := -Wall -Wextra -Werror
include $(BUILD_EXECUTABLE)
