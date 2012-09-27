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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <regex.h>
#include <sys/stat.h>
#include <getopt.h>

#include "crc32.h"
#include "wlconf.h"

#ifdef ANDROID
static ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{
	char *lptr;
	ssize_t len;

	lptr = fgetln(stream, (size_t*) &len);
	if (lptr == NULL) {
		len = -1;
		goto out;
	}

	if (*lineptr == NULL || (ssize_t) *n < (len + 1)) {
		/* needs reallocation */
		char *newptr = realloc(*lineptr, len + 1);
		if (newptr == NULL) {
			/* realloc failed, we still have old lineptr and n */
			len = -1;
			goto out;
		}

		*lineptr = newptr;
		*n = len + 1;
	}

	memcpy(*lineptr, lptr, len);
	(*lineptr)[len] = '\0';

out:
	return len;
}
#endif

static struct dict_entry *dict = NULL;
static int n_dict_entries = 0;

static struct structure *structures = NULL;
static int n_structs = 0;

static int magic		= 0;
static int version		= 0;
static int checksum		= 0;
static int struct_chksum	= 0;
static int ignore_checksum	= 0;

static int get_type(char *type_str)
{
	int i;

	for (i = 0; i < (sizeof(types) / sizeof(types[0])); i++)
		if (!strcmp(type_str, types[i].name))
			return i;

	for (i = 0; i < n_structs; i++) {
		struct structure *curr_struct;
		curr_struct = &structures[i];

		if (!strcmp(type_str, curr_struct->name)) {
			return STRUCT_BASE + i;
		}
	}

	return -1;
}

static char *remove_comments(const char *orig_str)
{
	regex_t r;
	char *new_str = NULL;
	regmatch_t m[6];

	if (regcomp(&r, CC_COMMENT_PATTERN, REG_EXTENDED) < 0)
		goto out;

	new_str = strdup(orig_str);

	while (!regexec(&r, new_str, 4, m, 0)) {
		char *part1, *part2;

		part1 = strndup(new_str + m[1].rm_so, m[1].rm_eo - m[1].rm_so);
		part2 = strndup(new_str + m[3].rm_so, m[3].rm_eo - m[3].rm_so);

		snprintf(new_str, strlen(orig_str), "%s%s", part1, part2);

		free(part1);
		free(part2);
	}

	regfree(&r);

	if (regcomp(&r, C_COMMENT_PATTERN, REG_EXTENDED) < 0)
		goto out;

	while (!regexec(&r, new_str, 6, m, 0)) {
		char *part1, *part2;

		part1 = strndup(new_str + m[1].rm_so, m[1].rm_eo - m[1].rm_so);
		part2 = strndup(new_str + m[5].rm_so, m[5].rm_eo - m[5].rm_so);

		snprintf(new_str, strlen(orig_str), "%s%s", part1, part2);

		free(part1);
		free(part2);
	}

	regfree(&r);
out:
	return new_str;
}

static struct structure *get_struct(const char *structure)
{
	int j;

	for (j = 0; j < n_structs; j++)
		if (!strcmp(structure, structures[j].name))
			return &structures[j];

	return NULL;
}

static int parse_define(const char *buffer, char *symbol, int *value)
{
	regex_t r;
	int ret;
	const char *str;

	ret = regcomp(&r, DEFINE_PATTERN, REG_EXTENDED);
	if (ret < 0)
		goto out;

	str = buffer;

	while (strlen(str)) {
		regmatch_t m[3];
		char *symbol_str, *value_str = NULL;

		if (regexec(&r, str, 3, m, 0))
			break;

		symbol_str =
			strndup(str + m[1].rm_so, m[1].rm_eo - m[1].rm_so);

		value_str = strndup(str + m[2].rm_so, m[2].rm_eo - m[2].rm_so);

		if (!strcmp(symbol, symbol_str)) {
			if (*value == 0) {
				*value = strtol(value_str, NULL, 0);
				printf("symbol %s found %s (%08x)\n",
				       symbol, value_str, *value);
			} else {
				fprintf(stderr, "symbol %s redefined\n",
					symbol_str);
				ret = -1;
			}
		}

		free(symbol_str);
		symbol_str = NULL;
		free(value_str);
		value_str = NULL;

		str += m[2].rm_eo;
	};

out:
	regfree(&r);
	return ret;
}

