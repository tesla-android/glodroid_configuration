/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2020 Roman Stratiienko (r.stratiienko@gmail.com)
 *
 * This is GloDroid u-boot Bootscript macro file.
 * .cpp extension is used only to enable syntax highlighting.
 */

#include "bootscript.h"
#include "platform.h"
#include "device.h"

echo "Android Booting from $devtype:$devnum"

#ifdef PLATFORM_SETUP_ENV
PLATFORM_SETUP_ENV()
#else
#error PLATFORM_SETUP_ENV is not defined
#endif

setenv    main_fdt_id 0x100
setenv overlay_fdt_id 0xFFF

/* EMMC cards have 512k erase block size. Align partitions accordingly to avoid issues with erasing. */

setenv partitions "uuid_disk=\${uuid_gpt_disk}"
#ifdef BOOTLOADER_PARTITION_OVERRIDE
BOOTLOADER_PARTITION_OVERRIDE()
#else
EXTENV(partitions, ";name=bootloader,start=128K,size=130944K,uuid=\${uuid_gpt_bootloader}")
#endif
EXTENV(partitions, ";name=uboot-env,size=512K,uuid=\${uuid_gpt_reserved}")
EXTENV(partitions, ";name=misc,size=512K,uuid=\${uuid_gpt_misc}")
EXTENV(partitions, ";name=boot_a,size=64M,uuid=\${uuid_gpt_boot_a}")
EXTENV(partitions, ";name=boot_b,size=64M,uuid=\${uuid_gpt_boot_b}")
EXTENV(partitions, ";name=vendor_boot_a,size=32M,uuid=\${uuid_gpt_vendor_boot_a}")
EXTENV(partitions, ";name=vendor_boot_b,size=32M,uuid=\${uuid_gpt_vendor_boot_b}")
EXTENV(partitions, ";name=dtbo_a,size=8M,uuid=\${uuid_gpt_dtbo_a}")
EXTENV(partitions, ";name=dtbo_b,size=8M,uuid=\${uuid_gpt_dtbo_b}")
EXTENV(partitions, ";name=vbmeta_a,size=512K,uuid=\${uuid_gpt_vbmeta_a}")
EXTENV(partitions, ";name=vbmeta_b,size=512K,uuid=\${uuid_gpt_vbmeta_b}")
EXTENV(partitions, ";name=vbmeta_system_a,size=512K,uuid=\${uuid_gpt_vbmeta_system_a}")
EXTENV(partitions, ";name=vbmeta_system_b,size=512K,uuid=\${uuid_gpt_vbmeta_system_b}")
EXTENV(partitions, ";name=super,size="STR(__GD_SUPER_PARTITION_SIZE_MB__)"M,uuid=\${uuid_gpt_super}")
EXTENV(partitions, ";name=metadata,size=16M,uuid=\${uuid_gpt_metadata}")
EXTENV(partitions, ";name=userdata,size=-,uuid=\${uuid_gpt_userdata}")

setenv bootargs " init=/init rootwait ro androidboot.boottime=223.708 androidboot.selinux=permissive"
EXTENV(bootargs, " androidboot.revision=1.0 androidboot.board_id=0x1234567 androidboot.serialno=${serial#}")
EXTENV(bootargs, " firmware_class.path=/vendor/etc/firmware")
EXTENV(bootargs, " ${debug_bootargs} printk.devkmsg=on quiet")

FUNC_BEGIN(enter_fastboot)
#ifdef PRE_ENTER_FASTBOOT
 PRE_ENTER_FASTBOOT()
#endif
 setenv fastboot_fail 0
#ifdef platform_sunxi
 /* OTG on sunxi require USB to be initialized */
 usb start ;
#endif
 fastboot 0 || setenv fastboot_fail 1;
 /* If for some reason uboot-fastboot fail for this board, fallback to fastbootd */
 if test STRESC(${fastboot_fail}) = STRESC(1);
 then
  /* If the sdcard image is deploy image - reformat the GPT to allow fastbootd to flash Android partitions */
  part start \$devtype \$devnum misc misc_start || gpt write $partitions
  /* Boot into the fastbootd mode */
  bcb load \$devtype \$devnum misc && bcb set command boot-fastboot && bcb store
 fi;
FUNC_END()

FUNC_BEGIN(bootcmd_bcb)
 ab_select slot_name \$devtype \${devnum}#misc --no-dec || run enter_fastboot ;

 bcb load \$devtype \$devnum misc ;
 /* Handle $ adb reboot bootloader */
 bcb test command = bootonce-bootloader && bcb clear command && bcb store && run enter_fastboot ;
 /* Handle $ adb reboot fastboot */
 bcb test command = boot-fastboot && setenv androidrecovery true ;
 /* Handle $ adb reboot recovery (Android 11+) */
 bcb test command = boot-recovery && setenv androidrecovery true ;

 if test STRESC(\${androidrecovery}) != STRESC(true);
 then
  /* ab_select is used as counter of failed boot attempts. After 14 failed boot attempt fallback to fastboot. */
  ab_select slot_name \$devtype \${devnum}#misc || run enter_fastboot ;
 fi;

 FEXTENV(bootargs, " androidboot.slot_suffix=_\$slot_name") ;
