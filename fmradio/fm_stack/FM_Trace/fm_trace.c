/*
 * FM Sample Application
 *
 * Copyright 2001-2008 Texas Instruments, Inc. - http://www.ti.com/
 *
 * Written by Ohad Ben-Cohen <ohad@bencohen.org>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and  
 * limitations under the License.
 */

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <string.h>

#include "fmc_types.h"
#include "fmc_defs.h"
#include "fm_trace.h"

static FILE *g_stack_log_file = NULL;
static FILE *g_app_log_file = NULL;
unsigned int g_stack_trace_threshold = FM_STACK_TRACE_THRESHOLD;
unsigned int g_app_trace_threshold = FM_APP_TRACE_THRESHOLD;

static void __fm_trace(char output_device, char *buffer)
{
	if ((output_device & FM_TRACE_TO_STACK_LOGFILE) && g_stack_log_file)
		fputs(buffer, g_stack_log_file);

	if ((output_device & FM_TRACE_TO_APP_LOGFILE) && g_app_log_file)
		fputs(buffer, g_app_log_file);

	if (output_device & FM_TRACE_TO_STDOUT)
		fputs(buffer, stdout);

	if (output_device & FM_TRACE_TO_STDERR)
		fputs(buffer, stdout);
}

void fm_trace_out(char output_device, const char *function,
			const char *trailing_message, const char *fmt, ...)
{
	unsigned int res, space_left, offset_shift, truncated = 0;
	char *buffer_offset, buffer[512];
	va_list args;
	time_t t;
	struct tm *tmp;

	/* add time stamp */
	t = time(NULL);
	tmp = localtime(&t);
	if (tmp)
		strftime(buffer, sizeof(buffer), FM_TRACE_TIME_FMT, tmp);
	offset_shift = FM_TRACE_TIME_LEN;
	buffer_offset = buffer + offset_shift;
	space_left = sizeof(buffer) - offset_shift;

	/* do we need to add a function name ? */
	if (function) {
		res = snprintf(buffer_offset, space_left, "%s: ", function);
		if (res >= space_left) {
			truncated = 1;
			goto out;
		}
		offset_shift = (2 + strlen(function));
		buffer_offset += offset_shift;
		space_left -= offset_shift;
	}

	/* add the message core */
	va_start(args, fmt);
	res = vsnprintf(buffer_offset, space_left, fmt, args);
	va_end(args);
	if (res >= space_left) {
		truncated = 1;
		goto out;
	}

	offset_shift = strlen(buffer);

	/* do we need to add a trailing message ? */
	if (trailing_message) {
		snprintf(buffer + offset_shift, sizeof(buffer) - offset_shift,
							": %s", trailing_message);
		offset_shift += strlen(trailing_message)+2;
	}

	/* always end with a newline */
	if (offset_shift + 1 >= sizeof(buffer)) {
		offset_shift = sizeof(buffer) - 2;
		truncated = 1;
	}
	buffer[offset_shift] = '\n';
	buffer[offset_shift + 1] = '\0';

out:
	__fm_trace(output_device, buffer);

	/* have we truncated the message due to lack of buffer space ? */
	if (truncated) {
		snprintf(buffer + FM_TRACE_TIME_LEN,
				sizeof(buffer) - FM_TRACE_TIME_LEN,
				"%s: previous message was truncated\n",
				__FUNCTION__);
		__fm_trace(output_device, buffer);
	}
}

int fm_trace_init(void)
{
	int ret = FMC_STATUS_SUCCESS;
	FM_BEGIN();

	/* allow multiple initializations. good for early fm application tracing */
	if (g_stack_log_file || g_app_log_file)
		goto out;

	g_stack_log_file = fopen(FM_STACK_LOG_FILE, "a");
	if (!g_stack_log_file) {
		FM_ERROR_SYS("failed to open stack log file: %s",
							FM_STACK_LOG_FILE);
		ret = FMC_STATUS_FAILED;
		goto out;
	}

	/* set tracing to be in line-buffered mode */
	setlinebuf(g_stack_log_file);

	FM_TRACE("++++++++++++++++++++++++++++++++++++++");
	FM_TRACE("hello FM stack !");

	g_app_log_file = fopen(FM_APP_LOG_FILE, "a");
	if (!g_app_log_file) {
		FM_ERROR_SYS("failed to open app log file: %s", FM_APP_LOG_FILE);
		ret = FMC_STATUS_FAILED;
		fclose(g_stack_log_file);
		goto out;
	}

	/* set tracing to be in line-buffered mode */
	setlinebuf(g_app_log_file);

	FMAPP_TRACE("++++++++++++++++++++++++++++++++++++++");
	FMAPP_TRACE("hello FM app !");

out:
	FM_END();
	return ret;
}

void fm_trace_deinit(void)
{
	FM_BEGIN();

	FM_TRACE("FM stack - bye");

	if (g_stack_log_file)
		fclose(g_stack_log_file);

	if (g_app_log_file)
		fclose(g_app_log_file);

	g_stack_log_file = g_app_log_file = NULL;

	FM_END();
}