static int parse_elements(char *orig_str, struct element **elements,
			  size_t *size)
{
	regex_t r;
	int ret, n_elements = 0;
	char *str, *clean_str;

	*size = 0;

	ret = regcomp(&r, ELEMENT_PATTERN, REG_EXTENDED);
	if (ret < 0)
		goto out;

	clean_str = remove_comments(orig_str);
	if (!clean_str)
		goto out;

	*elements = NULL;

	str = clean_str;

	while (strlen(str)) {
		regmatch_t m[6];
		char *type_str, *array_size_str = NULL;
		struct element *curr_element;

		if (regexec(&r, str, 6, m, 0))
			break;

		*elements = realloc(*elements,
				    ++n_elements * sizeof(**elements));
		if (!elements) {
			ret = -1;
			goto out_free;
		}

		curr_element = &(*elements)[n_elements - 1];

		curr_element->name =
			strndup(str + m[2].rm_so, m[2].rm_eo - m[2].rm_so);

		curr_element->position = *size;

		if (m[4].rm_so == m[4].rm_eo) {
			curr_element->array_size = 1;
		} else {
			array_size_str =
				strndup(str + m[4].rm_so, m[4].rm_eo - m[4].rm_so);
			curr_element->array_size = strtol(array_size_str, NULL, 0);
		}

		type_str = strndup(str + m[1].rm_so, m[1].rm_eo - m[1].rm_so);
		curr_element->type = get_type(type_str);
		if (curr_element->type == -1)
			fprintf(stderr, "Error! Unknown type '%s'\n", type_str);

		if (curr_element->type < STRUCT_BASE)
			*size += curr_element->array_size *
				types[curr_element->type].size;
		else {
			struct structure *structure =
				get_struct(type_str);

			*size += curr_element->array_size *
				structure->size;
		}

		free(type_str);
		free(array_size_str);

		str += m[2].rm_eo;
	}

	ret = n_elements;

out_free:
	free(clean_str);
	regfree(&r);
out:
	return ret;
}

static void print_usage(char *executable)
{
	printf("Usage:\n\t%s [OPTIONS] [COMMANDS]\n"
	       "\n\tOPTIONS\n"
	       "\t-S, --source-struct\tuse the structure specified in a C header file\n"
	       "\t-b, --binary-struct\tspecify the binary file where the structure is defined\n"
	       "\t-i, --input-config\tlocation of the input binary configuration file\n"
	       "\t-o, --output-config\tlocation of the input binary configuration file\n"
	       "\t-X, --ignore-checksum\tignore file checksum error detection\n"
	       "\n\tCOMMANDS\n"
	       "\t-D, --create-default\tcreate default configuration bin file (%s)\n"
	       "\t-g, --get\t\tget the value of the specified element (element[.element...]) or\n"
	       "\t\t\t\tprint the entire tree if no element is specified\n"
	       "\t-s, --set\t\tset the value of the specified element (element[.element...])\n"
	       "\t-G, --generate-struct\tgenerate the binary structure file from\n"
	       "\t\t\t\tthe specified source file\n"
	       "\t-C, --parse-text-conf\tparse the specified text config and set the values accordingly\n"
	       "\t-I, --parse-ini\t\tparse the specified INI file and set the values accordingly\n"
	       "\t\t\t\tin the output binary configuration file\n"
	       "\t-p, --print-struct\tprint out the structure\n"
	       "\t-h, --help\t\tprint this help\n"
	       "\n",
	       executable,
	       DEFAULT_INPUT_FILENAME);
}

static void free_structs(void)
{
	int i;

	for (i = 0; i < n_structs; i++)
		free(structures[i].elements);

	free(structures);
}

static void free_dict(void)
{
	int i;

	for (i = 0; i < n_dict_entries; i++) {
		free(dict[i].ini_str);
		free(dict[i].element_str);
	}

	free(dict);
}

static int parse_header(const char *buffer)
{
	regex_t r;
	const char *str;
	int ret;

	ret = parse_define(buffer, DEFAULT_MAGIC_SYMBOL, &magic);
	if (ret < 0)
		goto out;

	ret = parse_define(buffer, DEFAULT_VERSION_SYMBOL, &version);
	if (ret < 0)
		goto out;

	ret = regcomp(&r, STRUCT_PATTERN, REG_EXTENDED);
	if (ret < 0)
		goto out;

	str = buffer;

	while (strlen(str)) {
		char *elements_str;
		struct structure *curr_struct;
		regmatch_t m[4];

		if (regexec(&r, str, 4, m, 0))
			break;

		structures = realloc(structures, ++n_structs *
				     sizeof(struct structure));
		if (!structures) {
			ret = -1;
			goto out_free;
		}

		curr_struct = &structures[n_structs - 1];

		curr_struct->name =
			strndup(str + m[1].rm_so, m[1].rm_eo - m[1].rm_so);

		elements_str = strndup(str + m[2].rm_so, m[2].rm_eo - m[2].rm_so);

		ret = parse_elements(elements_str, &curr_struct->elements,
				     &curr_struct->size);
		if (ret < 0)
			break;

		curr_struct->n_elements = ret;

		str += m[2].rm_eo;
	}

out_free:
	regfree(&r);
out:
	return ret;
}

