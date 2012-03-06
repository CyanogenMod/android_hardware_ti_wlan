/*
 * Linux FM Sample and Testing Application for TI's FM stack
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

#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "bluetooth.h"
#include "hci.h"
#include "hci_lib.h"
#include <time.h>
#include <getopt.h>

/* stack includes */
#include <fm_rx.h>
#include <fm_tx.h>
#include <fmc_debug.h>
#include "fmc_core.h"
#include "fmc_defs.h"

#ifdef ANDROID
/* lib_alsa ALSA Include */
#include "asoundlib.h"
#endif

/* local app */
#include "fm_trace.h"
#include "fmc_common.h"
#include "fm_app.h"

/* transient stage */
typedef FmcStatus fm_status;
typedef FmRxStatus fm_rx_status;
typedef FmRxContext fm_rx_context_s;
typedef FmRxEvent fm_rx_event_s;
typedef FmTxStatus fm_tx_status;
typedef FmTxContext fm_tx_context_s;
typedef FmTxEvent fm_tx_event_s;
/* FM state */
static volatile int 		g_keep_running = 1;
static int 			g_fmapp_hci_dev = 0;
static FMC_UINT 		g_fmapp_volume;
static FmTxPowerLevel	g_fmapp_tx_power;
static FmcFreq 		g_fmapp_freq = 0;
static FMC_INT 		g_fmapp_seek_rssi;
static int 			g_fmapp_startup_script_fd = -1;
static char 		g_fmapp_power_mode;
static char 		g_fmapp_tx_transission_mode;
static char 		g_fmapp_now_initializing;
static FmRxCmdType 	g_fmapp_rds;
static FMC_UINT 		g_fmapp_mute_mode;
static FmcEmphasisFilter 	g_fmapp_emphasis_filter;
static FmRxRfDependentMuteMode g_fmapp_rfmute_mode;
static char 		g_fmapp_rxtx_mode = FMAPP_RX_MODE;
static FmRxRdsAfSwitchMode g_fmapp_af_mode;
static FmRxMonoStereoMode 	g_fmapp_stereo_mode;
static FMC_UINT 		g_fmapp_band_mode;
static char 		g_fmapp_radio_text[1024] = {0};
static char 		g_fmapp_af_list[1024] = {0};
static FmcAudioRouteMask 	g_fmapp_audio_route;
static FmRxRdsAfSwitchMode g_fmapp_rds_af_switch;
static FmcRdsGroupTypeMask g_fmapp_rds_group_mask;
static FmcRdsSystem 	g_fmapp_rds_system;
static FmTxRdsTransmissionMode g_fmapp_tx_rds_transission_mode;
static FmcRdsRepertoire g_fmapp_tx_rds_text_repertoire;
static FmcRdsPsDisplayMode g_fmapp_tx_rds_ps_display_mode;
static FmTxRdsTransmittedGroupsMask g_fmapp_tx_rds_transmitted_groups_mask;
static FmcRdsMusicSpeechFlag g_fmapp_tx_music_speech_flag;
static FmRxCmdType	g_fmapp_audio;

/*
fm_status set_fmapp_audio_routing(fm_rx_context_s *);
fm_status unset_fmapp_audio_routing(fm_rx_context_s *);
*/

#define STATUS_DBG_STR(x)						\
		g_fmapp_rxtx_mode == FMAPP_TX_MODE ?			\
		FMC_DEBUG_FmTxStatusStr(x) : FMC_DEBUG_FmcStatusStr(x)

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

void validate_uint_boundaries(FMC_UINT *variable, FMC_UINT minimum, FMC_UINT maximum)
{
	if (*variable < minimum)
		*variable = minimum;
	if (*variable > maximum)
		*variable = maximum;
}

void validate_int_boundaries(FMC_INT *variable, FMC_INT minimum, FMC_INT maximum)
{
	if (*variable < minimum)
		*variable = minimum;
	if (*variable > maximum)
		*variable = maximum;
}

static void fmapp_initialize_global_status(void)
{
	FMAPP_BEGIN();

	g_fmapp_power_mode = 0;
	g_fmapp_rds_af_switch =  FM_RX_RDS_AF_SWITCH_MODE_OFF;
	g_fmapp_rds_group_mask = FM_RDS_GROUP_TYPE_MASK_ALL;
	g_fmapp_rds_system = FM_RDS_SYSTEM_RBDS;
	g_fmapp_audio_route = FMC_AUDIO_ROUTE_MASK_NONE;
	g_fmapp_band_mode = FMC_BAND_EUROPE_US;
	g_fmapp_volume = FMAPP_VOLUME_INIT;
	g_fmapp_rds = FM_RX_CMD_DISABLE_RDS;
	g_fmapp_mute_mode = FMC_NOT_MUTE;
	g_fmapp_emphasis_filter = FMC_EMPHASIS_FILTER_50_USEC;
	g_fmapp_rfmute_mode = FM_RX_RF_DEPENDENT_MUTE_ON;
	g_fmapp_stereo_mode = FM_RX_STEREO_MODE;
	g_fmapp_af_mode = FM_RX_RDS_AF_SWITCH_MODE_ON;
	g_fmapp_now_initializing = 0;
	g_fmapp_tx_transission_mode = 0;
	g_fmapp_tx_rds_transission_mode =  FMC_RDS_TRANSMISSION_MANUAL;
	g_fmapp_tx_rds_text_repertoire = FMC_RDS_REPERTOIRE_G0_CODE_TABLE;
	g_fmapp_tx_rds_ps_display_mode = FMC_RDS_PS_DISPLAY_MODE_STATIC;
	g_fmapp_tx_rds_transmitted_groups_mask = FM_TX_RDS_RADIO_TRANSMITTED_GROUP_PS_MASK;
	g_fmapp_tx_music_speech_flag = FMC_RDS_MUSIC;
	g_fmapp_audio = FM_RX_CMD_DISABLE_AUDIO;

	memset(g_fmapp_radio_text, 0, sizeof(g_fmapp_radio_text));
	memset(g_fmapp_af_list, 0, sizeof(g_fmapp_af_list));

	FMAPP_END();
}

static void fmapp_termination_handler(int sig)
{
	FMAPP_BEGIN();

	/* silence compiler warning */
	(void)sig;

	FMAPP_MSG("breaking...");
	g_keep_running = 0;

	FMAPP_END();
}

static void fmapp_display_band(FMC_U32 value)
{
	FMAPP_BEGIN();

	FMAPP_MSG("Band is %s", FMC_DEBUG_BandStr(value));

	FMAPP_END();
}

static void fmapp_display_volume_bar(void)
{
	char volume_bar[FMAPP_VOLUME_LEVELS+1] = {0};
	unsigned int i;
	FMAPP_BEGIN();

	for (i = 0; i < g_fmapp_volume/FMAPP_VOLUME_DELTA; i++)
		volume_bar[i] = '#';

	FMAPP_MSG("Volume: %s", volume_bar);

	FMAPP_END();
}

static void fmapp_display_mute_mode(FMC_U32 value)
{
	FMAPP_BEGIN();

	switch (value) {
	case FMC_NOT_MUTE:
		FMAPP_MSG("Mute Off");
		break;
	case FMC_MUTE:
		FMAPP_MSG("Mute On");
		break;
	case FMC_ATTENUATE:
		FMAPP_MSG("Attenuate Voice");
		break;
	default:
		FMAPP_ERROR("illegal mute mode");
		break;
	}

	FMAPP_END();
}

static void fmapp_display_rds_system(FMC_U32 value)
{
	FMAPP_BEGIN();

	switch (value) {
	case FM_RDS_SYSTEM_RBDS:
		FMAPP_MSG("FM_RDS_SYSTEM_RBDS");
		break;
	case FM_RDS_SYSTEM_RDS:
		FMAPP_MSG("FM_RDS_SYSTEM_RDS");
		break;
	default:
		FMAPP_ERROR("illegal rds system");
		break;
	}

	FMAPP_END();
}

static void fmapp_display_rds_group_mask(FMC_U32 value)
{
	FMAPP_BEGIN();

	switch (value) {
	case FM_RDS_GROUP_TYPE_MASK_0A:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_0A"); break;
	case FM_RDS_GROUP_TYPE_MASK_0B:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_0B"); break;
	case FM_RDS_GROUP_TYPE_MASK_1A:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_1A"); break;
	case FM_RDS_GROUP_TYPE_MASK_1B:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_1B"); break;
	case FM_RDS_GROUP_TYPE_MASK_2A:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_2A"); break;
	case FM_RDS_GROUP_TYPE_MASK_2B:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_2B"); break;
	case FM_RDS_GROUP_TYPE_MASK_3A:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_3A"); break;
	case FM_RDS_GROUP_TYPE_MASK_3B:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_3B"); break;
	case FM_RDS_GROUP_TYPE_MASK_4A:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_4A"); break;
	case FM_RDS_GROUP_TYPE_MASK_4B:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_4B"); break;
	case FM_RDS_GROUP_TYPE_MASK_5A:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_5A"); break;
	case FM_RDS_GROUP_TYPE_MASK_5B:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_5B"); break;
	case FM_RDS_GROUP_TYPE_MASK_6A:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_6A"); break;
	case FM_RDS_GROUP_TYPE_MASK_6B:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_6B"); break;
	case FM_RDS_GROUP_TYPE_MASK_7A:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_7A"); break;
	case FM_RDS_GROUP_TYPE_MASK_7B:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_7B"); break;
	case FM_RDS_GROUP_TYPE_MASK_8A:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_8A"); break;
	case FM_RDS_GROUP_TYPE_MASK_8B:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_8B"); break;
	case FM_RDS_GROUP_TYPE_MASK_9A:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_9A"); break;
	case FM_RDS_GROUP_TYPE_MASK_9B:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_9B"); break;
	case FM_RDS_GROUP_TYPE_MASK_10A:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_10A"); break;
	case FM_RDS_GROUP_TYPE_MASK_10B:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_10B"); break;
	case FM_RDS_GROUP_TYPE_MASK_11A:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_11A"); break;
	case FM_RDS_GROUP_TYPE_MASK_11B:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_11B"); break;
	case FM_RDS_GROUP_TYPE_MASK_12A:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_12A"); break;
	case FM_RDS_GROUP_TYPE_MASK_12B:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_12B"); break;
	case FM_RDS_GROUP_TYPE_MASK_13A:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_13A"); break;
	case FM_RDS_GROUP_TYPE_MASK_13B:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_13B"); break;
	case FM_RDS_GROUP_TYPE_MASK_14A:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_14A"); break;
	case FM_RDS_GROUP_TYPE_MASK_14B:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_14B"); break;
	case FM_RDS_GROUP_TYPE_MASK_15A:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_15A"); break;
	case FM_RDS_GROUP_TYPE_MASK_15B:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_15B"); break;
	case FM_RDS_GROUP_TYPE_MASK_NONE:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_NONE"); break;
	case FM_RDS_GROUP_TYPE_MASK_ALL:
		FMAPP_MSG("FM_RDS_GROUP_TYPE_MASK_ALL"); break;
	default:
		FMAPP_ERROR("illegal rds af switch");
		break;
	}

	FMAPP_END();
}

static void fmapp_display_rds_af_switch(FMC_U32 value)
{
	FMAPP_BEGIN();

	switch (value) {
	case FM_RX_RDS_AF_SWITCH_MODE_ON:
		FMAPP_MSG("FM_RX_RDS_AF_SWITCH_MODE_ON");
		break;
	case FM_RX_RDS_AF_SWITCH_MODE_OFF:
		FMAPP_MSG("FM_RX_RDS_AF_SWITCH_MODE_OFF");
		break;
	default:
		FMAPP_ERROR("illegal rds af switch");
		break;
	}

	FMAPP_END();
}

#if 0
static void fmapp_display_audio_routing(FMC_U32 value)
{
	FMAPP_BEGIN();

	switch (value) {
	case FMC_AUDIO_ROUTE_MASK_I2S:
		FMAPP_MSG("FMC_AUDIO_ROUTE_MASK_I2S");
		break;
	case FMC_AUDIO_ROUTE_MASK_ANALOG:
		FMAPP_MSG("FMC_AUDIO_ROUTE_MASK_ANALOG");
		break;
	case FMC_AUDIO_ROUTE_MASK_NONE:
		FMAPP_MSG("FMC_AUDIO_ROUTE_MASK_NONE");
		break;
	case FMC_AUDIO_ROUTE_MASK_ALL:
		FMAPP_MSG("FMC_AUDIO_ROUTE_MASK_ALL");
		break;
	default:
		FMAPP_ERROR("illegal audio routing");
		break;
	}

	FMAPP_END();
}
#endif

static void fmapp_display_emphasis_filter(FMC_U32 value)
{
	FMAPP_BEGIN();

	switch (value) {
	case FMC_EMPHASIS_FILTER_NONE:
		FMAPP_MSG("EMPHASIS FILTER NONE");
		break;
	case FMC_EMPHASIS_FILTER_50_USEC:
		FMAPP_MSG("EMPHASIS FILTER 50 USEC");
		break;
	case FMC_EMPHASIS_FILTER_75_USEC:
		FMAPP_MSG("EMPHASIS FILTER 75 USEC");
		break;
	default:
		FMAPP_ERROR("illegal emphasis filter");
		break;
	}

	FMAPP_END();
}

