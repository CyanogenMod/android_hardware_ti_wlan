wl12xx-objs		= main.o cmd.o io.o event.o tx.o rx.o ps.o acx.o \
			  boot.o init.o debugfs.o scan.o

# generate version string
WL12XX_GIT_VER = $(shell git describe --long)

# define the version string when compiling version.c
CFLAGS_version.o	+= -D"WL12XX_GIT_HEAD=KBUILD_STR($(WL12XX_GIT_VER))"

# convert all wl12xx-objs to $(src)/file form
define WL12XX_OBJS_SRC
$(shell echo $(wl12xx-objs) |
	awk '{for (i=1;i<=NF;i++) {printf "$(src)/%s ", $$i} }')
endef

# touch version.c on each change to the objs or git head
$(obj)/version.c: .git/HEAD .git/index $(WL12XX_OBJS_SRC)
	@touch $@
	@$(kecho) '$@ touched'

wl12xx-objs		+= version.o

wl12xx_spi-objs 	= spi.o
wl12xx_sdio-objs	= sdio.o

wl12xx-$(CONFIG_NL80211_TESTMODE)	+= testmode.o
obj-$(CONFIG_WL12XX)			+= wl12xx.o
obj-$(CONFIG_WL12XX_SPI)		+= wl12xx_spi.o
obj-$(CONFIG_COMPAT_WL12XX_SDIO)		+= wl12xx_sdio.o

# small builtin driver bit
obj-$(CONFIG_WL12XX_PLATFORM_DATA)	+= wl12xx_platform_data.o