static void print_data(struct element *elem, void *data)
{
	uint8_t *u8;
	uint16_t *u16;
	uint32_t *u32;
	int i;
	char *pos = data;

	for (i = 0; i < elem->array_size; i++) {
		switch (types[elem->type].size) {
		case 1:
			u8 = (uint8_t *) pos;
			printf("0x%02x", *u8);
			break;
		case 2:
			u16 = (uint16_t *) pos;
			printf("0x%04x", *u16);
			break;
		case 4:
			u32 = (uint32_t *) pos;
			printf("0x%08x", *u32);
			break;
		default:
			fprintf(stderr, "Error! Unsupported data size\n");
			break;
		}

		if (i < elem->array_size - 1)
			printf(", ");

		pos += types[elem->type].size;
	}

	printf("\n");
}

static int set_data(struct element *elem, void *buffer, void *data)
{
	switch (types[elem->type].size) {
	case 1:
		*((uint8_t *)buffer) = *((uint8_t *)data);
		break;
	case 2:
		*((uint16_t *)buffer) = *((uint16_t *)data);
		break;
	case 4:
		*((uint32_t *)buffer) = *((uint32_t *)data);
		break;
	default:
		fprintf(stderr, "Error! Unsupported data size\n");
		break;
	}

	return 0;
}

static int print_element(struct element *elem, char *parent, void *data)
{
	char *pos = data, *curr_name = NULL;
	size_t len;
	int ret = 0;

	if (parent) {
		len = parent ? strlen(parent) : 0;
		len += strlen(elem->name) + 2;
		curr_name = malloc(len);
		if (!curr_name) {
			fprintf(stderr, "couldn't allocate memory\n");
			ret = -1;
			goto out;
		}

		sprintf(curr_name, "%s.%s", parent, elem->name);
	} else if (elem->type != get_type(DEFAULT_ROOT_STRUCT)) {
		curr_name = strdup(elem->name);
	}

	if (elem->type < STRUCT_BASE) {
		if (data) {
			printf("%s = ", curr_name);
			print_data(elem, pos);
			pos += elem->array_size *
				types[elem->type].size;
		} else {
			printf("\t%s %s[%d]\n",
			       types[elem->type].name, elem->name,
			       elem->array_size);
		}
	} else {
		struct structure *sub;
		int j;

		sub = &structures[elem->type - STRUCT_BASE];

		if (!data)
			printf("struct %s {\n", sub->name);

		for (j = 0; j < sub->n_elements; j++) {
			print_element(&sub->elements[j], curr_name, pos);

			if (!data)
				continue;

			if (sub->elements[j].type < STRUCT_BASE)
				pos += sub->elements[j].array_size *
					types[sub->elements[j].type].size;
			else
				pos += sub->elements[j].array_size *
					structures[sub->elements[j].type - STRUCT_BASE].size;
		}

		if (!data)
			printf("} /* %s */\n", sub->name);
	}

	free(curr_name);

out:
	return ret;
}

static int read_file(const char *filename, void **buffer, size_t size)
{
	FILE *file;
	struct stat st;
	int ret;
	size_t buf_size;

	ret = stat(filename, &st);
	if (ret < 0) {
		fprintf(stderr, "Couldn't get file size '%s'\n", filename);
		goto out;
	}

	file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Couldn't open file '%s'\n", filename);
		return -1;
	}

	if (size)
		buf_size = size;
	else
		buf_size = st.st_size;

	*buffer = malloc(buf_size);
	if (!*buffer) {
		fprintf(stderr, "Couldn't allocate enough memory (%d)\n", buf_size);
		fclose(file);
		return -1;
	}

	if (fread(*buffer, 1, buf_size, file) != buf_size) {
		fprintf(stderr, "Failed to read file '%s'\n", filename);
		fclose(file);
		return -1;
	}

	fclose(file);
out:
	return ret;
}

static void free_file(void *buffer)
{
	free(buffer);
}

static int write_file(const char *filename, const void *buffer, size_t size)
{
	FILE *file;
	int ret;

	file = fopen(filename, "w");
	if (!file) {
		fprintf(stderr, "Couldn't open file '%s' for writing\n",
			filename);
		ret = -1;
		goto out;
	}

	if (fwrite(buffer, 1, size, file) != size) {
		fprintf(stderr, "Failed to write file '%s'\n", filename);
		fclose(file);
		ret = -1;
		goto out;
	}

	fclose(file);
out:
	return ret;
}

static int get_element_pos(struct structure *structure, const char *argument,
			   struct element **element)
{
	int i, pos = 0;
	struct structure *curr_struct = structure;
	struct element *curr_element = NULL;
	char *str, *arg = strdup(argument);

	str = strtok(arg, ".");

	while(str) {

		for (i = 0; i < curr_struct->n_elements; i++)
			if (!strcmp(str, curr_struct->elements[i].name)) {
				curr_element = &curr_struct->elements[i];
				pos += curr_element->position;
				break;
			}

		if (i == curr_struct->n_elements) {
			pos = -1;
			goto out;
		}

		str = strtok(NULL, ".");
		if (str && curr_element->type < STRUCT_BASE) {
			fprintf(stderr, "element %s is not a struct\n",
				curr_element->name);
			pos = -1;
			goto out;
		}

		if (str)
			curr_struct =
				&structures[curr_element->type - STRUCT_BASE];
	}

out:
	*element = curr_element;
	free(arg);
	return pos;
}