static void fmapp_display_rfmute_mode(FMC_U32 value)
{
	FMAPP_BEGIN();

	switch (value) {
	case FM_RX_RF_DEPENDENT_MUTE_ON:
		FMAPP_MSG("RF Dependent Mute On");
		break;
	case FM_RX_RF_DEPENDENT_MUTE_OFF:
		FMAPP_MSG("RF Dependent Mute Off");
		break;
	default:
		FMAPP_ERROR("illegal rf mute mode");
		break;
	}

	FMAPP_END();
}

static void fmapp_display_stop_seek(FMC_U32 frequency)
{
	FMAPP_BEGIN();

	float freq = ((float)frequency) / 1000;

	FMAPP_MSG("Seek stopped at %2.1f FM", freq);

	FMAPP_END();
}

static void fmapp_display_seek(FMC_U32 frequency, fm_status status)
{
	FMAPP_BEGIN();

	float freq = ((float)frequency) / 1000;

	if (status == FM_RX_STATUS_SEEK_REACHED_BAND_LIMIT ||
					status == FM_RX_STATUS_SEEK_STOPPED)
		FMAPP_MSG("%s at %2.1f FM", STATUS_DBG_STR(status), freq);
	else
		FMAPP_MSG("Seeked to %2.1f FM", freq);

	FMAPP_END();
}

static void fmapp_radio_tuned_handler(FmcFreq frequency)
{
	FMAPP_BEGIN();

	float freq = ((float)frequency) / 1000;
	FMAPP_TRACE("freq tuned to %u", frequency);

	FMAPP_MSG("Tuned to %2.1f FM", freq);

	FMAPP_END();
}

static void fmapp_display_text_repertoire(FmcRdsRepertoire value)
{
	FMAPP_BEGIN();

	switch(value) {
	case FMC_RDS_REPERTOIRE_G0_CODE_TABLE:
		FMAPP_MSG("FMC_RDS_REPERTOIRE_G0_CODE_TABLE");
		break;
	case FMC_RDS_REPERTOIRE_G1_CODE_TABLE:
		FMAPP_MSG("FMC_RDS_REPERTOIRE_G1_CODE_TABLE");
		break;
	case FMC_RDS_REPERTOIRE_G2_CODE_TABLE:
		FMAPP_MSG("FMC_RDS_REPERTOIRE_G2_CODE_TABLE");
		break;
	default:
		FMAPP_MSG("Unknown FMC_RDS_REPERTOIRE");
		break;
	}

	FMAPP_END();
}

static void fmapp_display_generic_str(char *prefix, FMC_U8 *msg, FMC_UINT len)
{
	FMC_UINT how_much;

	how_much = min(len, sizeof(g_fmapp_radio_text)) - 1;

	memcpy(g_fmapp_radio_text, msg, how_much);

	g_fmapp_radio_text[how_much] = '\0';

	FMAPP_MSG("%s %s", prefix, g_fmapp_radio_text);
}

static void fmapp_display_transmitted_groups_mask(FMC_UINT value)
{
	switch(value) {
	case FM_TX_RDS_RADIO_TRANSMITTED_GROUP_PS_MASK:
		FMAPP_MSG("RDS RADIO TRANSMITTED GROUP PS MASK");
		break;
	case FM_TX_RDS_RADIO_TRANSMITTED_GROUP_RT_MASK:
		FMAPP_MSG("RDS RADIO TRANSMITTED GROUP RT MASK");
		break;
	case FM_TX_RDS_RADIO_TRANSMITTED_GROUP_ECC_MASK:
		FMAPP_MSG("RDS RADIO TRANSMITTED GROUP ECC MASK");
		break;
	default:
		FMAPP_MSG("Unknown RDS RADIO TRANSMITTED GROUP MASK");
		break;
	}
}

static void fmapp_tx_cmd_done_handler(const fm_tx_event_s *event)
{
	FMAPP_BEGIN();

	FMC_UINT cmd = event->p.cmdDone.cmdType;
	FmTxStatus status = event->status;
	FMC_UINT value = event->p.cmdDone.v.value;

	FMAPP_MSG_TO_FILE("%s(%d) done, value: %d (status: %s (%d))",
						FMC_DEBUG_FmTxCmdStr(cmd),
						cmd, value,
						FMC_DEBUG_FmTxStatusStr(status),
						status);

	if (status && status != FM_TX_STATUS_NOT_APPLICABLE) {
		FMAPP_ERROR("%s completed with error: %s", FMC_DEBUG_FmTxCmdStr(cmd),
						 FMC_DEBUG_FmTxStatusStr(status));
		goto out;
	}

	switch (cmd) {
	case FM_TX_CMD_DESTROY	:
		FMAPP_MSG("FM TX destroyed");
		break;

	case FM_TX_CMD_ENABLE:
		FMAPP_MSG("FM TX enabled");
		g_fmapp_now_initializing = 0;
		break;

	case FM_TX_CMD_DISABLE:
		FMAPP_MSG("FM TX disabled");
		fmapp_initialize_global_status();
		break;

	case FM_TX_CMD_SET_INTERRUPT_MASK:
		FMAPP_MSG("FM_TX_CMD_SET_INTERRUPT_MASK");
		break;

	case FM_TX_CMD_GET_INTERRUPT_SATUS:
		FMAPP_MSG("FM_TX_CMD_GET_INTERRUPT_SATUS");
		break;

	case FM_TX_CMD_START_TRANSMISSION:
		FMAPP_MSG("Started transmission");
		break;

	case FM_TX_CMD_STOP_TRANSMISSION:
		FMAPP_MSG("Stopped transmission");
		break;

	case FM_TX_CMD_TUNE:
		FMAPP_MSG("Set frequency to %2.1f FM", ((float)value)/1000);
		break;

	case FM_TX_CMD_GET_TUNED_FREQUENCY:
		FMAPP_MSG("Tuned to frequency %2.1f FM", ((float)value)/1000);
		break;

	case FM_TX_CMD_SET_MONO_STEREO_MODE:
		FMAPP_MSG("Set %s", g_fmapp_stereo_mode ? "Stereo Mode" : "Mono Mode");
		break;

	case FM_TX_CMD_GET_MONO_STEREO_MODE:
		FMAPP_MSG("%s", value ? "Stereo Mode" : "Mono Mode");
		break;

	case FM_TX_CMD_SET_POWER_LEVEL:
		FMAPP_MSG("Set power level %u", value);
		break;

	case FM_TX_CMD_GET_POWER_LEVEL:
		FMAPP_MSG("Power level is %u", value);
		break;

	case FM_TX_CMD_SET_MUTE_MODE:
		fmapp_display_mute_mode(g_fmapp_mute_mode);
		break;

	case FM_TX_CMD_GET_MUTE_MODE:
		fmapp_display_mute_mode(value);
		break;

	case FM_TX_CMD_ENABLE_RDS:
		FMAPP_MSG("RDS Enabled");
		break;

	case FM_TX_CMD_DISABLE_RDS:
		FMAPP_MSG("RDS Disabled");
		break;

	case FM_TX_CMD_SET_RDS_TRANSMISSION_MODE:
		FMAPP_MSG("Set %s RDS transmission mode", value ==
				FMC_RDS_TRANSMISSION_AUTOMATIC ? "Automatic" : "Manual");
		break;

	case FM_TX_CMD_GET_RDS_TRANSMISSION_MODE:
		FMAPP_MSG("%s RDS transmission mode", value ==
				FMC_RDS_TRANSMISSION_AUTOMATIC ? "Automatic" : "Manual");
		break;

	case FM_TX_CMD_SET_RDS_AF_CODE:
		FMAPP_MSG("Set RDS AF code to %u", value);
		break;

	case FM_TX_CMD_GET_RDS_AF_CODE:
		FMAPP_MSG("RDS_AF_CODE: %u", value);
		break;

	case FM_TX_CMD_SET_RDS_PI_CODE:
		FMAPP_MSG("SET_RDS_PI_CODE to %u", value);
		break;

	case FM_TX_CMD_GET_RDS_PI_CODE:
		FMAPP_MSG("RDS_PI_CODE: %u", value);
		break;

	case FM_TX_CMD_SET_RDS_PTY_CODE:
		FMAPP_MSG("SET_RDS_PTY_CODE to %u", value);
		break;

	case FM_TX_CMD_GET_RDS_PTY_CODE:
		FMAPP_MSG("RDS_PTY_CODE: %u", value);
		break;

	case FM_TX_CMD_SET_RDS_TEXT_REPERTOIRE:
		FMAPP_MSG("RDS TEXT REPERTOIRE Set");
		break;

	case FM_TX_CMD_GET_RDS_TEXT_REPERTOIRE:
		fmapp_display_text_repertoire(value);
		break;

	case FM_TX_CMD_SET_RDS_PS_DISPLAY_MODE:
		FMAPP_MSG("Set %s RDS PS display mode", value ==
				FMC_RDS_PS_DISPLAY_MODE_STATIC ? "Static" : "Scroll");
		break;

	case FM_TX_CMD_GET_RDS_PS_DISPLAY_MODE:
		FMAPP_MSG("%s RDS PS display mode", value ==
				FMC_RDS_PS_DISPLAY_MODE_STATIC ? "Static" : "Scroll");
		break;

	case FM_TX_CMD_SET_RDS_PS_DISPLAY_SPEED:
		FMAPP_MSG("SET_RDS_PS_DISPLAY_SPEED to %u", value);
		break;

	case FM_TX_CMD_GET_RDS_PS_DISPLAY_SPEED:
		FMAPP_MSG("RDS_PS_DISPLAY_SPEED is %u", value);
		break;

	case FM_TX_CMD_SET_RDS_TEXT_PS_MSG:
		FMAPP_MSG("RDS TEXT PS MSG Set");
		break;

	case FM_TX_CMD_GET_RDS_TEXT_PS_MSG:
		fmapp_display_generic_str("RDS text PS msg: ",
						event->p.cmdDone.v.psMsg.msg,
						event->p.cmdDone.v.psMsg.len);
		break;

	case FM_TX_CMD_SET_RDS_TEXT_RT_MSG:
		FMAPP_MSG("RDS TEXT RT MSG Set");
		break;

	case FM_TX_CMD_GET_RDS_TEXT_RT_MSG:
		fmapp_display_generic_str("RDS text RT msg: ",
						event->p.cmdDone.v.rtMsg.msg,
						event->p.cmdDone.v.rtMsg.len);
		break;

	case FM_TX_CMD_SET_RDS_TRANSMITTED_MASK:
		FMAPP_MSG("RDS TRANSMITTED MASK Set");
		break;

	case FM_TX_CMD_GET_RDS_TRANSMITTED_MASK:
		fmapp_display_transmitted_groups_mask(value);
		break;

	case FM_TX_CMD_SET_RDS_TRAFFIC_CODES:
		FMAPP_MSG("RDS TRAFFIC CODES Set");
		break;

	case FM_TX_CMD_GET_RDS_TRAFFIC_CODES:
		FMAPP_MSG("RDS traffic codes: ta = %d, tp = %d",
						event->p.cmdDone.v.trafficCodes.taCode,
						event->p.cmdDone.v.trafficCodes.tpCode);
		break;

	case FM_TX_CMD_SET_RDS_MUSIC_SPEECH_FLAG:
		FMAPP_MSG("RDS MUSIC SPEECH FLAG Set");
		break;

	case FM_TX_CMD_GET_RDS_MUSIC_SPEECH_FLAG:
		FMAPP_MSG("RDS %s flag", value == FMC_RDS_SPEECH ? "speech" : "music");
		break;

	case FM_TX_CMD_SET_PRE_EMPHASIS_FILTER:
		FMAPP_MSG("PRE-EMPHASIS FILTER Set");
		break;

	case FM_TX_CMD_GET_PRE_EMPHASIS_FILTER:
		fmapp_display_emphasis_filter(value);
		break;

	case FM_TX_CMD_SET_RDS_EXTENDED_COUNTRY_CODE:
		FMAPP_MSG("RDS EXTENDED COUNTRY CODE Set");
		break;

	case FM_TX_CMD_GET_RDS_EXTENDED_COUNTRY_CODE:
		FMAPP_MSG("RDS_EXTENDED_COUNTRY_CODE is %u", value);
		break;

	case FM_TX_CMD_WRITE_RDS_RAW_DATA:
		FMAPP_MSG("FM_TX_CMD_WRITE_RDS_RAW_DATA");
		break;

	case FM_TX_CMD_READ_RDS_RAW_DATA:
		FMAPP_MSG("FM_TX_CMD_READ_RDS_RAW_DATA");
		break;

	case FM_TX_CMD_CHANGE_AUDIO_SOURCE:
		FMAPP_MSG("Audio Source Changed");
		break;

	case FM_TX_CMD_CHANGE_DIGITAL_AUDIO_CONFIGURATION:
		FMAPP_MSG("Digital Audio Configuration Changed");
		break;

	default:
		FMAPP_ERROR("Received completion event of unknown FM TX command: %u", cmd);
		break;
	}

out:
	FMAPP_END();
}


