/*
 * Copyright (C) 2012 Texas Instruments Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef __WLCONF_H__
#define __WLCONF_H__

struct element {
	char *name;
	int type;
	int array_size;
	int *value;
	size_t position;
};

struct structure {
	char *name;
	int n_elements;
	struct element *elements;
	size_t size;
};

struct type {
	char *name;
	int size;
	char *format;
};

struct dict_entry {
	char *ini_str;
	char *element_str;
};

enum text_file_type {
	TEXT_FILE_INI,
	TEXT_FILE_CONF,
};

struct type types[] = {
	{ "u32", 4, "%u" },
	{ "u16", 2, "%u" },
	{ "u8",  1, "%u" },
	{ "s32", 4, "%d" },
	{ "s16", 2, "%d" },
	{ "s8",  1, "%d" },
	{ "__le32", 4, "%d" },
	{ "__le16", 2, "%d" },
};

/*
 * WLCONF_DIR can contains the default directory for wlconf metafiles
 * It is used in case of Android build so wlconf can run anywhere
 * See in Android.mk
 */
#ifndef WLCONF_DIR
#define WLCONF_DIR ""
#endif

#define DEFAULT_INPUT_FILENAME	WLCONF_DIR "wl18xx-conf-default.bin"
#define DEFAULT_OUTPUT_FILENAME		   "wl18xx-conf.bin"
#define DEFAULT_BIN_FILENAME	WLCONF_DIR "struct.bin"
#define DEFAULT_DICT_FILENAME	WLCONF_DIR "dictionary.txt"
#define DEFAULT_CONF_FILENAME	WLCONF_DIR "default.conf"
#define DEFAULT_ROOT_STRUCT	"wlcore_conf_file"
#define DEFAULT_MAGIC_SYMBOL	"WL18XX_CONF_MAGIC"
#define DEFAULT_VERSION_SYMBOL	"WL18XX_CONF_VERSION"
#define DEFAULT_MAGIC_ELEMENT	"header.magic"
#define DEFAULT_VERSION_ELEMENT	"header.version"
#define DEFAULT_CHKSUM_ELEMENT	"header.checksum"

#define STRUCT_BASE		1000

#define MAX_ARRAY_STR_LEN	4096
#define MAX_VALUE_STR_LEN	256

#define STRUCT_PATTERN 	"[\n\t\r ]*struct[\n\t\r ]+([a-zA-Z0-9_]+)"	\
	"[\n\t\r ]*\\{[\n\t\r ]*([^}]*)\\}[\n\t\r ]*"			\
	"[a-zA-Z0-9_]*;[\n\t\r ]*"

#define ELEMENT_PATTERN	"[\n\t\r ]*([A-Za-z0-9_]+)[\n\t\r ]+" \
	"([a-zA-Z_][a-zA-Z0-9_]*)(\\[([0-9]+)\\])?[\n\t\r ]*;[\n\t\r ]*"

#define TEXT_CONF_PATTERN	"^[\t ]*([A-Za-z_][A-Za-z0-9_.]*)" \
	"[\t ]*=[\t ]*([A-Za-z0-9_, \t]+)"

#define TEXT_INI_PATTERN	"^[\t ]*([A-Za-z_][A-Za-z0-9_]*)" \
	"[\t ]*=[\t ]*([0-9A-Fa-f \t]+)"

#define DICT_PATTERN		"^[\t ]*([A-Za-z_][A-Za-z0-9_]*)" \
	"[\t ]+([A-Za-z_][A-Za-z0-9_.]*)"

#define CC_COMMENT_PATTERN	"(([^/]|[/][^/])*)//[^\n]*\n(.*)"

#define C_COMMENT_PATTERN	"(([^/]|[/][^*])*)/\\*(([^*]|[*][^/])*)\\*/(.*)"


/* we only match WL12XX and WL18XX magic */
#define DEFINE_PATTERN	"#define[\n\t\r ]+([A-Za-z_][A-Za-z0-9_]*)"	\
	"[\n\t\r ]+(0x[0-9A-Fa-f]+)"

#define WRITE_INT32(from, file) {			\
		int32_t val = (int32_t) from;		\
		fwrite(&val, 1, sizeof(val), file);	\
	}

#define READ_INT32(into, type, file) {			\
		int32_t val;				\
		fread(&val, 1, sizeof(val), file);	\
		into = type val;			\
	}

#endif /* __WLCONF_H__ */
