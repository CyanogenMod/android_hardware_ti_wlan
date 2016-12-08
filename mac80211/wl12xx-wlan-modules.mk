WL12XX_FOLDER ?= hardware/ti/wlan/mac80211/compat_wl12xx

WL12XX_MODULES:
	make clean -C $(WL12XX_FOLDER)
	make -j8 -C $(WL12XX_FOLDER) KERNELDIR=$(KERNEL_OUT) KLIB=$(KERNEL_OUT) \
		KLIB_BUILD=$(KERNEL_OUT) ARCH=arm $(if $(ARM_CROSS_COMPILE),$(ARM_CROSS_COMPILE),$(KERNEL_CROSS_COMPILE))
	mv $(WL12XX_FOLDER)/{compat/compat,net/mac80211/mac80211,net/wireless/cfg80211,drivers/net/wireless/wl12xx/wl12xx,drivers/net/wireless/wl12xx/wl12xx_sdio}.ko $(KERNEL_MODULES_OUT)
	$(if $(ARM_EABI_TOOLCHAIN),$(ARM_EABI_TOOLCHAIN)/arm-eabi-strip,$(KERNEL_TOOLCHAIN_PATH)strip) \
		--strip-unneeded $(KERNEL_MODULES_OUT)/{compat,cfg80211,mac80211,wl12xx,wl12xx_sdio}.ko

TARGET_KERNEL_MODULES += WL12XX_MODULES
