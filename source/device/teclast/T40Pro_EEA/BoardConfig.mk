#
# Teclast T40 Pro EEA (T1B1) - TWRP 11
# Values below are derived from the device's own boot_a, FDL partition table,
# stock recovery ramdisk and official T1B1 EEA V1.05 firmware package.
#

DEVICE_PATH := device/teclast/T40Pro_EEA

ALLOW_MISSING_DEPENDENCIES := true
BUILD_BROKEN_ELF_PREBUILT_PRODUCT_COPY_FILES := true

# A/B / recovery-as-boot
AB_OTA_UPDATER := true
AB_OTA_PARTITIONS += \
    boot \
    dtbo \
    vbmeta \
    vbmeta_system \
    vbmeta_vendor \
    vbmeta_product \
    vbmeta_system_ext \
    system \
    system_ext \
    vendor \
    product
BOARD_USES_RECOVERY_AS_BOOT := true

# Architecture
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_ABI2 :=
TARGET_CPU_VARIANT := generic
TARGET_CPU_VARIANT_RUNTIME := cortex-a75

TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv7-a-neon
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_ABI2 := armeabi
TARGET_2ND_CPU_VARIANT := generic
TARGET_2ND_CPU_VARIANT_RUNTIME := cortex-a55

TARGET_CPU_SMP := true
ENABLE_CPUSETS := true
ENABLE_SCHEDBOOST := true
DEXPREOPT_GENERATE_APEX_IMAGE := true

# Bootloader / platform
TARGET_BOOTLOADER_BOARD_NAME := ums512_1h10
TARGET_NO_BOOTLOADER := true
TARGET_BOARD_PLATFORM := ums512
BOARD_USES_SPRD_HARDWARE := true

# Recovery UI. 280 is intentionally used for the first TWRP test build;
# stock Android advertises 480, but that is an Android logical density and is
# too aggressive for a 10.4-inch recovery UI. Tune after first boot if needed.
TARGET_SCREEN_DENSITY := 280
TARGET_SCREEN_WIDTH := 1200
TARGET_SCREEN_HEIGHT := 2000

# Stock boot_a header v2 geometry
BOARD_BOOT_HEADER_VERSION := 2
BOARD_BOOTIMG_HEADER_VERSION := 2
BOARD_KERNEL_BASE := 0x00000000
BOARD_KERNEL_OFFSET := 0x00008000
BOARD_KERNEL_CMDLINE := console=ttyS1,115200n8 buildvariant=user androidboot.selinux=permissive
BOARD_KERNEL_PAGESIZE := 2048
BOARD_RAMDISK_OFFSET := 0x05400000
BOARD_KERNEL_TAGS_OFFSET := 0x00000100
BOARD_DTB_OFFSET := 0x01f00000

BOARD_MKBOOTIMG_ARGS += --header_version $(BOARD_BOOT_HEADER_VERSION)
BOARD_MKBOOTIMG_ARGS += --kernel_offset $(BOARD_KERNEL_OFFSET)
BOARD_MKBOOTIMG_ARGS += --ramdisk_offset $(BOARD_RAMDISK_OFFSET)
BOARD_MKBOOTIMG_ARGS += --tags_offset $(BOARD_KERNEL_TAGS_OFFSET)
BOARD_MKBOOTIMG_ARGS += --dtb_offset $(BOARD_DTB_OFFSET)

BOARD_KERNEL_IMAGE_NAME := Image
TARGET_FORCE_PREBUILT_KERNEL := true
TARGET_PREBUILT_KERNEL := $(DEVICE_PATH)/prebuilt/kernel
TARGET_PREBUILT_DTB := $(DEVICE_PATH)/prebuilt/dtb.img
BOARD_MKBOOTIMG_ARGS += --dtb $(TARGET_PREBUILT_DTB)

# Exact DTBO extracted from the official T1B1 EEA V1.05 PAC
BOARD_PREBUILT_DTBOIMAGE := $(DEVICE_PATH)/prebuilt/dtbo.img