static void fmapp_rx_cmd_done_handler(fm_rx_status status, FmRxCmdType cmd, FMC_U32 value)
{
	FMAPP_BEGIN();

	FMAPP_MSG_TO_FILE("%s(%d) done (status: %s, value: %d)", FMC_DEBUG_FmRxCmdStr(cmd),
						cmd,
						STATUS_DBG_STR(status),
						value);
	if (status) {
		FMAPP_ERROR("%s completed with error: %s", FMC_DEBUG_FmRxCmdStr(cmd), STATUS_DBG_STR(status));
		goto out;
	}

	switch (cmd) {
	case FM_RX_CMD_ENABLE:
		FMAPP_MSG("FM RX powered on");
		g_fmapp_now_initializing = 0;
		break;

	case FM_RX_CMD_DISABLE:
		FMAPP_MSG("FM RX powered off");
		fmapp_initialize_global_status();
		break;

	case FM_RX_CMD_SET_BAND:
		fmapp_display_band(g_fmapp_band_mode);
		break;

	case FM_RX_CMD_GET_BAND:
		fmapp_display_band(value);
		break;

	case FM_RX_CMD_SET_MONO_STEREO_MODE:
		FMAPP_MSG("Set mode to %s", g_fmapp_stereo_mode ? "Stereo Mode" : "Mono Mode");
		break;

	case FM_RX_CMD_GET_MONO_STEREO_MODE:
		FMAPP_MSG("%s", value ? "Stereo Mode" : "Mono Mode");
		break;

	case FM_RX_CMD_SET_MUTE_MODE:
		fmapp_display_mute_mode(g_fmapp_mute_mode);
		break;

	case FM_RX_CMD_GET_MUTE_MODE:
		fmapp_display_mute_mode(value);
		break;

	case FM_RX_CMD_SET_RF_DEPENDENT_MUTE_MODE:
		fmapp_display_rfmute_mode(g_fmapp_rfmute_mode);
		break;

	case FM_RX_CMD_GET_RF_DEPENDENT_MUTE_MODE:
		fmapp_display_rfmute_mode(value);
		break;

	case FM_RX_CMD_SET_RSSI_THRESHOLD:
		FMAPP_MSG("RSSI threshold set to %d", g_fmapp_seek_rssi);
		break;

	case FM_RX_CMD_GET_RSSI_THRESHOLD:
		FMAPP_MSG("RSSI threshold is %d", value);
		break;

	case FM_RX_CMD_SET_DEEMPHASIS_FILTER:
		fmapp_display_emphasis_filter(g_fmapp_emphasis_filter);
		break;

	case FM_RX_CMD_GET_DEEMPHASIS_FILTER:
		fmapp_display_emphasis_filter(value);
		break;

	case FM_RX_CMD_SET_VOLUME:
		fmapp_display_volume_bar();
		break;

	case FM_RX_CMD_GET_VOLUME:
		FMAPP_MSG("Volume is %d", value);
		break;

	case FM_RX_CMD_TUNE:
		fmapp_radio_tuned_handler(value);
		/* reset af list */
		memset(g_fmapp_af_list, 0, sizeof(g_fmapp_af_list));
		break;

	case FM_RX_CMD_GET_TUNED_FREQUENCY:
		fmapp_radio_tuned_handler(value);
		break;

	case FM_RX_CMD_SEEK:
		fmapp_display_seek(value, status);
		break;

	case FM_RX_CMD_STOP_SEEK:
		fmapp_display_stop_seek(value);
		break;

	case FM_RX_CMD_GET_RSSI:
		FMAPP_MSG("RSSI level is %d", value);
		break;

/*	case FM_RX_CMD_GET_PI:
		FMAPP_MSG("get pi %d", value);
		break;*/

	case FM_RX_CMD_ENABLE_RDS:
		g_fmapp_rds = FM_RX_CMD_ENABLE_RDS;
		FMAPP_MSG("RDS On");
		break;

	case FM_RX_CMD_DISABLE_RDS:
		g_fmapp_rds = FM_RX_CMD_DISABLE_RDS;
		FMAPP_MSG("RDS is Off");
		break;

	case FM_RX_CMD_SET_RDS_SYSTEM:
		fmapp_display_rds_system(g_fmapp_rds_system);
		break;

	case FM_RX_CMD_GET_RDS_SYSTEM:
		fmapp_display_rds_system(value);
		break;

	case FM_RX_CMD_SET_RDS_GROUP_MASK:
		fmapp_display_rds_group_mask(g_fmapp_rds_group_mask);
		break;

	case FM_RX_CMD_GET_RDS_GROUP_MASK:
		fmapp_display_rds_group_mask(value);
		break;

	case FM_RX_CMD_SET_RDS_AF_SWITCH_MODE:
		fmapp_display_rds_af_switch(g_fmapp_rds_af_switch);
		break;

	case FM_RX_CMD_GET_RDS_AF_SWITCH_MODE:
		fmapp_display_rds_af_switch(value);
		break;

	case FM_RX_CMD_ENABLE_AUDIO:
		g_fmapp_audio = FM_RX_CMD_ENABLE_AUDIO;
		FMAPP_MSG("Audio Enabled");
		break;

	case FM_RX_CMD_DISABLE_AUDIO:
		g_fmapp_audio = FM_RX_CMD_DISABLE_AUDIO;
		FMAPP_MSG("Audio Disabled");
		break;

	case FM_RX_CMD_DESTROY:
		FMAPP_MSG("Destroyed");
		break;

	case FM_RX_CMD_CHANGE_AUDIO_TARGET:
		FMAPP_MSG("Audio Target Changed");
		break;

	case FM_RX_CMD_CHANGE_DIGITAL_AUDIO_CONFIGURATION:
		FMAPP_MSG("Digital Audio Configuration Changed");
		break;

/*
	case FM_RX_CMD_SET_AUDIO_ROUTING:
		fmapp_display_audio_routing(g_fmapp_audio_route);
		break;

	case FM_RX_CMD_GET_AUDIO_ROUTING:
		fmapp_display_audio_routing(value);
		break;
*/

	default:
		FMAPP_ERROR("Received completion event of unknown FM RX command: %u", cmd);
		break;
	}

out:
	FMAPP_END();
}

static void fmapp_ps_changed_handle(FMC_U8 *name)
{
	FMAPP_BEGIN();

	FMAPP_MSG("RDS Name: %s", name);

	FMAPP_END();
}

static void fmapp_af_jump_handler(fm_rx_status status,
					FmcRdsPiCode pi,
					FmcFreq tunedFreq,
					FmcFreq afFreq)
{
	FMAPP_BEGIN();

	FMAPP_TRACE("af jump handler, status %d, pi %u", status, pi);

	switch (status) {
	case FM_RX_STATUS_SUCCESS:
		FMAPP_MSG("AF jump success (from %d to %d)", tunedFreq, afFreq);
		break;

	case FM_RX_STATUS_AF_SWITCH_FAILED_LIST_EXHAUSTED:
		FMAPP_MSG("AF jump failed (tried %d, no more)", afFreq);
		break;

	default:
		FMAPP_ERROR("unhandled AF jump handler status");
		break;
	}

	FMAPP_END();
}

static void fmapp_print_af_list(void)
{
	FMAPP_BEGIN();

	if (strlen(g_fmapp_af_list) > 0)
		FMAPP_MSG("last received AF list: %s", g_fmapp_af_list);
	else
		FMAPP_MSG("AF list is empty");

	FMAPP_END();
}

static void fmapp_radio_text_handler(FMC_BOOL resetDisplay, FMC_UINT len,
					FMC_U8 *msg, FMC_U8 startIndex)
{
	FMAPP_BEGIN();

	FMC_UNUSED_PARAMETER(resetDisplay);
	FMC_UNUSED_PARAMETER(startIndex);

/*	if (!erase) {
		offset = strlen(g_fmapp_radio_text);
		g_fmapp_radio_text[offset] = ' ';
		offset++;
	}

	how_much = min(sizeof(g_fmapp_radio_text) - startIndex - 1, len);

	memcpy(g_fmapp_radio_text+startIndex, msg, how_much);

	if (resetDisplay)
		g_fmapp_radio_text[startIndex + how_much] = '\0';*/

	fmapp_display_generic_str("RDS Text: ", msg, len);

	FMAPP_END();
}

static void fmapp_pi_code_changed_handler(FmcRdsPiCode pi)
{
	static FmcRdsPiCode last_msg_printed = 255;
	FMAPP_BEGIN();

	/* simple mechanism to prevent displaying repetitious messages */
	if (pi != last_msg_printed) {
		FMAPP_MSG("New RDS Pi code: %d", pi);
		last_msg_printed = pi;
	}

	FMAPP_END();
}

static void fmapp_stereo_mode_changed_handler(FmRxMonoStereoMode mode)
{
	FMAPP_BEGIN();

	/* simple mechanism to prevent displaying repetitious messages */
	if (mode == g_fmapp_stereo_mode)
		return;

	g_fmapp_stereo_mode = mode;

	switch(g_fmapp_stereo_mode) {
	case FM_RX_STEREO_MODE:
		FMAPP_MSG("Stereo Mode");
		break;
	case FM_RX_MONO_MODE:
		FMAPP_MSG("Mono Mode");
		break;
	default:
		FMAPP_TRACE("Illegal stereo mode received from stack: %d", mode);
		break;
	}

	FMAPP_END();
}

static void fmapp_af_list_changed_handler(FMC_UINT af_list_size, FmcFreq *af_list)
{
	int freq_m, freq_k;
	uint8_t index;
	int len = 0;

	for (index = 0; index < af_list_size; index++) {
		freq_m = af_list[index]/1000;
		freq_k = (af_list[index]-(freq_m*1000))/10;
		len +=sprintf(g_fmapp_af_list+len, "%d.%d ", freq_m, freq_k);
	}

	FMAPP_TRACE_L(LOWEST_TRACE_LVL, "received af list: %s",
							g_fmapp_af_list);
}

void fmapp_rx_callback(const fm_rx_event_s *event)
{
	FMAPP_BEGIN();

	FMAPP_TRACE_L(LOWEST_TRACE_LVL, "got event %d", event->eventType);

	switch (event->eventType) {
	case FM_RX_EVENT_CMD_DONE:
		fmapp_rx_cmd_done_handler(event->status,
						event->p.cmdDone.cmd,
						event->p.cmdDone.value);
		break;

	case FM_RX_EVENT_MONO_STEREO_MODE_CHANGED:
		fmapp_stereo_mode_changed_handler(event->p.monoStereoMode.mode);
		break;

	case FM_RX_EVENT_PI_CODE_CHANGED:
		fmapp_pi_code_changed_handler(event->p.piChangedData.pi);
		break;

	case FM_RX_EVENT_AF_SWITCH_START:
		FMAPP_MSG("AF switch process has started...");
		break;

	case FM_RX_EVENT_AF_SWITCH_TO_FREQ_FAILED:
		FMAPP_MSG("AF switch to %d failed", event->p.afSwitchData.afFreq);
		break;

	case FM_RX_EVENT_AF_SWITCH_COMPLETE:
		fmapp_af_jump_handler(event->status,
					event->p.afSwitchData.pi,
					event->p.afSwitchData.tunedFreq,
					event->p.afSwitchData.afFreq);
		break;

	case FM_RX_EVENT_AF_LIST_CHANGED:
		fmapp_af_list_changed_handler(event->p.afListData.afListSize,
					event->p.afListData.afList);
		break;

	case FM_RX_EVENT_PS_CHANGED:
		fmapp_ps_changed_handle(event->p.psData.name);
		break;

	case FM_RX_EVENT_RADIO_TEXT:
		fmapp_radio_text_handler(event->p.radioTextData.resetDisplay,
						event->p.radioTextData.len,
						event->p.radioTextData.msg,
						event->p.radioTextData.startIndex);
		break;

	case FM_RX_EVENT_RAW_RDS:
		FMAPP_MSG("RDS group data is received");
		break;

	case FM_RX_EVENT_AUDIO_PATH_CHANGED:
		FMAPP_MSG("Audio Path Changed Event received");
		break;

	case FM_RX_EVENT_PTY_CODE_CHANGED:
		FMAPP_MSG("RDS PTY Code has changed to %d",event->p.ptyChangedData.pty);
		break;

	default:
		FMAPP_ERROR("unhandled fm event %d", event->eventType);
		break;
	}

	FMAPP_END();
}

void fmapp_tx_callback(const fm_tx_event_s *event)
{
	FMAPP_BEGIN();

	FMAPP_TRACE_L(LOWEST_TRACE_LVL, "got event %d", event->eventType);

	switch (event->eventType) {
	case FM_TX_EVENT_CMD_DONE:
		fmapp_tx_cmd_done_handler(event);
		break;

	default:
		FMAPP_ERROR("unhandled fm event %d", event->eventType);
		break;
	}

	FMAPP_END();
}

fm_status register_sig_handlers(void)
{
	struct sigaction sa;
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = fmapp_termination_handler;

	if (-1 == sigaction(SIGTERM, &sa, NULL)) {
		FMAPP_ERROR_SYS("failed to register SIGTERM");
		ret = FMC_STATUS_FAILED;
		goto out;
	}

	if (-1 == sigaction(SIGINT, &sa, NULL)) {
		FMAPP_ERROR_SYS("failed to register SIGINT");
		ret = FMC_STATUS_FAILED;
		goto out;
	}

	if (-1 == sigaction(SIGQUIT, &sa, NULL)) {
		FMAPP_ERROR_SYS("failed to register SIGQUIT");
		ret = FMC_STATUS_FAILED;
		goto out;
	}

out:
	FMAPP_END();
	return ret;
}

