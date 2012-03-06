/*
 * FM Sample Application
 *
 * Copyright 2001-2008 Texas Instruments, Inc. - http://www.ti.com/
 *
 *  Written by Ohad Ben-Cohen <ohad@bencohen.org>
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

#ifndef __FM_TRACE_H
#define __FM_TRACE_H

#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* activate debug traces */
#define FM_STACK_DEBUG

/* trace levels are between 0 (highest priority) to 7 (lowest priority) */
#define HIGHEST_TRACE_LVL		(0)
#define DEFAULT_TRACE_LVL		(4)
#define DEFAULT_BEGIN_LVL		(5)
#define LOWEST_TRACE_LVL		(7)

/* trace messages with value equal or higher than this level will be ignored */
/* value must be between 1 to 8 */
#define FM_STACK_TRACE_THRESHOLD	(5)
#define FM_APP_TRACE_THRESHOLD		(5)
#define MAX_THRESHOLD_LVL		(LOWEST_TRACE_LVL+1)
#define MIN_THRESHOLD_LVL		(HIGHEST_TRACE_LVL+1)

/* bit maps to output_device parameter, which controls to which
 * output devices the traces are going to be dumped to */
#define FM_TRACE_TO_STDOUT		(1)
#define FM_TRACE_TO_STDERR		(2)
#define FM_TRACE_TO_STACK_LOGFILE	(4)
#define FM_TRACE_TO_STACK_LOG_ERR	(6)
#define FM_TRACE_TO_APP_LOGFILE		(8)
#define FM_TRACE_TO_APP_LOG_ERR		(10)

#define FM_STACK_LOG_FILE		"fmstack.log"
#define FM_APP_LOG_FILE			"fmapp.log"

#define FM_ERR_BUF			_fm_trace_error_buffer
#define FM_TRACE_LVL_VAR		_fm_func_trace_level

#ifdef FM_STACK_DEBUG
/* used to print debug messages to log file with controlled trace level */
#define FM_TRACE_L(lvl, x...)						\
	do {								\
		if (lvl < g_stack_trace_threshold)			\
			fm_trace_out(FM_TRACE_TO_STACK_LOGFILE,		\
							__FUNCTION__, 	\
							NULL, x);	\
	} while (0)

/* used to print debug messages to log file using a default trace level */
#define FM_TRACE(x...)		FM_TRACE_L(DEFAULT_TRACE_LVL, x)

/*
 * define local trace level and outputs function begin msg with controlled
 * trace level
 */
#define FM_BEGIN_L(lvl)							\
	unsigned char FM_TRACE_LVL_VAR = lvl;					\
	do {								\
		FM_TRACE_L(FM_TRACE_LVL_VAR, "begin");			\
	} while (0)

/*
 * define local trace level and outputs function begin msg using default trace
 * level
 */
#define FM_BEGIN(x)		FM_BEGIN_L(DEFAULT_BEGIN_LVL)

/* outputs functoin end msg; uses local trace level defined by FM_BEGIN macro */
#define FM_END(x)							\
	do {								\
		FM_TRACE_L(FM_TRACE_LVL_VAR, "end");			\
	} while (0)
#else
#define FM_TRACE(x...)		do {} while (0)
#define FM_BEGIN(x)		do {} while (0)
#define FM_END(x)		do {} while (0)
#endif

/* prints detailed system error msg to stack logfile */
#define FM_ERROR_SYS(x...)						\
	do {								\
		char FM_ERR_BUF[128] = {0};				\
		strerror_r(errno, FM_ERR_BUF, sizeof(FM_ERR_BUF));	\
		fm_trace_out(FM_TRACE_TO_STACK_LOG_ERR, __FUNCTION__,	\
		FM_ERR_BUF, x);						\
	} while (0)

/* prints detailed user error msg to stack logfile */
#define FM_ERROR(x...)							\
	do {								\
		fm_trace_out(FM_TRACE_TO_STACK_LOG_ERR, __FUNCTION__,	\
							NULL, x);	\
	} while (0)

/* if condition is false, prints error message and brutally exits */
#define FM_ASSERT(x)							\
	do {								\
		if ((x))						\
			break;						\
		FM_ERROR("critical assertion failed! (%s, line %d)",	\
							__FILE__,	\
							__LINE__);	\
		exit(1);						\
	} while (0)

/* these macros are for the FM Application;
 * they use the fm app log file and trace threshold */
#define FMAPP_MSG(x...)							\
	do {								\
		fm_trace_out(FM_TRACE_TO_STDOUT, NULL, NULL, x);	\
	} while (0)

#define FMAPP_MSG_TO_FILE(x...)						\
	do {								\
		fm_trace_out(FM_TRACE_TO_APP_LOGFILE, NULL, NULL, x);	\
	} while (0)

#define FMAPP_TRACE_L(lvl, x...)					\
	do {								\
		if (lvl < g_app_trace_threshold)			\
			fm_trace_out(FM_TRACE_TO_APP_LOGFILE,		\
							__FUNCTION__,	\
							NULL, x);	\
	} while (0)

#define FMAPP_TRACE(x...)	FMAPP_TRACE_L(DEFAULT_TRACE_LVL, x)

#define FMAPP_ERROR_SYS(x...)						\
	do {								\
		char FM_ERR_BUF[128] = {0};				\
		strerror_r(errno, FM_ERR_BUF, sizeof(FM_ERR_BUF));	\
		fm_trace_out(FM_TRACE_TO_APP_LOG_ERR, __FUNCTION__,	\
							FM_ERR_BUF, x);	\
	} while (0)

#define FMAPP_ERROR(x...)						\
	do {								\
		fm_trace_out(FM_TRACE_TO_APP_LOG_ERR, __FUNCTION__,	\
								NULL,	\
								x);	\
	} while (0)

#define FMAPP_BEGIN_L(lvl)						\
	unsigned char FM_TRACE_LVL_VAR = lvl;					\
	do {								\
		FMAPP_TRACE_L(FM_TRACE_LVL_VAR, "begin");		\
	} while (0)

#define FMAPP_BEGIN(x)		FMAPP_BEGIN_L(DEFAULT_BEGIN_LVL)

#define FMAPP_END(x)							\
	do {								\
		FMAPP_TRACE_L(FM_TRACE_LVL_VAR, "end");			\
	} while (0)

#define FM_TRACE_TIME_FMT	"%b %d %H:%M:%S "
#define FM_TRACE_TIME_LEN	(3 + 2 + 2 + 2 + 2 + /* seperators */ 5)

void fm_trace_out(char output_device, const char *function,
			const char *trailing_message, const char *fmt, ...);
int fm_trace_init(void);
void fm_trace_deinit(void);

extern unsigned int g_stack_trace_threshold;
extern unsigned int g_app_trace_threshold;

#endif
