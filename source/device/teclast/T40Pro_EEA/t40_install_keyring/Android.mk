LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := t40_install_keyring
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := t40_install_keyring.c
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES := libc
LOCAL_CFLAGS := -Wall -Wextra -Werror
LOCAL_MULTILIB := 64
LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/system/bin
include $(BUILD_EXECUTABLE)