static void get_value(void *buffer, struct structure *structure,
		      char *argument)
{
	int pos;
	struct element *element, *root_element = NULL;
	char *elim;

	if (argument) {
		pos = get_element_pos(structure, argument, &element);
		if (pos < 0) {
			fprintf(stderr, "couldn't find %s\n", argument);
			return;
		}
		/* change argument into parent by removing the last
		 * element name */
		elim = strrchr(argument, '.');
		if (elim)
			*elim = '\0';
		else
			argument = NULL;
	} else {
		root_element = malloc(sizeof(*element));
		root_element->name = NULL;
		root_element->type = get_type(DEFAULT_ROOT_STRUCT);
		root_element->array_size = 1;
		root_element->value = NULL;
		root_element->position = 0;
		element = root_element;
		pos = 0;
	}

	if (buffer)
		buffer = ((char *)buffer) + pos;

	print_element(element, argument, buffer);

	free(root_element);
}

static int set_value(void *buffer, struct structure *structure,
		     char *argument)
{
	int pos, ret = 0;
	char *split_point, *element_str, *value_str;
	struct element *element;
	uint32_t value;

	split_point = strchr(argument, '=');
	if (!split_point) {
		fprintf(stderr,
			"--set requires the format <element>[.<element>...]=<value>\n");
		ret = -1;
		goto out;
	}

	*split_point = '\0';
	element_str = argument;
	value_str = split_point + 1;

	pos = get_element_pos(structure, element_str, &element);
	if (pos < 0) {
		fprintf(stderr, "couldn't find %s\n", element_str);
		ret = -1;
		goto out;
	}

	if (element->array_size > 1) {
		fprintf(stderr, "setting arrays not supported yet\n");
		ret = -1;
		goto out;
	}

	if (element->type >= STRUCT_BASE) {
		fprintf(stderr,
			"setting entire structures is not supported.\n");
		ret = -1;
		goto out;
	}

	value = strtoul(value_str, NULL, 0);
	ret = set_data(element, ((char *)buffer) + pos, &value);

out:
	return ret;
}

static int write_element(FILE *file, struct element *element)
{
	size_t name_len = strlen(element->name);

	WRITE_INT32(name_len, file);
	fwrite(element->name, 1, name_len, file);
	WRITE_INT32(element->type, file);
	WRITE_INT32(element->array_size, file);
	WRITE_INT32(element->position, file);

	return 0;
}

static int write_struct(FILE *file, struct structure *structure)
{
	size_t name_len = strlen(structure->name);
	int i, ret = 0;

	WRITE_INT32(name_len, file);
	fwrite(structure->name, 1, name_len, file);
	WRITE_INT32(structure->n_elements, file);
	WRITE_INT32(structure->size, file);

	for (i = 0; i < structure->n_elements; i++) {
		ret = write_element(file, &structure->elements[i]);
		if (ret < 0)
			break;
	}

	return ret;
}

static int generate_struct(const char *filename)
{
	FILE *file;
	int i, ret = 0;

	file = fopen(filename, "w");
	if (!file) {
		fprintf(stderr, "Couldn't open file '%s' for writing\n",
			filename);
		ret = -1;
		goto out;
	}

	WRITE_INT32(magic, file);
	WRITE_INT32(version, file);
	WRITE_INT32(struct_chksum, file);
	WRITE_INT32(n_structs, file);

	for (i = 0; i < n_structs; i++) {
		ret = write_struct(file, &structures[i]);
		if (ret < 0)
			break;
	}

	fclose(file);
out:
	return ret;
}

static int read_element(FILE *file, struct element *element)
{
	size_t name_len;
	int ret = 0;

	READ_INT32(name_len, (size_t), file);

	element->name = malloc(name_len + 1);
	if (!element->name) {
		fprintf(stderr, "Couldn't allocate enough memory (%d)\n",
			name_len);
		ret = -1;
		goto out;
	}

	fread(element->name, 1, name_len, file);
	element->name[name_len] = '\0';

	READ_INT32(element->type, (int), file);
	READ_INT32(element->array_size, (int), file);
	READ_INT32(element->position, (size_t), file);

out:
	return ret;
}

