# Copyright (C) 2016 The Android Open Source Project
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

TI_WLAN_FOLDER := hardware/ti/wlan

WL12XX_MODULES:
	make clean -C $(TI_WLAN_FOLDER)/mac80211/compat_wl12xx
	make -j8 -C $(TI_WLAN_FOLDER)/mac80211/compat_wl12xx KERNELDIR=$(KERNEL_OUT) KLIB=$(KERNEL_OUT) \
		KLIB_BUILD=$(KERNEL_OUT) ARCH=arm $(if $(ARM_CROSS_COMPILE),$(ARM_CROSS_COMPILE),$(KERNEL_CROSS_COMPILE))
	mv $(TI_WLAN_FOLDER)/mac80211/compat_wl12xx/{compat/compat,net/mac80211/mac80211,net/wireless/cfg80211,drivers/net/wireless/wl12xx/wl12xx,drivers/net/wireless/wl12xx/wl12xx_sdio}.ko $(KERNEL_MODULES_OUT)
	$(if $(ARM_EABI_TOOLCHAIN),$(ARM_EABI_TOOLCHAIN)/arm-eabi-strip,$(KERNEL_TOOLCHAIN_PATH)strip) \
		--strip-unneeded $(KERNEL_MODULES_OUT)/{compat,cfg80211,mac80211,wl12xx,wl12xx_sdio}.ko
