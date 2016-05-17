wl18xx-objs	= main.o acx.o tx.o io.o debugfs.o

define filechk_version.h
	(echo 'static const char *wl18xx_git_head = "";')
endef

$(obj)/version.h: .git
	@$(call filechk,version.h)

$(obj)/main.c: $(src)/version.h

obj-$(CONFIG_WL18XX)		+= wl18xx.o