static int read_struct(FILE *file, struct structure *structure)
{
	size_t name_len;
	int i, ret = 0;

	READ_INT32(name_len, (size_t), file);

	structure->name = malloc(name_len + 1);
	if (!structure->name) {
		fprintf(stderr, "Couldn't allocate enough memory (%d)\n",
			name_len);
		ret = -1;
		goto out;
	}

	fread(structure->name, 1, name_len, file);
	structure->name[name_len] = '\0';

	READ_INT32(structure->n_elements, (int), file);
	READ_INT32(structure->size, (size_t), file);

	structure->elements = malloc(structure->n_elements *
				     sizeof(struct element));
	if (!structure->elements) {
		fprintf(stderr, "Couldn't allocate enough memory (%d)\n",
			structure->n_elements * sizeof(struct element));
		ret = -1;
		goto out;
	}

	for (i = 0; i < structure->n_elements; i++) {
		ret = read_element(file, &structure->elements[i]);
		if (ret < 0)
			break;
	}

out:
	return ret;
}

static int read_binary_struct(const char *filename)
{
	FILE *file;
	int i, ret = 0;

	file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Couldn't open file '%s' for reading\n",
			filename);
		ret = -1;
		goto out;
	}

	READ_INT32(magic, (int), file);
	READ_INT32(version, (int), file);
	READ_INT32(struct_chksum, (int), file);
	READ_INT32(n_structs, (int), file);

	structures = malloc(n_structs * sizeof(struct structure));
	if (!structures) {
		fprintf(stderr, "Couldn't allocate enough memory (%d)\n",
			n_structs * sizeof(struct structure));
		ret = -1;
		goto out_close;
	}

	for (i = 0; i < n_structs; i++) {
		ret = read_struct(file, &structures[i]);
		if (ret < 0)
			break;
	}

out_close:
	fclose(file);
out:
	return ret;
}

static int get_value_int(void *buffer, struct structure *structure,
			 int *value, char *element_str)
{
	int pos, ret = 0;
	struct element *element;

	pos = get_element_pos(structure, element_str, &element);
	if (pos < 0) {
		fprintf(stderr, "couldn't find %s\n", element_str);
		ret = pos;
		goto out;
	}

	if ((element->type != get_type("u32")) &&
	    (element->type != get_type("__le32"))) {
		fprintf(stderr,
			"element %s has invalid type (expected u32 or le32)\n",
			element_str);
		ret = -1;
		goto out;
	}

	*value = *(int *) (((char *)buffer) + pos);

out:
	return ret;
}

static int set_value_int(void *buffer, struct structure *structure,
			 int value, char *element_str)
{
	int pos, ret = 0;
	struct element *element;

	pos = get_element_pos(structure, element_str, &element);
	if (pos < 0) {
		fprintf(stderr, "couldn't find %s\n", element_str);
		ret = pos;
		goto out;
	}

	if ((element->type != get_type("u32")) &&
	    (element->type != get_type("__le32"))) {
		fprintf(stderr,
			"element %s has invalid type (expected u32 or le32)\n",
			element_str);
		ret = -1;
		goto out;
	}

	*(int *) (((char *)buffer) + pos) = value;

out:
	return ret;
}

static int read_input(const char *filename, void **buffer,
		      struct structure *structure)
{
	int ret;
	int input_magic, input_version, input_checksum;

	ret = read_file(filename, buffer, structure->size);
	if (ret < 0)
		goto out;

	ret = get_value_int(*buffer, structure, &input_magic,
			    DEFAULT_MAGIC_ELEMENT);
	if (ret < 0)
		goto out;

	ret = get_value_int(*buffer, structure, &input_version,
			    DEFAULT_VERSION_ELEMENT);
	if (ret < 0)
		goto out;

	ret = get_value_int(*buffer, structure, &input_checksum,
			    DEFAULT_CHKSUM_ELEMENT);
	if (ret < 0)
		goto out;

	/* after reading the checksum, set it to 0 for checksum calculation */
	ret = set_value_int(*buffer, structure, 0, DEFAULT_CHKSUM_ELEMENT);
	if (ret < 0)
		goto out;

	checksum = calc_crc32(*buffer, structure->size);

	if ((magic != input_magic) ||
	    (version != input_version)) {
		fprintf(stderr, "incompatible binary file\n"
			"expected 0x%08x 0x%08x\n"
			"got 0x%08x 0x%08x\n",
			magic, version, input_magic, input_version);
		ret = -1;
		goto out;
	}

	if (!ignore_checksum && (checksum != input_checksum)) {
		fprintf(stderr, "corrupted binary file\n"
			"expected checksum 0x%08x got 0x%08x\n",
			checksum, input_checksum);
		ret = -1;
		goto out;
	}


out:
	return ret;
}

