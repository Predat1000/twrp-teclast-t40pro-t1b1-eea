#
# Teclast T40 Pro EEA (T1B1) - TWRP product
#

$(call inherit-product, $(SRC_TARGET_DIR)/product/aosp_base.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product-if-exists, $(SRC_TARGET_DIR)/product/gsi_keys.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)
$(call inherit-product-if-exists, vendor/twrp/config/common.mk)

$(call inherit-product, device/teclast/T40Pro_EEA/device.mk)

PRODUCT_USE_DYNAMIC_PARTITIONS := true

PRODUCT_DEVICE := T40Pro_EEA
PRODUCT_NAME := twrp_T40Pro_EEA
PRODUCT_BRAND := Teclast
PRODUCT_MODEL := T40 Pro_EEA
PRODUCT_MANUFACTURER := teclast
PRODUCT_RELEASE_NAME := T40Pro_EEA

# Stock identity carried by boot_a (slot A is older than the currently used slot B).
PRODUCT_BUILD_PROP_OVERRIDES += \
    PRIVATE_BUILD_DESC="T40Pro_EEA-user 11 RP1A.201005.001 20210924 release-keys"
BUILD_FINGERPRINT := Teclast/T40Pro_EEA/T40Pro_EEA:11/RP1A.201005.001/20210924:user/release-keys
