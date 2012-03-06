/*
 * Linux FM Sample and Testing Application for TI's FM Stack
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

#ifndef __FM_APP_H
#define __FM_APP_H

#define FMAPP_WELCOME_STR	"Texas Instruments FM sample & testing application, welcome !"

#define FMAPP_VOLUME_MIN	0
#define FMAPP_VOLUME_MAX	70
#define FMAPP_VOLUME_INIT	25

#define FMAPP_VOLUME_LEVELS	71
#define FMAPP_VOLUME_DELTA 	1
#define FMAPP_VOLUME_LVL_INIT 	FMAPP_VOLUME_INIT

#define FMAPP_RX_MODE		0
#define FMAPP_TX_MODE		1

#define FMAPP_BATCH		0
#define FMAPP_INTERACTIVE	1

#define OUT
#define IN

/*
 * Changes/Additions for Android
 */
#include <cutils/properties.h>
/*
 * set the FM-Enable GPIO to Low[0]
 */
int set_fm_chip_enable(int enable);
/*
 * The FM Enable GPIO is accessed as a rfkill switch
 */
static int get_rfkill_path(char **rfkill_state_path);

#endif