static int translate_ini(char **element_str, char **value_array)
{
	int i, ret = 0;
	char *translated_array, *translated_value, *value_str;
	size_t len;

	for (i = 0; i < n_dict_entries; i++)
		if (!strcmp(dict[i].ini_str, *element_str)) {
			free(*element_str);
			*element_str = strdup(dict[i].element_str);
			if (!element_str) {
				fprintf(stderr, "couldn't allocate memory\n");
				ret = -1;
				goto out;
			}
		}

	translated_array = malloc(MAX_ARRAY_STR_LEN);
	translated_value = malloc(MAX_VALUE_STR_LEN);
	if(!translated_array || !translated_value) {
		fprintf(stderr, "couldn't allocate memory\n");
		ret = -1;
		goto out;
	}

	translated_array[0] = '\0';

	value_str = strtok(*value_array, " \t");
	while (value_str) {
		len = snprintf(translated_value, MAX_VALUE_STR_LEN, "0x%s,",
			       value_str);
		if (len >= MAX_VALUE_STR_LEN) {
			fprintf(stderr, "value string is too long!\n");
			ret = -1;
			goto out_free;
		}

		len = strlen(translated_value) + strlen(translated_array);
		if (len >= MAX_ARRAY_STR_LEN) {
			fprintf(stderr, "value array is too long!\n");
			ret = -1;
			goto out_free;
		}

		strncat(translated_array, translated_value, MAX_ARRAY_STR_LEN);

		value_str = strtok(NULL, " \t");
	}

	/* remove last comma */
	translated_array[strlen(translated_array) - 1] = '\0';

	free(*value_array);
	*value_array = strdup(translated_array);
	if (!*value_array) {
		fprintf(stderr, "couldn't allocate memory\n");
		ret = -1;
		goto out_free;
	}

out_free:
	free(translated_array);
	free(translated_value);
out:
	return ret;
}

static int parse_dict(const char *filename)
{
	regex_t r;
	FILE *file;
	unsigned int parse_errors = 0, line_number = 0;
	int ret;

	file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Couldn't open file '%s'\n", filename);
		return -1;
	}

	ret = regcomp(&r, DICT_PATTERN, REG_EXTENDED);
	if (ret < 0)
		goto out;

	while (!feof(file)) {
		char *ini_str = NULL, *element_str = NULL, *line = NULL;
		char *elim;
		regmatch_t m[3];
		size_t len;

		ret = getline(&line, &len, file);
		if (ret < 0) {
			ret = 0;
			break;
		}

		line_number++;

		/* eliminate comments */
		elim = strchr(line, '#');
		if (elim)
			*elim = '\0';

		/* eliminate newline */
		elim = strchr(line, '\n');
		if (elim)
			*elim = '\0';

		if (!strlen(line))
			goto cont;

		if (regexec(&r, line, 3, m, 0)) {
			fprintf(stderr, "line %d: invalid syntax: '%s'\n",
				line_number, line);

			parse_errors++;
			goto cont;
		}

		ini_str =
			strndup(line + m[1].rm_so, m[1].rm_eo - m[1].rm_so);

		element_str = strndup(line + m[2].rm_so,
				      m[2].rm_eo - m[2].rm_so);

		dict = realloc(dict, ++n_dict_entries *
			       sizeof(struct dict_entry));
		if (!dict) {
			free(line);
			ret = -1;
			goto out_free;
		}

		dict[n_dict_entries - 1].ini_str = ini_str;
		dict[n_dict_entries - 1].element_str = element_str;

	cont:
		free(line);
	};

out_free:
	regfree(&r);
out:
	if (parse_errors) {
		fprintf(stderr,
			"%d errors found, output file was not generated.\n",
			parse_errors);
		ret = -1;
	}

	fclose(file);
	return ret;
}

static int parse_text_file(char *conf_buffer, struct structure *structure,
			   const char *filename, enum text_file_type type)
{
	regex_t r;
	FILE *file;
	unsigned int parse_errors = 0, line_number = 0;
	int ret = -1;

	file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Couldn't open file '%s'\n", filename);
		return -1;
	}

	if (type == TEXT_FILE_CONF)
		ret = regcomp(&r, TEXT_CONF_PATTERN, REG_EXTENDED);
	else if (type == TEXT_FILE_INI)
		ret = regcomp(&r, TEXT_INI_PATTERN, REG_EXTENDED);

	if (ret < 0)
		goto out;

	while (!feof(file)) {
		char *element_str = NULL, *line = NULL, *elim;
		char *value_str = NULL, *value_array = NULL;
		regmatch_t m[3];
		struct element *element;
		long int value;
		int pos, i;
		size_t len;

		ret = getline(&line, &len, file);
		if (ret < 0) {
			ret = 0;
			break;
		}

		line_number++;

		/* eliminate comments */
		elim = strchr(line, '#');
		if (elim)
			*elim = '\0';

		/* eliminate newline */
		elim = strchr(line, '\n');
		if (elim)
			*elim = '\0';

		if (!strlen(line))
			goto cont;

		if (regexec(&r, line, 3, m, 0)) {
			fprintf(stderr, "line %d: invalid syntax: '%s'\n",
				line_number, line);

			parse_errors++;
			goto cont;
		}

		element_str =
			strndup(line + m[1].rm_so, m[1].rm_eo - m[1].rm_so);

		value_array = strndup(line + m[2].rm_so, m[2].rm_eo - m[2].rm_so);

		if (type == TEXT_FILE_INI) {
			ret = translate_ini(&element_str, &value_array);
			if (ret < 0) {
				fprintf(stderr,
					"line %d: couldn't translate INI file: '%s'\n",
					line_number, line);
				parse_errors++;
				goto cont_free;
			}
		}

		pos = get_element_pos(structure, element_str, &element);
		if (pos < 0) {
			fprintf(stderr, "line %d: couldn't find element %s\n",
				line_number, element_str);
			parse_errors++;
			goto cont_free;
		} else if (element->type >= STRUCT_BASE) {
			fprintf(stderr,
				"line %d: setting entire structures is not supported.\n",
				line_number);
			parse_errors++;
			goto cont_free;
		}

		i = 0;

		value_str = strtok(value_array, ",");
		while (value_str) {
			if (++i > element->array_size)
				break;
			value = strtoul(value_str, NULL, 0);
			ret = set_data(element, conf_buffer + pos, &value);

			pos += types[element->type].size;

			value_str = strtok(NULL, ",");
		}

		if (i != element->array_size) {
			fprintf(stderr,
				"line %d: invalid array size, expected %d got %d\n",
				line_number, element->array_size, i);
			parse_errors++;
			goto cont_free;
		}

	cont_free:
		free(element_str);
		free(value_array);

	cont:
		free(line);
	};

	regfree(&r);