fm_status init_rx_stack(fm_rx_context_s  **fm_context)
{
	void * hMcpf = NULL;
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();
//	set_fmapp_audio_routing(fm_context);
	FMAPP_MSG("Powering on FM RX... (this might take awhile)");

	ret = FM_RX_Init(hMcpf);
	if (ret) {
		FMAPP_ERROR("failed to initialize FM rx stack");
		goto out;
	}

	ret = FM_RX_Create(NULL, fmapp_rx_callback, fm_context);
	if (ret) {
		FMAPP_ERROR("failed to create FM rx stack context");
		goto deinit;
	}

	/* power on FM core */
	ret = FM_RX_Enable(*fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS) {
		FMAPP_ERROR("failed to enable FM rx stack");
		goto destroy;
	}

	/* operation is pending now */
	g_fmapp_now_initializing = 1;

	goto out;

destroy:
	/* deinit FM stack */
	if (FMC_STATUS_FAILED == FM_RX_Destroy(fm_context))
		FMAPP_ERROR("failed to destroy FM stack context");
deinit:
	if (FMC_STATUS_FAILED == FM_RX_Deinit())
		FMAPP_ERROR("failed to de-init fm stack");
out:
	FMAPP_END();
	return ret;
}

fm_status deinit_rx_stack(fm_rx_context_s **fm_context)
{
	FmRxStatus ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();
//	unset_fmapp_audio_routing(fm_context);
	g_fmapp_now_initializing = 0;

	/* power off FM core */
	ret = FM_RX_Disable(*fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to disable FM RX stack context (%s)", STATUS_DBG_STR(ret));

	sleep(2);

	/* deinit FM stack */
	ret = FM_RX_Destroy(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to destroy FM RX stack context (%s)", STATUS_DBG_STR(ret));

	ret = FM_RX_Deinit();
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to de-init FM RX stack context (%s)", STATUS_DBG_STR(ret));

	FMAPP_END();
	return ret;
}

fm_status init_tx_stack(fm_tx_context_s  **fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	FMAPP_MSG("Powering on FM TX... (this might take awhile)");

	ret = FM_TX_Init();
	if (ret) {
		FMAPP_ERROR("failed to initialize FM TX stack");
		goto out;
	}

	ret = FM_TX_Create(NULL, fmapp_tx_callback, fm_context);
	if (ret) {
		FMAPP_ERROR("failed to create FM TX stack context");
		goto deinit;
	}

	/* power on FM core */
	ret = FM_TX_Enable(*fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS) {
		FMAPP_ERROR("failed to enable FM TX stack");
		goto destroy;
	}

	/* operation is pending now */
	g_fmapp_now_initializing = 1;

	goto out;

destroy:
	/* deinit FM stack */
	if (FMC_STATUS_FAILED == FM_TX_Destroy(fm_context))
		FMAPP_ERROR("failed to destroy FM stack context");
deinit:
	if (FMC_STATUS_FAILED == FM_TX_Deinit())
		FMAPP_ERROR("failed to de-init fm stack");
out:
	FMAPP_END();
	return ret;
}

fm_status deinit_tx_stack(fm_tx_context_s **fm_context)
{
	FmRxStatus ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	g_fmapp_now_initializing = 0;

	/* power off FM core */
	ret = FM_TX_Disable(*fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to disable FM TX stack context (%s)", STATUS_DBG_STR(ret));

	sleep(2);

	/* deinit FM stack */
	ret = FM_TX_Destroy(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to destroy FM TX stack context (%s)", STATUS_DBG_STR(ret));

	ret = FM_TX_Deinit();
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to de-init FM TX stack context (%s)", STATUS_DBG_STR(ret));

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_band(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	FMAPP_TRACE("Getting band");

	ret = FM_RX_GetBand(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get band mode (%s)",
						STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_band(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	switch (g_fmapp_band_mode) {
	case FMC_BAND_EUROPE_US:
		g_fmapp_band_mode = FMC_BAND_JAPAN;
		break;
	case FMC_BAND_JAPAN:
	default:
		g_fmapp_band_mode = FMC_BAND_EUROPE_US;
		break;
	}

	FMAPP_TRACE("Setting band to %d", g_fmapp_band_mode);

	ret = FM_RX_SetBand(fm_context, g_fmapp_band_mode);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set band (%s)",
						STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_change_rxtx_mode(void)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	if (g_fmapp_power_mode) {
		FMAPP_MSG("Power down before changing rxtx mode");
		goto out;
	}

	switch (g_fmapp_rxtx_mode) {
	case FMAPP_RX_MODE:
		FMAPP_MSG("Mode is now set to FM TX");
		g_fmapp_rxtx_mode = FMAPP_TX_MODE;
		break;
	case FMAPP_TX_MODE:
	default:
		FMAPP_MSG("Mode is now set to FM RX");
		g_fmapp_rxtx_mode = FMAPP_RX_MODE;
		break;
	}

out:
	FMAPP_END();
	return ret;
}

fm_status fmapp_get_mute(void *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	if (g_fmapp_rxtx_mode == FMAPP_RX_MODE)
		ret = FM_RX_GetMuteMode(fm_context);
	else
		ret = FM_TX_GetMuteMode(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get mute mode (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_mute(void *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	switch (g_fmapp_mute_mode) {
	case FMC_NOT_MUTE:
		g_fmapp_mute_mode = FMC_MUTE;
		break;
	case FMC_MUTE:
		/* Only FM RX has FMC_ATTENUATE mode */
		if (g_fmapp_rxtx_mode == FMAPP_RX_MODE)
			g_fmapp_mute_mode = FMC_ATTENUATE;
		else
			g_fmapp_mute_mode = FMC_NOT_MUTE;
		break;
	case FMC_ATTENUATE:
	default:
		g_fmapp_mute_mode = FMC_NOT_MUTE;
		break;
	}

	FMAPP_TRACE("setting %d", g_fmapp_mute_mode);

	if (g_fmapp_rxtx_mode == FMAPP_RX_MODE)
		ret = FM_RX_SetMuteMode(fm_context, g_fmapp_mute_mode);
	else
		ret = FM_TX_SetMuteMode(fm_context, g_fmapp_mute_mode);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set mute mode (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_rfmute(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	ret = FM_RX_GetRfDependentMute(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set mute mode (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_rfmute(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	switch (g_fmapp_rfmute_mode) {
	case FM_RX_RF_DEPENDENT_MUTE_ON:
		g_fmapp_rfmute_mode = FM_RX_RF_DEPENDENT_MUTE_OFF;
		break;
	case FM_RX_RF_DEPENDENT_MUTE_OFF:
	default:
		g_fmapp_rfmute_mode = FM_RX_RF_DEPENDENT_MUTE_ON;
		break;
	}

	FMAPP_TRACE("setting %d", g_fmapp_rfmute_mode);

	ret = FM_RX_SetRfDependentMuteMode(fm_context, g_fmapp_rfmute_mode);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set mute mode (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_emphasis_filter(void *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	if (g_fmapp_rxtx_mode == FMAPP_RX_MODE)
		ret = FM_RX_GetDeEmphasisFilter(fm_context);
	else
		ret = FM_TX_GetPreEmphasisFilter(fm_context);

	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set mute mode (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_emphasis_filter(void *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	switch (g_fmapp_emphasis_filter) {
	case FMC_EMPHASIS_FILTER_NONE:
		g_fmapp_emphasis_filter = FMC_EMPHASIS_FILTER_50_USEC;
		break;
	case FMC_EMPHASIS_FILTER_50_USEC:
		g_fmapp_emphasis_filter = FMC_EMPHASIS_FILTER_75_USEC;
		break;
	case FMC_EMPHASIS_FILTER_75_USEC:
	default:
		g_fmapp_emphasis_filter = FMC_EMPHASIS_FILTER_NONE;
		break;
	}

	FMAPP_TRACE("setting %d", g_fmapp_emphasis_filter);

	if (g_fmapp_rxtx_mode == FMAPP_RX_MODE)
		ret = FM_RX_SetDeEmphasisFilter(fm_context, g_fmapp_emphasis_filter);
	else
		ret = FM_TX_SetPreEmphasisFilter(fm_context, g_fmapp_emphasis_filter);

	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set filter: %s", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

#if 0
fm_status fmapp_get_audio_routing(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	ret = FM_RX_GetAudioRouting(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get audio routing mask (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_audio_routing(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	switch (g_fmapp_audio_route) {
	case FMC_AUDIO_ROUTE_MASK_NONE:
		g_fmapp_audio_route = FMC_AUDIO_ROUTE_MASK_I2S;
		break;
	case FMC_AUDIO_ROUTE_MASK_I2S:
		g_fmapp_audio_route = FMC_AUDIO_ROUTE_MASK_ANALOG;
		break;
	case FMC_AUDIO_ROUTE_MASK_ANALOG:
		g_fmapp_audio_route = FMC_AUDIO_ROUTE_MASK_ALL;
		break;
	case FMC_AUDIO_ROUTE_MASK_ALL:
	default:
		g_fmapp_audio_route = FMC_AUDIO_ROUTE_MASK_NONE;
		break;
	}

	FMAPP_TRACE("setting %d", g_fmapp_audio_route);

	ret = FM_RX_SetAudioRouting(fm_context, g_fmapp_audio_route);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set audio routing mask (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}
#endif

fm_status fmapp_get_rds_system(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	ret = FM_RX_GetRdsSystem(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set rds system (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_rds_system(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	switch (g_fmapp_rds_system) {
	case FM_RDS_SYSTEM_RDS:
		g_fmapp_rds_system = FM_RDS_SYSTEM_RBDS;
		break;
	case FM_RDS_SYSTEM_RBDS:
	default:
		g_fmapp_rds_system = FM_RDS_SYSTEM_RDS;
		break;
	}

	FMAPP_TRACE("setting %d", g_fmapp_rds_system);

	ret = FM_RX_SetRdsSystem(fm_context, g_fmapp_rds_system);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set rds system (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_rds_group_mask(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	ret = FM_RX_GetRdsGroupMask(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get rds group mask (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_rds_group_mask(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	switch (g_fmapp_rds_group_mask) {
	case FM_RDS_GROUP_TYPE_MASK_NONE:
		g_fmapp_rds_group_mask = FM_RDS_GROUP_TYPE_MASK_0A;
		break;
	case FM_RDS_GROUP_TYPE_MASK_15B:
		g_fmapp_rds_group_mask = FM_RDS_GROUP_TYPE_MASK_ALL;
		break;
	case FM_RDS_GROUP_TYPE_MASK_ALL:
		g_fmapp_rds_group_mask = FM_RDS_GROUP_TYPE_MASK_NONE;
		break;
	default:
		g_fmapp_rds_group_mask <<= 1;
		break;
	}

	ret = FM_RX_SetRdsGroupMask(fm_context, g_fmapp_rds_group_mask);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set rds group mask (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_rds_af_switch(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	ret = FM_RX_GetRdsAfSwitchMode(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get rds af switch mode (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_rds_af_switch(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	switch (g_fmapp_rds_af_switch) {
	case FM_RX_RDS_AF_SWITCH_MODE_ON:
		g_fmapp_rds_af_switch = FM_RX_RDS_AF_SWITCH_MODE_OFF;
		break;
	case FM_RX_RDS_AF_SWITCH_MODE_OFF:
	default:
		g_fmapp_rds_af_switch = FM_RX_RDS_AF_SWITCH_MODE_ON;
		break;
	}

	FMAPP_TRACE("setting rds af switch to %x", g_fmapp_rds_af_switch);

	ret = FM_RX_SetRdsAfSwitchMode(fm_context, g_fmapp_rds_af_switch);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set rds af switch mode (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_frequency(void *fm_context, char interactive,
							char *cmd, int *index)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	float user_freq;
	char *endp;
	int progress;
	FMAPP_BEGIN();

	if (interactive == FMAPP_INTERACTIVE)
		sscanf(cmd, "%f", &user_freq);
	else /* FMAPP_BATCH */ {
		user_freq = strtod(cmd + 1, &endp);
		progress = endp - (cmd + 1);
		*index += progress;
	}

	g_fmapp_freq = rint(user_freq * 1000);

	FMAPP_TRACE("freq tuning to %u", g_fmapp_freq);

	if (g_fmapp_rxtx_mode == FMAPP_RX_MODE)
		ret = FM_RX_Tune(fm_context, g_fmapp_freq);
	else
		ret = FM_TX_Tune(fm_context, g_fmapp_freq);

	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("tuning failed (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_rssi_threshold(fm_rx_context_s *fm_context, char interactive,
							char *cmd, int *index)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	char *endp;
	int progress;

	FMAPP_BEGIN();

	if (interactive == FMAPP_INTERACTIVE)
		sscanf(cmd, "%d", &g_fmapp_seek_rssi);
	else /* FMAPP_BATCH */ {
		g_fmapp_seek_rssi = strtol(cmd + 1, &endp, 10);
		progress = endp - (cmd + 1);
		*index += progress;
	}

	validate_int_boundaries(&g_fmapp_seek_rssi, -16, 15);

	FMAPP_TRACE("setting rssi to %d", g_fmapp_seek_rssi);

	ret = FM_RX_SetRssiThreshold(fm_context, g_fmapp_seek_rssi);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("setting rssi level failed (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_frequency(void *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;

	FMAPP_BEGIN();

	if (g_fmapp_rxtx_mode == FMAPP_RX_MODE)
		ret = FM_RX_GetTunedFrequency(fm_context);
	else
		ret = FM_TX_GetTunedFrequency(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get current frequency (%s)",
							STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_stereo(void *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	if (g_fmapp_rxtx_mode == FMAPP_RX_MODE)
		ret = FM_RX_GetMonoStereoMode(fm_context);
	else
		ret = FM_TX_GetMonoStereoMode(fm_context);

	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get stereo mode: %s", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_stereo(void *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	switch (g_fmapp_stereo_mode) {
	case FM_RX_STEREO_MODE:
		g_fmapp_stereo_mode = FM_RX_MONO_MODE;
		break;
	case FM_RX_MONO_MODE:
	default:
		g_fmapp_stereo_mode = FM_RX_STEREO_MODE;
		break;
	}

	if (g_fmapp_rxtx_mode == FMAPP_RX_MODE)
		ret = FM_RX_SetMonoStereoMode(fm_context, g_fmapp_stereo_mode);
	else
		ret = FM_TX_SetMonoStereoMode(fm_context, g_fmapp_stereo_mode);

	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set stereo mode: %s", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_rssi_level(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	ret = FM_RX_GetRssi(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get RSSI level (%s)", 	STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_rssi_threshold(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	ret = FM_RX_GetRssiThreshold(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get RSSI level (%s)", 	STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_increase_volume(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	g_fmapp_volume += FMAPP_VOLUME_DELTA;

	validate_uint_boundaries(&g_fmapp_volume, FMAPP_VOLUME_MIN,
							FMAPP_VOLUME_MAX);

	ret = FM_RX_SetVolume(fm_context, g_fmapp_volume);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
// && ret != FM_STATUS_PENDING_UPDATE_CMD_PARAMS)
		FMAPP_ERROR("failed to increase volume (%s)",
							STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_decrease_volume(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	g_fmapp_volume -= FMAPP_VOLUME_DELTA;

	validate_uint_boundaries(&g_fmapp_volume, FMAPP_VOLUME_MIN,
							FMAPP_VOLUME_MAX);

	ret = FM_RX_SetVolume(fm_context, g_fmapp_volume);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
// && ret !=FM_STATUS_PENDING_UPDATE_CMD_PARAMS)
		FMAPP_ERROR("failed to decrease volume (%s)",
							STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_volume(fm_rx_context_s *fm_context)

{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	ret = FM_RX_GetVolume(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get volume (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_tx_power(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	ret = FM_TX_GetPowerLevel(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get tx power: %s", FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_tx_power(fm_tx_context_s *fm_context, char interactive, char *cmd,
								int *index)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	FMC_UNUSED_PARAMETER(interactive);
	FMC_UNUSED_PARAMETER(index);

	sscanf(cmd, "%d", &g_fmapp_tx_power);

	validate_uint_boundaries(&g_fmapp_tx_power, FM_TX_POWER_LEVEL_MIN,
							FM_TX_POWER_LEVEL_MAX);

	FMAPP_TRACE("Setting tx power to %d", g_fmapp_tx_power);

	ret = FM_TX_SetPowerLevel(fm_context, g_fmapp_tx_power);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set tx power: %s", FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_volume(fm_rx_context_s *fm_context, char interactive, char *cmd,
								int *index)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();
	char *endp;
	int progress;

	if (interactive == FMAPP_INTERACTIVE)
		sscanf(cmd, "%d", &g_fmapp_volume);
	else /* FMAPP_BATCH */ {
		g_fmapp_volume = strtol(cmd + 1, &endp, 10);
		progress = endp - (cmd + 1);
		*index += progress;
	}

	g_fmapp_volume *= FMAPP_VOLUME_DELTA;
	validate_uint_boundaries(&g_fmapp_volume, FMAPP_VOLUME_MIN,
							FMAPP_VOLUME_MAX);

	FMAPP_TRACE("Setting volume to %d", g_fmapp_volume);

	ret = FM_RX_SetVolume(fm_context, g_fmapp_volume);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set volume (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_change_rds_mode(void *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	g_fmapp_rds = (g_fmapp_rds == FM_RX_CMD_ENABLE_RDS ?
				FM_RX_CMD_DISABLE_RDS : FM_RX_CMD_ENABLE_RDS);

	switch (g_fmapp_rds) {
	case FM_RX_CMD_ENABLE_RDS:
		if (FMAPP_RX_MODE == g_fmapp_rxtx_mode)
			ret = FM_RX_EnableRds(fm_context);
		else
			ret = FM_TX_EnableRds(fm_context);
		if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
			FMAPP_ERROR("failed to enable RDS: %s",
							STATUS_DBG_STR(ret));
		else
			ret = FMC_STATUS_SUCCESS;
		break;
	case FM_RX_CMD_DISABLE_RDS:
		if (FMAPP_RX_MODE == g_fmapp_rxtx_mode)
			ret = FM_RX_DisableRds(fm_context);
		else
			ret = FM_TX_DisableRds(fm_context);
		if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
			FMAPP_ERROR("failed to disable RDS: %s",
							STATUS_DBG_STR(ret));
		else
			ret = FMC_STATUS_SUCCESS;
		break;
	default:
		FMAPP_ERROR("invalid fmapp rds mode: %d", g_fmapp_rds);
		break;
	}

	FMAPP_END();
	return ret;
}

#if 0
fm_status fmapp_change_af_mode(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	g_fmapp_af_mode = (g_fmapp_af_mode == FM_RX_RDS_AF_SWITCH_MODE_OFF ?
						FM_RX_RDS_AF_SWITCH_MODE_ON :
						FM_RX_RDS_AF_SWITCH_MODE_OFF);

	ret = FM_RX_SetRdsAfSwitchMode(fm_context, g_fmapp_af_mode);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to change AF mode (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}
#endif

fm_status fmapp_rx_stop_seek(fm_rx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	ret = FM_RX_StopSeek(fm_context);

	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to stop seek: %s", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_rx_seek(fm_rx_context_s *fm_context, unsigned char command)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	uint8_t direction = 0;
	FMAPP_BEGIN();

	switch (command) {
	case '<':
		direction = FM_RX_SEEK_DIRECTION_DOWN;
		FMAPP_MSG("Seeking down...");
		break;
	case '>':
		direction = FM_RX_SEEK_DIRECTION_UP;
		FMAPP_MSG("Seeking up...");
		break;
	default:
		FMAPP_ERROR("illegal set seek command");
		goto out;
		break;
	}

	ret = FM_RX_Seek(fm_context, direction);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
// && ret != FM_STATUS_PENDING_UPDATE_CMD_PARAMS && ret != FMC_STATUS_SUCCESS && ret != FM_STATUS_FAILED_ALREADY_PENDING)
		FMAPP_ERROR("failed to set seek mode (%s)", STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

out:
	FMAPP_END();
	return ret;
}

fm_status fmapp_get_tx_rds_transmission_mode(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	ret = FM_TX_GetRdsTransmissionMode(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get rds TX transmission mode: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_tx_rds_transmission_mode(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	switch(g_fmapp_tx_rds_transission_mode) {
	case FMC_RDS_TRANSMISSION_MANUAL:
		g_fmapp_tx_rds_transission_mode = FMC_RDS_TRANSMISSION_AUTOMATIC;
		break;
	case FMC_RDS_TRANSMISSION_AUTOMATIC:
	default:
		g_fmapp_tx_rds_transission_mode = FMC_RDS_TRANSMISSION_MANUAL;
		break;
	}

	ret = FM_TX_SetRdsTransmissionMode(fm_context, g_fmapp_tx_rds_transission_mode);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set rds TX transmission mode: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_tx_rds_af_code(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	ret = FM_TX_GetRdsAfCode(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get rds TX af code: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_tx_rds_af_code(fm_tx_context_s *fm_context, char interactive,
							char *cmd, int *index)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMC_UNUSED_PARAMETER(index);
	FMC_UNUSED_PARAMETER(interactive);
	FmcAfCode af_code;
	FMAPP_BEGIN();

	sscanf(cmd, "%u", &af_code);

	FMAPP_TRACE("freq tuning to %u", af_code);

	ret = FM_TX_SetRdsAfCode(fm_context, af_code);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set rds TX af code: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_tx_rds_pi_code(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	ret = FM_TX_GetRdsPiCode(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get_tx_rds_pi_code: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_tx_rds_pi_code(fm_tx_context_s *fm_context, char interactive,
							char *cmd, int *index)
{
	fm_status ret = FMC_STATUS_FAILED;
	unsigned int tmp;
	FmcRdsPiCode piCode;
	FMC_UNUSED_PARAMETER(index);
	FMC_UNUSED_PARAMETER(interactive);
	FMAPP_BEGIN();

	sscanf(cmd, "%u", &tmp);

	piCode = tmp;

	FMAPP_TRACE("set_tx_rds_pi_code to %u", (unsigned int)piCode);

	ret = FM_TX_SetRdsPiCode(fm_context, piCode);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set_tx_rds_pi_code: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_tx_rds_pty_code(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	ret = FM_TX_GetRdsPtyCode(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get_tx_rds_pty_codeto: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_tx_rds_pty_code(fm_tx_context_s *fm_context, char interactive,
							char *cmd, int *index)
{
	fm_status ret = FMC_STATUS_FAILED;
	FmcRdsPtyCode ptyCode;
	FMC_UNUSED_PARAMETER(index);
	FMC_UNUSED_PARAMETER(interactive);
	FMAPP_BEGIN();

	sscanf(cmd, "%u", &ptyCode);

	FMAPP_TRACE("fmapp_set_tx_rds_pty_code to %u", ptyCode);

	ret = FM_TX_SetRdsPtyCode(fm_context, ptyCode);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set_tx_rds_pty_codeto: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_tx_rds_text_repertoire(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	ret = FM_TX_GetRdsTextRepertoire(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get_tx_rds_text_repertoire: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_tx_rds_text_repertoire(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	switch(g_fmapp_tx_rds_text_repertoire) {
	case FMC_RDS_REPERTOIRE_G0_CODE_TABLE:
		g_fmapp_tx_rds_text_repertoire = FMC_RDS_REPERTOIRE_G1_CODE_TABLE;
		break;
	case FMC_RDS_REPERTOIRE_G1_CODE_TABLE:
		g_fmapp_tx_rds_text_repertoire = FMC_RDS_REPERTOIRE_G2_CODE_TABLE;
		break;
	case FMC_RDS_REPERTOIRE_G2_CODE_TABLE:
	default:
		g_fmapp_tx_rds_text_repertoire = FMC_RDS_REPERTOIRE_G0_CODE_TABLE;
		break;
	}

	FMAPP_TRACE("fmapp_set_tx_rds_text_repertoire to %u", g_fmapp_tx_rds_text_repertoire);

	ret = FM_TX_SetRdsTextRepertoire(fm_context, g_fmapp_tx_rds_text_repertoire);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set_tx_rds_text_repertoire: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_tx_rds_ps_display_mode(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	ret = FM_TX_GetRdsPsDisplayMode(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get_tx_rds_ps_display_mode: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_tx_rds_ps_display_mode(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	switch(g_fmapp_tx_rds_ps_display_mode) {
	case FMC_RDS_PS_DISPLAY_MODE_STATIC:
		g_fmapp_tx_rds_ps_display_mode = FMC_RDS_PS_DISPLAY_MODE_SCROLL;
		break;
	case FMC_RDS_PS_DISPLAY_MODE_SCROLL:
	default:
		g_fmapp_tx_rds_ps_display_mode = FMC_RDS_PS_DISPLAY_MODE_STATIC;
		break;
	}

	FMAPP_TRACE("fmapp_set_tx_rds_ps_display_mode to %u", g_fmapp_tx_rds_ps_display_mode);

	ret = FM_TX_SetRdsPsDisplayMode(fm_context, g_fmapp_tx_rds_ps_display_mode);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set_tx_rds_ps_display_mode: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_tx_rds_scroll_speed(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	ret = FM_TX_GetRdsPsScrollSpeed(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get_tx_rds_scroll_speed: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_tx_rds_scroll_speed(fm_tx_context_s *fm_context, char interactive,
							char *cmd, int *index)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMC_UNUSED_PARAMETER(index);
	FMC_UNUSED_PARAMETER(interactive);
	FmcRdsPsScrollSpeed speed;
	FMAPP_BEGIN();

	sscanf(cmd, "%u", &speed);

	FMAPP_TRACE("fmapp_set_tx_rds_scroll_speed to %u", speed);

	ret = FM_TX_SetRdsPsScrollSpeed(fm_context, speed);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set_tx_rds_scroll_speed: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_tx_rds_text_ps_msg(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	ret = FM_TX_GetRdsTextPsMsg(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get_tx_rds_text_ps_msg: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_tx_rds_text_ps_msg(fm_tx_context_s *fm_context, char interactive,
							char *cmd, int *index)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMC_UNUSED_PARAMETER(index);
	FMC_UNUSED_PARAMETER(interactive);
	FMAPP_BEGIN();

	FMAPP_TRACE("fmapp_set_tx_rds_text_ps_msg to %s", cmd);

	ret = FM_TX_SetRdsTextPsMsg(fm_context, (unsigned char *)cmd, strlen(cmd));
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set_tx_rds_text_ps_msg: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_tx_rds_text_rt_msg(fm_tx_context_s *fm_context, char interactive,
							char *cmd, int *index)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMC_UNUSED_PARAMETER(index);
	FMC_UNUSED_PARAMETER(interactive);
	FMAPP_BEGIN();

	FMAPP_TRACE("fmapp_set_tx_rds_text_rt_msg to %s", cmd);

	ret = FM_TX_SetRdsTextRtMsg(fm_context, FMC_RDS_RT_MSG_TYPE_AUTO, (FMC_U8 *)cmd, strlen(cmd));
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set_tx_rds_text_rt_msg: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_tx_rds_text_rt_msg(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	ret = FM_TX_GetRdsTextRtMsg(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get_tx_rds_text_rt_msg: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_tx_rds_transmitted_groups_mask(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	switch(g_fmapp_tx_rds_transmitted_groups_mask) {
	case FM_TX_RDS_RADIO_TRANSMITTED_GROUP_PS_MASK:
		g_fmapp_tx_rds_transmitted_groups_mask = FM_TX_RDS_RADIO_TRANSMITTED_GROUP_RT_MASK;
		break;
	case FM_TX_RDS_RADIO_TRANSMITTED_GROUP_RT_MASK:
		g_fmapp_tx_rds_transmitted_groups_mask = FM_TX_RDS_RADIO_TRANSMITTED_GROUP_ECC_MASK;
		break;
	case FM_TX_RDS_RADIO_TRANSMITTED_GROUP_ECC_MASK:
	default:
		g_fmapp_tx_rds_transmitted_groups_mask = FM_TX_RDS_RADIO_TRANSMITTED_GROUP_PS_MASK;
		break;
	}

	FMAPP_TRACE("fmapp_set_tx_rds_transmitted_groups_mask to %u", g_fmapp_tx_rds_transmitted_groups_mask);

	ret = FM_TX_SetRdsTransmittedGroupsMask(fm_context, g_fmapp_tx_rds_transmitted_groups_mask);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set_tx_rds_transmitted_groups_mask: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_tx_rds_transmitted_groups_mask(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	ret = FM_TX_GetRdsTransmittedGroupsMask(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get_tx_rds_transmitted_groups_mask: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_tx_rds_traffic_codes(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	ret = FM_TX_GetRdsTrafficCodes(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get_tx_rds_traffic_codes: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_tx_rds_traffic_codes(fm_tx_context_s *fm_context, char interactive,
							char *cmd, int *index)
{
	fm_status ret = FMC_STATUS_FAILED;
	FmcRdsTaCode	taCode;
	FmcRdsTpCode 	tpCode;
	FMC_UNUSED_PARAMETER(index);
	FMC_UNUSED_PARAMETER(interactive);
	FMAPP_BEGIN();

	sscanf(cmd, "%u %u", &taCode, &tpCode);

	FMAPP_TRACE("fmapp_set_tx_rds_traffic_codes to ta %u tp %u", taCode, tpCode);

	ret = FM_TX_SetRdsTrafficCodes(fm_context, taCode, tpCode);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set_tx_rds_traffic_codes: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_tx_rds_music_speech_flag(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	ret = FM_TX_GetRdsMusicSpeechFlag(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get_tx_rds_music_speech_flag: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_tx_rds_music_speech_flag(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	switch(g_fmapp_tx_music_speech_flag) {
	case FMC_RDS_SPEECH:
		g_fmapp_tx_music_speech_flag = FMC_RDS_MUSIC;
		break;
	case FMC_RDS_MUSIC:
	default:
		g_fmapp_tx_music_speech_flag = FMC_RDS_SPEECH;
		break;
	}

	FMAPP_TRACE("fmapp_set_tx_rds_music_speech_flag to %u", g_fmapp_tx_music_speech_flag);

	ret = FM_TX_SetRdsMusicSpeechFlag(fm_context, g_fmapp_tx_music_speech_flag);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set_tx_rds_music_speech_flag: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_get_tx_rds_ecc(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	ret = FM_TX_GetRdsECC(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to get_tx_rds_ecc: %s", 	FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_set_tx_rds_ecc(fm_tx_context_s *fm_context, char interactive,
							char *cmd, int *index)
{
	fm_status ret = FMC_STATUS_FAILED;
	FmcRdsExtendedCountryCode ecc;
	FMC_UNUSED_PARAMETER(index);
	FMC_UNUSED_PARAMETER(interactive);
	FMAPP_BEGIN();

	sscanf(cmd, "%u", &ecc);

	FMAPP_TRACE("fmapp_set_tx_rds_ecc to %u", ecc);

	ret = FM_TX_SetRdsECC(fm_context, ecc);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to set_tx_rds_ecc: %s", 	FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_tx_read_rds_raw_data(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	ret = FM_TX_ReadRdsRawData(fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to tx_read_rds_raw_data: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_tx_write_rds_raw_data(fm_tx_context_s *fm_context, char interactive,
							char *cmd, int *index)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMC_UNUSED_PARAMETER(index);
	FMC_UNUSED_PARAMETER(interactive);
	FMAPP_BEGIN();

	FMAPP_TRACE("fmapp_tx_write_rds_raw_data to %s", cmd);

	ret = FM_TX_WriteRdsRawData(fm_context, (FMC_U8 *) cmd, strlen(cmd));
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to tx_write_rds_raw_data: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	FMAPP_END();
	return ret;
}

fm_status fmapp_change_tx_transmission_mode(fm_tx_context_s *fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	g_fmapp_tx_transission_mode = (g_fmapp_tx_transission_mode ? 0 : 1);

	if (g_fmapp_tx_transission_mode) {
		property_set("route.stream.to", "fm");
		ret = FM_TX_StartTransmission(fm_context);
		if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
			FMAPP_ERROR("failed to start TX transmission: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
		else
			ret = FMC_STATUS_SUCCESS;
	} else {
		ret = FM_TX_StopTransmission(fm_context);
		if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
			FMAPP_ERROR("failed to stop TX transmission: %s",
							FMC_DEBUG_FmTxStatusStr(ret));
		else
			ret = FMC_STATUS_SUCCESS;
	}

	FMAPP_END();
	return ret;
}

fm_status fmapp_init_stack(void  **fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	if (g_fmapp_rxtx_mode == FMAPP_RX_MODE)
		ret = init_rx_stack((fm_rx_context_s **) fm_context);
	else
		ret = init_tx_stack((fm_tx_context_s  **) fm_context);
	if (ret != FMC_STATUS_PENDING && ret != FMC_STATUS_SUCCESS)
		FMAPP_ERROR("failed to initialize FM stack (%s)",
						STATUS_DBG_STR(ret));
	else
		ret = FMC_STATUS_SUCCESS;

	if (set_fm_chip_enable(1) < 0)
		ret = FMC_STATUS_FAILED;
	FMAPP_END();
	return ret;
}

fm_status fmapp_deinit_stack(void  **fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	property_set("route.stream.to", "default");
	sleep(1);

	if (g_fmapp_rxtx_mode == FMAPP_RX_MODE)
		ret = deinit_rx_stack((fm_rx_context_s **) fm_context);
	else
		ret = deinit_tx_stack((fm_tx_context_s **) fm_context);
	if (ret)
		FMAPP_ERROR("failed to deinitialize FM stack (%s)",
						STATUS_DBG_STR(ret));

	if (set_fm_chip_enable(0) < 0)
		ret = FMC_STATUS_FAILED;
	FMAPP_END();
	return ret;
}

fm_status fmapp_change_power_mode(void **fm_context)
{
	fm_status ret = FMC_STATUS_FAILED;
	FMAPP_BEGIN();

	g_fmapp_power_mode = (g_fmapp_power_mode ? 0 : 1);

	if (g_fmapp_power_mode)
		/* initialize FM stack */
		ret = fmapp_init_stack(fm_context);
	else
		/* deinitialize FM stack */
		ret = fmapp_deinit_stack(fm_context);

	FMAPP_END();
	return ret;
}

void fmapp_delay(void)
{
	FMAPP_BEGIN();

	sleep(1);

	FMAPP_END();
}

inline void fmapp_interactive_printer(const char *msg)
{
	FMAPP_MSG(msg);
}

inline void fmapp_batch_printer(const char *msg)
{
	printf("%s\n ", msg);
}

void fmapp_show_rx_functionality(void (*printer)(const char *msg))
{
	printer("Available FM RX Commands:");
	printer("p power on/off");
	printer("* switch to FM TX (power must be off)");
	printer("f <freq> tune to freq");
	printer("gf get frequency");
	printer("gr get rssi level");
	printer("r turns RDS on/off");
	printer("+ increases the volume");
	printer("- decreases the volume");
	printer("v <0-70> sets the volume");
	printer("gv get volume");
	printer("b switches Japan / Eur-Us");
	printer("gb get band");
	printer("s switches stereo / mono");
	printer("gs get stereo/mono mode");
	printer("m changes mute mode");
	printer("gm get mute mode");
	printer("e set deemphasis filter");
	printer("ge get deemphasis filter");
	printer("d set rf dependent mute");
	printer("gd get rf dependent mute");
//	printer("o set audio routing");
//	printer("go get audio routing");
	printer("z set rds system");
	printer("gz get rds system");
	printer("x set rds group mask");
	printer("gx get rds group mask");
	printer("c set rds af switch");
	printer("gc get rds af switch");
//	printer("a turns AF on/off");
	printer("l prints AF list");
	printer("< seek down");
	printer("> seek up");
	printer("| stop seek");
	printer("? <(-16)-(15)> set RSSI threshold");
	printer("g? get rssi threshold");
	printer("! sleep one second (useful in script mode)");
	printer("q quit");
	printer("h display this crud");
}

void fmapp_show_tx_functionality(void (*printer)(const char *msg))
{
	printer("Available FM TX Commands:");
	printer("p power on/off");
	printer("* switch to FM RX (power must be off)");
	printer("f <freq> tune to freq");
	printer("gf get frequency");
	printer("m changes mute mode");
	printer("gm get mute mode");
	printer("s switches stereo / mono");
	printer("gs get stereo/mono mode");
	printer("r turns RDS on/off");
	printer("t turns transmission on/off");
	printer("e changes emphasis filter");
	printer("l <0-31> set power level");
	printer("gl get power level");
	printer("1 set rds transmission mode");
	printer("g1 get rds transmission mode");
	printer("2 <af> set rds af code");
	printer("g2 get rds af code");
	printer("3 <pi> set rds pi code");
	printer("g3 get rds pi code");
	printer("4 <pty> set rds pty code");
	printer("g4 get rds pty code");
	printer("5 set rds text repertoire");
	printer("g5 get rds text repertoire");
	printer("6 set rds ps display mode");
	printer("g6 get rds ps display mode");
	printer("7 <speed> set rds ps scroll speed (default is 3)");
	printer("g7 get rds ps scroll speed");
	printer("8 <msg> set rds text ps message");
	printer("g8 get rds text ps message");
	printer("9 <msg> set rds text rt message");
	printer("g9 get rds text rt message");
	printer("0 set rds transmitted groups mask");
	printer("g0 get rds transmitted groups mask");
	printer("- <0-1> <0-1> set rds traffic codes (ta, tp)");
	printer("g- get rds traffic codes");
	printer("= set rds music speech flag");
	printer("g= get rds music speech flag");
	printer("_ <ecc> set rds extended country code");
	printer("g_ get rds ecc");
	printer("+ <data> write rds raw data");
	printer("g+ read rds raw data");
	printer("h displays this crud");
	printer("q quits");
}

void fmapp_wait_until_fully_initialized(void)
{
	/*
	 * the stack does not allow queueing of commands
	 * while powering on the device.
	 * we will wait until powering on phase will pass.
	 */
	while (g_fmapp_now_initializing)
		sleep(1);
}

fm_status fmapp_execute_rx_get_command(char *cmd, int *index, fm_rx_context_s **fm_context,
							char interactive)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMC_UNUSED_PARAMETER(index);
	FMC_UNUSED_PARAMETER(interactive);
	FMAPP_BEGIN();

	switch (*cmd) {
	case 'b':
		ret = fmapp_get_band(*fm_context);
		break;
	case 's':
		ret = fmapp_get_stereo(*fm_context);
		break;
	case 'm':
		ret = fmapp_get_mute(*fm_context);
		break;
	case 'v':
		ret = fmapp_get_volume(*fm_context);
		break;
	case 'f':
		ret = fmapp_get_frequency(*fm_context);
		break;
	case 'r':
		ret = fmapp_get_rssi_level(*fm_context);
		break;
	case 'e':
		ret = fmapp_get_emphasis_filter(*fm_context);
		break;
	case '?':
		ret = fmapp_get_rssi_threshold(*fm_context);
		break;
	case 'd':
		ret = fmapp_get_rfmute(* fm_context);
		break;
//	case 'o':
//		ret = fmapp_get_audio_routing(*fm_context);
//		break;
	case 'z':
		ret = fmapp_get_rds_system(*fm_context);
		break;
	case 'x':
		ret = fmapp_get_rds_group_mask(*fm_context);
		break;
	case 'c':
		ret = fmapp_get_rds_af_switch(*fm_context);
		break;
	default:
		FMAPP_MSG("unknown command; type 'h' for help");
		break;
	}

	FMAPP_END();
	return ret;
}

fm_status fmapp_execute_rx_other_command(char *cmd, int *index, fm_rx_context_s **fm_context,
							char interactive)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	switch (*cmd) {
	case 'b':
		ret = fmapp_set_band(*fm_context);
		break;
	case 's':
		ret = fmapp_set_stereo(*fm_context);
		break;
	case 'm':
		ret = fmapp_set_mute(*fm_context);
		break;
	case '+':
		ret = fmapp_increase_volume(*fm_context);
		break;
	case 'v':
		ret = fmapp_set_volume(*fm_context, interactive, cmd+1, index);
		break;
	case '-':
		ret = fmapp_decrease_volume(*fm_context);
		break;
	case 'f':
		ret = fmapp_set_frequency(*fm_context, interactive, cmd+1, index);
		break;
	case 'd':
		ret = fmapp_set_rfmute(*fm_context);
		break;
	case 'e':
		ret = fmapp_set_emphasis_filter(*fm_context);
		break;
//	case 'o':
//		ret = fmapp_set_audio_routing(*fm_context, interactive,cmd+1,index);
//		break;
	case 'z':
		ret = fmapp_set_rds_system(*fm_context);
		break;
	case 'x':
		ret = fmapp_set_rds_group_mask(*fm_context);
		break;
	case 'c':
		ret = fmapp_set_rds_af_switch(*fm_context);
		break;
	case 'q':
		FMAPP_MSG("Quitting...");
		g_keep_running = 0;
		break;
	case '*':
		ret = fmapp_change_rxtx_mode();
		break;
//	case 'a':
//		ret = fmapp_change_af_mode(*fm_context);
//		break;
	case 'p':
		ret = fmapp_change_power_mode((void **) fm_context);
		break;
	case 'r':
		ret = fmapp_change_rds_mode(*fm_context);
		break;
	case 'l':
		fmapp_print_af_list();
		break;
	case '!':
		fmapp_delay();
		break;
	case '<':
	case '>':
		ret = fmapp_rx_seek(*fm_context, *cmd);
		break;
	case '|':
		ret = fmapp_rx_stop_seek(*fm_context);
		break;
	case '?':
		ret = fmapp_set_rssi_threshold(*fm_context, interactive, cmd+1, index);
		break;
	case 'h':
		fmapp_show_rx_functionality(fmapp_interactive_printer);
		break;
	default:
		FMAPP_MSG("unknown command; type 'h' for help");
		break;
	}

	FMAPP_END();
	return ret;
}

fm_status fmapp_execute_tx_get_command(char *cmd, int *index, fm_tx_context_s **fm_context,
							char interactive)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMC_UNUSED_PARAMETER(index);
	FMC_UNUSED_PARAMETER(interactive);
	FMAPP_BEGIN();

	switch (*cmd) {
	case 'f':
		ret = fmapp_get_frequency(*fm_context);
		break;
	case 'm':
		ret = fmapp_get_mute(*fm_context);
		break;
	case 'l':
		ret = fmapp_get_tx_power(*fm_context);
		break;
	case 's':
		ret = fmapp_get_stereo(*fm_context);
		break;
	case '1':
		ret = fmapp_get_tx_rds_transmission_mode(*fm_context);
		break;
	case '2':
		ret = fmapp_get_tx_rds_af_code(*fm_context);
		break;
	case '3':
		ret = fmapp_get_tx_rds_pi_code(*fm_context);
		break;
	case '4':
		ret = fmapp_get_tx_rds_pty_code(*fm_context);
		break;
	case '5':
		ret = fmapp_get_tx_rds_text_repertoire(*fm_context);
		break;
	case '6':
		ret = fmapp_get_tx_rds_ps_display_mode(*fm_context);
		break;
	case '7':
		ret = fmapp_get_tx_rds_scroll_speed(*fm_context);
		break;
	case '8':
		ret = fmapp_get_tx_rds_text_ps_msg(*fm_context);
		break;
	case '9':
		ret = fmapp_get_tx_rds_text_rt_msg(*fm_context);
		break;
	case '0':
		ret = fmapp_get_tx_rds_transmitted_groups_mask(*fm_context);
		break;
	case '-':
		ret = fmapp_get_tx_rds_traffic_codes(*fm_context);
		break;
	case '=':
		ret = fmapp_get_tx_rds_music_speech_flag(*fm_context);
		break;
	case '_':
		ret = fmapp_get_tx_rds_ecc(*fm_context);
		break;
	case '+':
		ret = fmapp_tx_read_rds_raw_data(*fm_context);
		break;
	default:
		FMAPP_MSG("unknown TX get command; type 'h' for help");
		break;
	}

	FMAPP_END();
	return ret;
}

fm_status fmapp_execute_tx_other_command(char *cmd, int *index, fm_tx_context_s **fm_context,
							char interactive)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	switch (*cmd) {
	case 'p':
		ret = fmapp_change_power_mode((void **) fm_context);
		break;
	case 'f':
		ret = fmapp_set_frequency(*fm_context, interactive, cmd+1, index);
		break;
	case 'm':
		ret = fmapp_set_mute(*fm_context);
		break;
	case 's':
		ret = fmapp_set_stereo(*fm_context);
		break;
	case 't':
		ret = fmapp_change_tx_transmission_mode(*fm_context);
		break;
	case 'e':
		ret = fmapp_set_emphasis_filter(*fm_context);
		break;
	case 'l':
		ret = fmapp_set_tx_power(*fm_context, interactive, cmd + 1, index);
		break;
	case '1':
		ret = fmapp_set_tx_rds_transmission_mode(*fm_context);
		break;
	case '2':
		ret = fmapp_set_tx_rds_af_code(*fm_context, interactive, cmd + 1, index);
		break;
	case '3':
		ret = fmapp_set_tx_rds_pi_code(*fm_context, interactive, cmd + 1, index);
		break;
	case '4':
		ret = fmapp_set_tx_rds_pty_code(*fm_context, interactive, cmd + 1, index);
		break;
	case '5':
		ret = fmapp_set_tx_rds_text_repertoire(*fm_context);
		break;
	case '6':
		ret = fmapp_set_tx_rds_ps_display_mode(*fm_context);
		break;
	case '7':
		ret = fmapp_set_tx_rds_scroll_speed(*fm_context, interactive, cmd + 1, index);
		break;
	case '8':
		ret = fmapp_set_tx_rds_text_ps_msg(*fm_context, interactive, cmd + 1, index);
		break;
	case '9':
		ret = fmapp_set_tx_rds_text_rt_msg(*fm_context, interactive, cmd + 1, index);
		break;
	case '0':
		ret = fmapp_set_tx_rds_transmitted_groups_mask(*fm_context);
		break;
	case '-':
		ret = fmapp_set_tx_rds_traffic_codes(*fm_context, interactive, cmd + 1, index);
		break;
	case '=':
		ret = fmapp_set_tx_rds_music_speech_flag(*fm_context);
		break;
	case '_':
		ret = fmapp_set_tx_rds_ecc(*fm_context, interactive, cmd + 1, index);
		break;
	case '+':
		ret = fmapp_tx_write_rds_raw_data(*fm_context, interactive, cmd + 1, index);
		break;
	case 'q':
		FMAPP_MSG("Quitting...");
		g_keep_running = 0;
		break;
	case '*':
		ret = fmapp_change_rxtx_mode();
		break;
	case 'r':
		ret = fmapp_change_rds_mode(*fm_context);
		break;
	case 'h':
		fmapp_show_tx_functionality(fmapp_interactive_printer);
		break;
	default:
		FMAPP_MSG("unknown command; type 'h' for help");
		break;
	}

	FMAPP_END();
	return ret;
}

fm_status fmapp_execute_rx_command(char *cmd, int *index, fm_rx_context_s **fm_context,
							char interactive)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	switch(cmd[0]) {
	case 'g':
		ret = fmapp_execute_rx_get_command(cmd+1, index, fm_context, interactive);
		break;
	default:
		ret = fmapp_execute_rx_other_command(cmd, index, fm_context, interactive);
		break;
	}

	FMAPP_END();
	return ret;
}

fm_status fmapp_execute_tx_command(char *cmd, int *index, fm_tx_context_s **fm_context,
							char interactive)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	switch(cmd[0]) {
	case 'g':
		ret = fmapp_execute_tx_get_command(cmd+1, index, fm_context, interactive);
		break;
	default:
		ret = fmapp_execute_tx_other_command(cmd, index, fm_context, interactive);
		break;
	}

	FMAPP_END();
	return ret;
}

fm_status fmapp_execute_command(char *cmd, int *index, void **fm_context,
							char interactive)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	FMAPP_TRACE("received command %d", *cmd);

	/* ignore non characters */
	if (*cmd < '!' || *cmd > '~')
		goto out;

	fmapp_wait_until_fully_initialized();

	switch(g_fmapp_rxtx_mode) {
	case FMAPP_RX_MODE:
		ret = fmapp_execute_rx_command(cmd, index, (fm_rx_context_s **) fm_context, interactive);
		break;
	case FMAPP_TX_MODE:
		ret = fmapp_execute_tx_command(cmd, index,(fm_tx_context_s **)  fm_context, interactive);
		break;
	default:
		FMAPP_ERROR("Bad internal rxtx mode");
		ret = FMC_STATUS_FAILED;
		break;
	}

out:
	FMAPP_END();
	return ret;
}

void fmapp_unmap_startup_script(void *startup, int startup_len)
{
	FMAPP_BEGIN();

	if (-1 == munmap(startup, startup_len))
		FMAPP_ERROR_SYS("failed to unmap startup script file");

	if (-1 == close(g_fmapp_startup_script_fd))
		FMAPP_ERROR_SYS("failed to close startup script file");

	g_fmapp_startup_script_fd = -1;

	FMAPP_END();
}

fm_status fmapp_mmap_startup_script(char *filename, char **startup,
							int *startup_len)
{
	struct stat buf;
	void *memory;
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	g_fmapp_startup_script_fd = open(filename, O_RDONLY);
	if (-1 == g_fmapp_startup_script_fd) {
		FMAPP_ERROR_SYS("failed to open %s", filename);
		ret = FMC_STATUS_FAILED;
		goto out;
	}

	if (-1 == fstat(g_fmapp_startup_script_fd, &buf)) {
		FMAPP_ERROR_SYS("failed to stat %s", filename);
		ret = FMC_STATUS_FAILED;
		goto closefd;
	}

	memory = mmap(NULL, buf.st_size, PROT_READ, MAP_PRIVATE,
						g_fmapp_startup_script_fd, 0);
	if (MAP_FAILED == memory) {
		FMAPP_ERROR_SYS("failed to mmap %s", filename);
		ret = FMC_STATUS_FAILED;
		goto closefd;
	}

	FMAPP_TRACE("script %s is %d bytes long", filename, buf.st_size);

	*startup = memory;
	*startup_len = buf.st_size;

	goto out;

closefd:
	if (-1 == close(g_fmapp_startup_script_fd))
		FMAPP_ERROR_SYS("failed to close startup script file");
out:
	FMAPP_END();
	return ret;
}

fm_status fmapp_startup_sequence(void **fm_context, char *startup, int startup_len)
{
	int index = 0;
	fm_status ret = FMC_STATUS_SUCCESS;
	FMAPP_BEGIN();

	FMAPP_TRACE("executing startup sequence");

	for (index = 0; index < startup_len; index++) {
		FMAPP_TRACE("executing %c", startup[index]);
		ret = fmapp_execute_command(&startup[index], &index,
						fm_context, FMAPP_BATCH);
		if (ret != FMC_STATUS_SUCCESS)
			break;
	}

	/* free resources if needed */
	if (-1 != g_fmapp_startup_script_fd)
		fmapp_unmap_startup_script(startup, startup_len);

	FMAPP_END();
	return ret;
}

static void fmapp_usage(char *app_name)
{
	printf(FMAPP_WELCOME_STR"\n");
	printf("Usage: %s [ OPTIONS ]\n", app_name);
	printf("  -i, --device=hci_dev          HCI device\n");
	printf("  -f, --frequency=freq          Initial frequency (in KHz)\n");
	printf("  -r, --rds                     Turn on RDS parsing\n");
	printf("  -v, --volume=level            Volume level (0-%d, default"
								" is %d)\n",
							FMAPP_VOLUME_LEVELS,
							FMAPP_VOLUME_LVL_INIT);
	printf("  -t, --trace=level             Trace level (1-8, default is"
								" %d)\n",
						FM_APP_TRACE_THRESHOLD);
	printf("  -y, --stack-trace=level       Stack Trace lvl  (1-8, deault"
								" is %d)\n",
						FM_STACK_TRACE_THRESHOLD);
	printf("  -s, --sript=file              Script file for automatic"
							" tests, see below\n");
	printf("  -1, --rx			Display RX command list\n");
	printf("  -2, --tx			Display RX command list\n");
	printf("  -h, --help                    Give this help list\n\n");
}

static struct option fmapp_main_options[] = {
	{ "device", 1, 0, 'i' },
	{ "trace", 1, 0, 't' },
	{ "stack-trace", 1, 0, 'y' },
	{ "frequency", 1, 0, 'f' },
	{ "volume", 1, 0, 'v' },
	{ "rds", 0, 0, 'r' },
	{ "script", 1, 0, 's' },
	{ "help", 0, 0, 'h' },
	{ "rx", 0, 0, '1' },
	{ "tx", 0, 0, '2' },
	{ NULL, 0, 0, 0 }
};

fm_status parse_options(int argc, char **argv, char **script, int *startup_len)
{
	fm_status ret = FMC_STATUS_SUCCESS;
	int opt, index = 0, ivar;
	double fvar;
	char *startup = *script, received_startup_script = 0;

	FMAPP_BEGIN();

	#define ADD_CMD_TO_STARTUP(c)	startup[index++] = (unsigned char)c
	#define ADD_VAR_TO_STARTUP(f,v)	\
		index += snprintf(&startup[index], *startup_len-index, f, v)

	/* libc should not display errors on our behalf */
	opterr = 0;
	while ((opt = getopt_long(argc, argv, "f:i:t:y:s:h12rv:",
					fmapp_main_options, NULL)) != -1) {
		switch (opt) {
		case 'i':
			g_fmapp_hci_dev = atoi(optarg);
			break;
		case 'f':
			fvar = atof(optarg);
			/* store command in startup string */
			/*
			 * futile if user also gives a sequence script, but
			 * not harmful
			 */
			ADD_CMD_TO_STARTUP(opt);
			ADD_VAR_TO_STARTUP("%2.1f", fvar);
			break;
		case 'v':
			ivar = atoi(optarg);
			/*
			 * futile if user also gives a sequence script, but
			 * not harmful
			 */
			ADD_CMD_TO_STARTUP(opt);
			ADD_VAR_TO_STARTUP("%d", ivar);
			break;
		case 't':
			g_app_trace_threshold = atoi(optarg);
			validate_uint_boundaries(&g_app_trace_threshold,
					MIN_THRESHOLD_LVL, MAX_THRESHOLD_LVL);
			break;
		case 'y':
			g_stack_trace_threshold = atoi(optarg);
			validate_uint_boundaries(&g_stack_trace_threshold,
					MIN_THRESHOLD_LVL, MAX_THRESHOLD_LVL);
			break;
		case '1':
			fmapp_show_rx_functionality(fmapp_batch_printer);
			ret = FMC_STATUS_FAILED;
			break;

		case '2':
			fmapp_show_tx_functionality(fmapp_batch_printer);
			ret = FMC_STATUS_FAILED;
			break;

		case 's':
			ret = fmapp_mmap_startup_script(optarg, script,
								startup_len);
			if (ret)
				break;
			received_startup_script = 1;
			break;
		case ':':
		case '?':
			FMAPP_ERROR("illegal command line arguments, try -h"
								" for help");
			ret = FMC_STATUS_FAILED;
			break;
		case 'h':
		default:
			fmapp_usage(argv[0]);
			ret = FMC_STATUS_FAILED;
			break;
		}

		if (index >= *startup_len) {
			FMAPP_ERROR("too many command line arguments");
			ret = FMC_STATUS_FAILED;
			break;
		}
	}

	if (!received_startup_script)
		*startup_len = index;

	FMAPP_END();
	return ret;
}

static int get_rfkill_path(char **rfkill_state_path)
{
	char path[64];
	char buf[16];
	int fd;
	int sz;
	int id;
	for (id = 0;; id++) {
		snprintf(path, sizeof(path), "/sys/class/rfkill/rfkill%d/type",
			  id);
		fd = open(path, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "open(%s) failed: %s (%d)\n", path,
				 strerror(errno), errno);
			return -1;
		}
		sz = read(fd, &buf, sizeof(buf));
		close(fd);
		if (sz >= 2 && memcmp(buf, "fm", 2) == 0) {
			break;
		}
	}
	asprintf(rfkill_state_path, "/sys/class/rfkill/rfkill%d/state", id);
	return 0;
}

int set_fm_chip_enable(int enable)
{
    return 0;
	/*
	 * const char enable_path[]="/sys/wl127x/fm_enable";
	 * change for donut branch [2.6.29 kernel only
	 */
	char *enable_path = NULL;
	/* set /sys/class/rfkill/rfkill1/state to enable FM chip */
	get_rfkill_path(&enable_path);

	char buffer='0';
	int ret;
	/* set /sys/wl127x/fm_enable=0 to enable FM chip */
	ret = open(enable_path, O_RDWR);
	if (ret < 0)
	{
		FMAPP_ERROR("Unable to open %s\n", enable_path);
		return -1;
	}
	/* FIXME: why does this require a pulse, and
	 * not just a transition from high to low
	 */
	buffer='0';
	write(ret,&buffer,1);
	buffer='1';
	write(ret,&buffer,1);
	buffer='0';
	write(ret,&buffer,1);

	if (!enable)
	{
		buffer='1';
		write(ret,&buffer,1);
	}
	close(ret);
	/* end of change for Zoom2+pg-2.0 */

    return 0;
}

int main(int argc, char **argv)
{
	char buffer[1024];
	int ret = 0, sequence_len = sizeof(buffer);
	void *fm_context = NULL;
	char *commands = buffer;

	ret = fm_trace_init();
	if (ret)
		goto fastout;

	FMAPP_BEGIN();

	fmapp_initialize_global_status();

	/* parse command line arguments */
	ret = parse_options(argc, argv, &commands, &sequence_len);
	if (ret)
		goto out;

	FMAPP_MSG(FMAPP_WELCOME_STR);
	FMAPP_MSG("attaching FM to hci%d", g_fmapp_hci_dev);

	/* register signal handlers */
	ret = register_sig_handlers();
	if (ret) {
		FMAPP_ERROR("failed to register sig handlers");
		goto out;
	}

	ret = fm_open_cmd_socket(g_fmapp_hci_dev);
	if (ret) {
		FMAPP_ERROR("failed to open cmd socket");
		goto out;
	}

	/* execute user startup sequence */
	ret = fmapp_startup_sequence(&fm_context, commands, sequence_len);
	if (ret != FMC_STATUS_SUCCESS)
		goto disable;

	FMAPP_MSG("FM RX module selected.");
	FMAPP_MSG("Press 'p' to power on, '*' to switch to FM TX or 'h' for help");

	/* main body */
	/* enjoy the music */
	while (g_keep_running) {
		fgets(buffer, sizeof(buffer), stdin);
		fmapp_execute_command(buffer, 0, &fm_context, FMAPP_INTERACTIVE);
	}

disable:
	/* deinitialize FM stack */
	if (g_fmapp_power_mode) {
		ret = fmapp_deinit_stack(&fm_context);
		if (ret) {
			FMAPP_ERROR("failed to deinitialize FM stack");
			goto out;
		}
	}

	fm_close_cmd_socket();
out:
	FMAPP_END();
	fm_trace_deinit();
fastout:
	exit(ret);
}


#ifdef ANDROID
const char *control_elements_of_interest[] = {
	"Analog Capture Volume",
	/* change for donut branch -
	 * kernel 2.6.29 has changed the mixer names
	 */
	"Analog Left Capture Route AUXL",
	"Analog Right Capture Route AUXR",
	"Left2 Analog Loopback Switch",
	"Right2 Analog Loopback Switch"
};

#define CHECK_ALSACTL_WR_STATUS(ctl,error_code,ctlname)   \
             if(error_code < 0) \
             {  \
                 FMAPP_ERROR("contorl \'%s\' element write error[error code %d]", ctlname,error_code); \
                 snd_ctl_close(ctl); \
                 return FMC_STATUS_FAILED; \
             }
/* TODO:
 * replace the hard-coded _enumerated/_boolean/_integer by
 * something better, also remove the hard-coded values_of_control
 */
int values_of_control[] = {5,3,2,1,1};
const char card[]="default";
/*
static int g_t2_default_auxl, g_t2_default_auxr;

int configure_T2_Left2_analog_switch(snd_ctl_t*, char);
int configure_T2_Right2_analog_switch(snd_ctl_t*, char);
int configure_T2_AUX_FM_volume_level(snd_ctl_t*, int);
int configure_T2_AUXL(snd_ctl_t*, char);
int configure_T2_AUXR(snd_ctl_t*, char);

int configure_T2_Left2_analog_switch(snd_ctl_t *ctl,char on_off_status)
{
     snd_ctl_elem_value_t *value;

     snd_ctl_elem_value_alloca(&value);
     snd_ctl_elem_value_set_interface(value, SND_CTL_ELEM_IFACE_MIXER);
     snd_ctl_elem_value_set_name(value,control_elements_of_interest[3]);
     snd_ctl_elem_value_set_enumerated(value,0, on_off_status);

     return(snd_ctl_elem_write(ctl, value));
}

int configure_T2_Right2_analog_switch(snd_ctl_t *ctl,char on_off_status)
{
     snd_ctl_elem_value_t *value;

     snd_ctl_elem_value_alloca(&value);
     snd_ctl_elem_value_set_interface(value, SND_CTL_ELEM_IFACE_MIXER);
     snd_ctl_elem_value_set_name(value,control_elements_of_interest[4]);
     snd_ctl_elem_value_set_enumerated(value,0, on_off_status);

     return(snd_ctl_elem_write(ctl, value));
}


int configure_T2_AUX_FM_volume_level(snd_ctl_t *ctl, int on_off_status)
{
     snd_ctl_elem_value_t *value;

     snd_ctl_elem_value_alloca(&value);
     snd_ctl_elem_value_set_interface(value, SND_CTL_ELEM_IFACE_MIXER);
     snd_ctl_elem_value_set_name(value,control_elements_of_interest[0]);
	snd_ctl_elem_value_set_integer(value, 0, on_off_status);
	snd_ctl_elem_value_set_integer(value, 1, on_off_status);

     return(snd_ctl_elem_write(ctl, value));
}

int configure_T2_AUXL(snd_ctl_t *ctl,char on_off_status)
{
     snd_ctl_elem_value_t *value;

     snd_ctl_elem_value_alloca(&value);
     snd_ctl_elem_value_set_interface(value, SND_CTL_ELEM_IFACE_MIXER);
     snd_ctl_elem_value_set_name(value,control_elements_of_interest[1]);
	if (on_off_status) {
		g_t2_default_auxl = snd_ctl_elem_value_get_enumerated(value,0);
		snd_ctl_elem_value_set_enumerated(value,0, on_off_status);
	} else {
		snd_ctl_elem_value_set_enumerated(value,0, g_t2_default_auxl);
	}

     return(snd_ctl_elem_write(ctl, value));
}

int configure_T2_AUXR(snd_ctl_t *ctl,char on_off_status)
{
     snd_ctl_elem_value_t *value;

     snd_ctl_elem_value_alloca(&value);
     snd_ctl_elem_value_set_interface(value, SND_CTL_ELEM_IFACE_MIXER);
     snd_ctl_elem_value_set_name(value,control_elements_of_interest[2]);
        if (on_off_status) {
                g_t2_default_auxr = snd_ctl_elem_value_get_enumerated(value,0);
                snd_ctl_elem_value_set_enumerated(value,0, on_off_status);
        } else {
                snd_ctl_elem_value_set_enumerated(value,0, g_t2_default_auxr);
        }

     return(snd_ctl_elem_write(ctl, value));
}

fm_status set_fmapp_audio_routing(fm_rx_context_s *fm_context)
{
     int error_code;
     snd_ctl_t *ctl;

	 (void)fm_context;
     FMAPP_BEGIN();
     error_code = snd_ctl_open(&ctl,card, 0);
     if(error_code < 0)
     {
         FMAPP_ERROR("Control \'%s\' open error [error code %d]",card, error_code);
         return FMC_STATUS_FAILED;
     }

     error_code = configure_T2_AUX_FM_volume_level(ctl, values_of_control[0]);
     CHECK_ALSACTL_WR_STATUS(ctl,error_code,control_elements_of_interest[0]);

     error_code = configure_T2_AUXL(ctl,values_of_control[1]);
     CHECK_ALSACTL_WR_STATUS(ctl,error_code,control_elements_of_interest[1]);

     error_code = configure_T2_AUXR(ctl,values_of_control[2]);
     CHECK_ALSACTL_WR_STATUS(ctl,error_code,control_elements_of_interest[2]);


     error_code = configure_T2_Left2_analog_switch(ctl,values_of_control[3]);
     CHECK_ALSACTL_WR_STATUS(ctl,error_code,control_elements_of_interest[3]);

     error_code = configure_T2_Right2_analog_switch(ctl,values_of_control[4]);
     CHECK_ALSACTL_WR_STATUS(ctl,error_code,control_elements_of_interest[4]);

     snd_ctl_close(ctl);

     FMAPP_END();

     return FMC_STATUS_SUCCESS;
}

fm_status unset_fmapp_audio_routing(fm_rx_context_s *fm_context)
{
     int error_code;
     snd_ctl_t *ctl;

	 (void)fm_context;
     FMAPP_BEGIN();
     error_code = snd_ctl_open(&ctl,card, 0);
     if(error_code < 0)
     {
         FMAPP_ERROR("Control \'%s\' open error [error code %d]",card, error_code);
         return FMC_STATUS_FAILED;
     }

     error_code = configure_T2_AUX_FM_volume_level(ctl, 0);
     CHECK_ALSACTL_WR_STATUS(ctl,error_code,control_elements_of_interest[0]);

     error_code = configure_T2_AUXL(ctl,0);
     CHECK_ALSACTL_WR_STATUS(ctl,error_code,control_elements_of_interest[1]);

     error_code = configure_T2_AUXR(ctl,0);
     CHECK_ALSACTL_WR_STATUS(ctl,error_code,control_elements_of_interest[2]);

     error_code = configure_T2_Left2_analog_switch(ctl,0);
     CHECK_ALSACTL_WR_STATUS(ctl,error_code,control_elements_of_interest[3]);

     error_code = configure_T2_Right2_analog_switch(ctl,0);
     CHECK_ALSACTL_WR_STATUS(ctl,error_code,control_elements_of_interest[4]);

     snd_ctl_close(ctl);

     FMAPP_END();

     return FMC_STATUS_SUCCESS;
}
*/

#endif
