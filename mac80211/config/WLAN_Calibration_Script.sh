#! /system/bin/sh

WIFION=`getprop init.svc.wpa_supplicant`

case "$WIFION" in
  "running") echo " ********************************************************"
             echo " * Turn Wi-Fi OFF and launch the script for calibration *"
             echo " ********************************************************"
             exit;;
          *) echo " ******************************"
             echo " * Starting Wi-Fi calibration *"
             echo " ******************************";;
esac

TARGET_FW_DIR=/system/etc/firmware/ti-connectivity
TARGET_FW_FILE=$TARGET_FW_DIR/wl1271-nvs.bin
TARGET_NVS_FILE=/system/etc/wifi/TQS_D_1.7.ini

# Remount system partition as rw
mount -o remount rw /system

cd $TARGET_FW_DIR

# Create reference NVS file
calibrator set ref_nvs $TARGET_NVS_FILE
mv ./new-nvs.bin $TARGET_FW_FILE

# Load driver
/system/bin/insmod /system/lib/modules/wl12xx_sdio.ko

# Calibrate...
calibrator wlan0 plt power_mode on
calibrator wlan0 plt tune_channel 0 7
calibrator wlan0 plt tx_bip 1 1 1 1 1 1 1 1
calibrator wlan0 plt power_mode off

# Update NVS with random mac address (Don't give second argument of MAC adress)
calibrator set nvs_mac ./new-nvs.bin
#calibrator set nvs_mac ./new-nvs.bin 08:00:28:12:12:12

/system/bin/rmmod wl12xx_sdio
/system/bin/rmmod wl12xx

mv ./new-nvs.bin $TARGET_FW_FILE

/system/bin/insmod /system/lib/modules/wl12xx.ko


echo " ******************************"
echo " * Finished Wi-Fi calibration *"
echo " ******************************"