out:
	if (parse_errors) {
		fprintf(stderr,
			"%d errors found, output file was not generated.\n",
			parse_errors);
		ret = -1;
	}

	fclose(file);
	return ret;
}

static int create_default(const char *conf_filename,
			  const char *output_filename,
			  void **conf_buf, struct structure *structure)
{
	int ret;

	*conf_buf = malloc(structure->size);
	if (!*conf_buf) {
		fprintf(stderr, "Couldn't allocate enough memory (%d)\n",
			structure->size);
		ret = -1;
		goto out;
	}

	/* set the magic for writing */
	ret = set_value_int(*conf_buf, structure,
			    magic,
			    DEFAULT_MAGIC_ELEMENT);
	if (ret < 0)
		goto out;

	/* set the version for writing */
	ret = set_value_int(*conf_buf, structure,
			    version,
			    DEFAULT_VERSION_ELEMENT);
	if (ret < 0)
		goto out;

	ret = parse_text_file(*conf_buf, structure,
			      conf_filename, TEXT_FILE_CONF);
	if (ret < 0)
		goto out;

	/* set the checksum for writing */
	ret = set_value_int(*conf_buf, structure,
			    calc_crc32(*conf_buf, structure->size),
			    DEFAULT_CHKSUM_ELEMENT);
	if (ret < 0)
		goto out;

	ret = write_file(output_filename, *conf_buf, structure->size);
	if (ret < 0)
		goto out;

out:
	if (ret < 0 && *conf_buf) free(*conf_buf);

	return ret;
}

#define SHORT_OPTIONS "S:s:b:i:o:g::G:C:I:phXD"

struct option long_options[] = {
	{ "binary-struct",	required_argument,	NULL,	'b' },
	{ "source-struct",	required_argument,	NULL,	'S' },
	{ "input-config",	required_argument,	NULL,	'i' },
	{ "output-config",	required_argument,	NULL,	'o' },
	{ "ignore-checksum",	no_argument,		NULL,	'X' },
	{ "create-default",	no_argument,		NULL,	'D' },
	{ "get",		optional_argument,	NULL,	'g' },
	{ "set",		required_argument,	NULL,	's' },
	{ "generate-struct",	required_argument,	NULL,	'G' },
	{ "parse-text-conf",	required_argument,	NULL,	'C' },
	{ "parse-ini",		required_argument,	NULL,	'I' },
	{ "print-struct",	no_argument,		NULL,	'p' },
	{ "help",		no_argument,		NULL,	'h' },
	{ 0, 0, 0, 0 },
};