# Physical partition sizes from the live FDL partition table
BOARD_FLASH_BLOCK_SIZE := 131072
BOARD_BOOTIMAGE_PARTITION_SIZE := 67108864
# Recovery lives in boot on this A/B layout; this is a build size guard only.
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 67108864
BOARD_DTBOIMG_PARTITION_SIZE := 8388608

BOARD_HAS_LARGE_FILESYSTEM := true
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true
BOARD_SYSTEMIMAGE_PARTITION_TYPE := ext4
BOARD_SYSTEMIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_PRODUCTIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_SYSTEM_EXTIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_USERDATAIMAGE_FILE_SYSTEM_TYPE := f2fs

TARGET_COPY_OUT_VENDOR := vendor
TARGET_COPY_OUT_PRODUCT := product
TARGET_COPY_OUT_SYSTEM_EXT := system_ext

# super = 4100 MiB exactly. Reserve 4 MiB for LP metadata, leaving a 4096 MiB group.
BOARD_SUPER_PARTITION_SIZE := 4299161600
BOARD_SUPER_PARTITION_GROUPS := teclast_dynamic_partitions
BOARD_TECLAST_DYNAMIC_PARTITIONS_PARTITION_LIST := system system_ext vendor product
BOARD_TECLAST_DYNAMIC_PARTITIONS_SIZE := 4294967296

BOARD_USES_METADATA_PARTITION := true

# Recovery
TARGET_RECOVERY_PIXEL_FORMAT := RGBX_8888
TARGET_RECOVERY_FSTAB := $(DEVICE_PATH)/recovery/root/system/etc/recovery.fstab
TW_SKIP_ADDITIONAL_FSTAB := true
TW_EMBEDDED_VENDOR_LIBS := true

# Verified Boot. The final image still needs the Unisoc signing/AVB treatment
# validated against this T1B1 before anything is flashed.
BOARD_AVB_ENABLE := true
BOARD_AVB_MAKE_VBMETA_IMAGE_ARGS += --flags 3

PLATFORM_VERSION := 11
PLATFORM_SECURITY_PATCH := 2021-08-05
VENDOR_SECURITY_PATCH := 2021-08-05

# TWRP
TWRP_INCLUDE_LOGCAT := true
TARGET_USES_LOGD := true
TW_THEME := portrait_hdpi
TW_EXTRA_LANGUAGES := false
TW_SCREEN_BLANK_ON_BOOT := false
TW_INPUT_BLACKLIST := "hbtp_vm"
TW_USE_TOOLBOX := true
TW_INCLUDE_CRYPTO := true
TW_INCLUDE_CRYPTO_FBE := true
TW_INCLUDE_FBE_METADATA_DECRYPT := true
TW_INCLUDE_REPACKTOOLS := true
TW_INCLUDE_RESETPROP := true
TW_INCLUDE_LIBRESETPROP := true
TW_INCLUDE_FASTBOOTD := true
TW_DRM_LEGACY_MODESET := true
TW_NO_FLASH_CURRENT_TWRP := true
TW_NO_FASTBOOT_BOOT := true
TW_INTERNAL_STORAGE_PATH := /data/media/0

# There is no compatible stock Unisoc boot-control HAL in the tree.
TARGET_EXCLUDE_BOOTCTRL := true
TARGET_NO_UPDATE_ENGINE := true
TARGET_NO_RECOVERY_UPDATE_ENGINE := true
USE_GENERIC_BOOTCTRL_HEADERS := true
BOARD_BOOTCTRL_HEADER_VERSION := 0
TARGET_BOOTCTRL_DISABLE := true
TARGET_BOARD_PLATFORM_NO_BOOTCTRL := true

# Overlay the tablet stock recovery runtime after AOSP has generated its SELinux targets.
BOARD_RECOVERY_IMAGE_PREPARE = bash $(DEVICE_PATH)/recovery-overlay.sh $(TARGET_RECOVERY_ROOT_OUT)


# T40 V48: existing keys only, readonly metadata mapper trial.
TW_T40_EXISTING_KEYS_ONLY := true
