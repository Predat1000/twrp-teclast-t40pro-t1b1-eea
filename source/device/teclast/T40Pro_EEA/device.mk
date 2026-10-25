#
# Teclast T40 Pro EEA (T1B1)
#

LOCAL_PATH := device/teclast/T40Pro_EEA

# Recovery-as-boot on this Unisoc device needs the stock recovery runtime
# (init/ueventd/fstab + vendor SELinux policy/contexts).  Do not stage those
# files through PRODUCT_COPY_FILES: Android 12.1 already owns the SELinux
# recovery targets and duplicate destinations make Kati abort.  The complete
# stock runtime is overlaid at BOARD_RECOVERY_IMAGE_PREPARE time instead.
#
# recovery.fstab is also wired through TARGET_RECOVERY_FSTAB in BoardConfig.
PRODUCT_USE_DYNAMIC_PARTITIONS := true