FUNC_END()

FUNC_BEGIN(avb_verify)
 avb init \$devnum; avb verify _\$slot_name;
FUNC_END()

FUNC_BEGIN(bootcmd_avb)
#ifdef TODO_AVB_DISABLED
 EXTENV(bootargs, " androidboot.verifiedbootstate=orange ")
#else
 if run avb_verify; then
  echo AVB verification OK. Continue boot;
  EXTENV(bootargs, " ${avb_bootargs} ")
 else
  echo AVB verification failed;
  reset;
 fi;
#endif
FUNC_END()

FUNC_BEGIN(bootcmd_prepare_env)
 if test \"\${devtype}\" = \"mmc";
 then
 setenv device_path \"emmc2bus/fe340000.mmc\";
 fi;
 if test \"\${devtype}\" = \"nvme\";
 then
 setenv device_path \"scb/fd500000.pcie\";
 fi;
 if test \"\${devtype}\" = \"usb\";
 then
 setenv device_path \"soc/fe980000.usb\";
 fi;
 FEXTENV(bootargs, " androidboot.boot_devices=\${device_path}") ;
FUNC_END()

FUNC_BEGIN(bootcmd_start)
 if test STRESC(\${androidrecovery}) != STRESC(true);
 then
  FEXTENV(bootargs, " androidboot.force_normal_boot=1") ;
 fi;
 abootimg addr \$loadaddr \$vloadaddr

 adtimg addr \${dtboaddr}
#ifdef DEVICE_HANDLE_FDT
 DEVICE_HANDLE_FDT()
#endif
#ifdef PLATFORM_HANDLE_FDT
 PLATFORM_HANDLE_FDT()
#else
#error PLATFORM_HANDLE_FDT is not defined
#endif

#ifdef platform_broadcom
#endif
 adtimg get dt --id=\$overlay_fdt_id dtb_start dtb_size overlay_fdt_index &&
 cp.b \$dtb_start \$dtboaddr \$dtb_size &&
 fdt resize 8192 &&
#ifdef POSTPROCESS_FDT
 POSTPROCESS_FDT()
#endif
 fdt apply \$dtboaddr &&
 FEXTENV(bootargs, " androidboot.dtbo_idx=\${main_fdt_index},\${overlay_fdt_index}") ;
 /* START KERNEL */
 bootm \$loadaddr
 /* Should never get here */
FUNC_END()

FUNC_BEGIN(bootcmd_block)
#ifdef DEVICE_HANDLE_BUTTONS
 DEVICE_HANDLE_BUTTONS()
#endif
 run bootcmd_bcb
 EXTENV(bootargs, " androidboot.verifiedbootstate=orange ");

 part start \$devtype \$devnum boot_\$slot_name boot_start &&
 part size \$devtype \$devnum boot_\$slot_name boot_size

 part start \$devtype \$devnum vendor_boot_\$slot_name vendor_boot_start &&
 part size \$devtype \$devnum vendor_boot_\$slot_name vendor_boot_size

 part start \$devtype \$devnum dtbo_\$slot_name dtbo_start &&
 part size \$devtype \$devnum dtbo_\$slot_name dtbo_size
 \$devtype dev \$devnum &&
 \$devtype read \$loadaddr \$boot_start \$boot_size
 \$devtype read \$vloadaddr \$vendor_boot_start \$vendor_boot_size
 \$devtype read \$dtboaddr \$dtbo_start \$dtbo_size
FUNC_END()

FUNC_BEGIN(rename_and_expand_userdata_placeholder)
  part number $devtype ${devnum} userdata_placeholder partition_number
  if test -n "${partition_number}";
  then
    echo "Renaming userdata_placeholder partition to userdata...";
    gpt read $devtype ${devnum} current_layout
    setexpr new_layout gsub "name=userdata_placeholder" "name=userdata" ${current_layout}
    gpt write $devtype ${devnum} ${new_layout}
    echo "The userdata_placeholder partition has been renamed to userdata.";
    echo "Expanding userdata partition to fill the entire drive...";
    gpt read $devtype ${devnum} expanded_layout
    setexpr final_layout gsub "name=userdata,start=[^,]*,size=[^,]*,uuid" "name=userdata,start=[^,]*,size=-,uuid" ${expanded_layout}
    gpt write $devtype ${devnum} ${final_layout}
    echo "The userdata partition has been expanded.";
  fi;
FUNC_END()

FUNC_BEGIN(bootcmd)
 run bootcmd_prepare_env ;
 run rename_and_expand_userdata_placeholder ;
 run bootcmd_block ;
 run bootcmd_start ;
FUNC_END()

run bootcmd

reset