int main(int argc, char **argv)
{
	void *header_buf = NULL;
	void *conf_buf = NULL;
	char *header_filename = NULL;
	char *binary_struct_filename = NULL;
	char *input_filename = NULL;
	char *output_filename = NULL;
	char *dict_filename = NULL;
	char *command_arg = NULL;
	struct structure *root_struct;
	int c, ret = 0;
	char command = 0;

	while (1) {
		c = getopt_long(argc, argv, SHORT_OPTIONS, long_options, NULL);

		if (c < 0)
			break;

		switch(c) {
		case 'S':
			header_filename = optarg;
			break;

		case 'b':
			binary_struct_filename = strdup(optarg);
			break;

		case 'i':
			input_filename = strdup(optarg);
			break;

		case 'o':
			output_filename = strdup(optarg);
			break;

		case 'X':
			ignore_checksum = 1;
			break;

		case 'D':
			/* Build default configuration bin file (default input) */
			if (output_filename) {
				fprintf(stderr,	"Cannot specify output file name with -D\n");
				print_usage(argv[0]);
				exit(-1);
			}
			else
				output_filename = strdup(DEFAULT_INPUT_FILENAME);
			/* Fall through */
		case 'G':
		case 'g':
		case 's':
		case 'C':
		case 'I':
			command_arg = optarg;
			/* Fall through */
		case 'p':
			if (command) {
				fprintf(stderr,
					"Only one command option is allowed, can't use -%c with -%c.\n",
					command, c);
				print_usage(argv[0]);
				exit(-1);
			}
			command = c;
			break;

		case 'h':
			print_usage(argv[0]);
			exit(0);

		default:
			print_usage(argv[0]);
			exit(-1);
		}
	}

	if (!input_filename)
		input_filename = strdup(DEFAULT_INPUT_FILENAME);

	if (!dict_filename)
		dict_filename = strdup(DEFAULT_DICT_FILENAME);

	if (!output_filename)
		output_filename = strdup(DEFAULT_OUTPUT_FILENAME);

	if (header_filename && binary_struct_filename) {
		fprintf(stderr,
			"Can't specify both source struct and binary struct\n");
		ret = -1;
		goto out;
	}

	if (!header_filename && !binary_struct_filename) {
		binary_struct_filename = strdup(DEFAULT_BIN_FILENAME);
		ret = read_binary_struct(binary_struct_filename);
		if (ret < 0)
			goto out;
	}

	if (binary_struct_filename) {
		ret = read_binary_struct(binary_struct_filename);
		if (ret < 0)
			goto out;
	}

	if (header_filename) {
		ret = read_file(header_filename, &header_buf, 0);
		if (ret < 0)
			goto out;

		ret = parse_header(header_buf);
		if (ret < 0)
			goto out;
	}

	root_struct = get_struct(DEFAULT_ROOT_STRUCT);
	if (!root_struct) {
		fprintf(stderr,
			"error: root struct (%s) is not defined\n",
			DEFAULT_ROOT_STRUCT);
		ret = -1;
		goto out;
	}

	switch (command) {
	case 'D':
		/* Generate default configuration bin file */
		ret = create_default(DEFAULT_CONF_FILENAME, output_filename,
			&conf_buf, root_struct);

		if (ret < 0)
			goto out;

		break;
	case 'G':
		if (!header_buf) {
			fprintf(stderr, "Source struct file must be specified.\n");
			ret = -1;
			break;
		}

		ret = generate_struct(command_arg);
		break;

	case 'g':
		ret = read_input(input_filename, &conf_buf, root_struct);
		if (ret < 0)
			goto out;

		get_value(conf_buf, root_struct, command_arg);
		break;

	case 's':
		ret = read_input(input_filename, &conf_buf, root_struct);
		if (ret < 0)
			goto out;

		ret = set_value(conf_buf, root_struct, command_arg);
		if (ret < 0)
			goto out;

		/* update the checksum for writing */
		ret = set_value_int(conf_buf, root_struct,
				    calc_crc32(conf_buf, root_struct->size),
				    DEFAULT_CHKSUM_ELEMENT);
		if (ret < 0)
			goto out;

		ret = write_file(output_filename, conf_buf, root_struct->size);
		if (ret < 0)
			goto out;

		break;

	case 'C':
		ret = read_input(input_filename, &conf_buf, root_struct);
		if (ret < 0)
			goto out;

		ret = parse_text_file(conf_buf, root_struct,
				      command_arg, TEXT_FILE_CONF);
		if (ret < 0)
			goto out;

		/* update the checksum for writing */
		ret = set_value_int(conf_buf, root_struct,
				    calc_crc32(conf_buf, root_struct->size),
				    DEFAULT_CHKSUM_ELEMENT);
		if (ret < 0)
			goto out;

		ret = write_file(output_filename, conf_buf, root_struct->size);
		if (ret < 0)
			goto out;

		break;

	case 'I':
		ret = read_input(input_filename, &conf_buf, root_struct);
		if (ret < 0)
			goto out;

		ret = parse_dict(dict_filename);
		if (ret < 0)
			goto out;

		ret = parse_text_file(conf_buf, root_struct,
				      command_arg, TEXT_FILE_INI);
		if (ret < 0)
			goto out;

		/* update the checksum for writing */
		ret = set_value_int(conf_buf, root_struct,
				    calc_crc32(conf_buf, root_struct->size),
				    DEFAULT_CHKSUM_ELEMENT);
		if (ret < 0)
			goto out;

		ret = write_file(output_filename, conf_buf, root_struct->size);
		if (ret < 0)
			goto out;

		break;

	case 'p':
		get_value(NULL, root_struct, NULL);
		break;

	default:
		print_usage(argv[0]);
		break;
	}

	free_file(header_buf);
	free_file(conf_buf);

	free_structs();
	free_dict();
out:
	free(input_filename);
	free(output_filename);
	free(dict_filename);
	free(binary_struct_filename);

	exit(ret);
}
