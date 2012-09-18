/*
 * This file is part of wl1271
 *
 * Copyright (C) 2009-2010 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
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

#ifndef __SCAN_H__
#define __SCAN_H__

#include "wlcore.h"

int wl1271_scan(struct wl1271 *wl, struct ieee80211_vif *vif,
		const u8 *ssid, size_t ssid_len,
		struct cfg80211_scan_request *req);
int wl1271_scan_stop(struct wl1271 *wl);
int wl1271_scan_build_probe_req(struct wl1271 *wl,
				const u8 *ssid, size_t ssid_len,
				const u8 *ie, size_t ie_len, u8 band);
//void wl1271_scan_stm(struct wl1271 *wl, struct ieee80211_vif *vif);
void wl12xx_scan_completed(struct wl1271 *wl);
void wl1271_scan_complete_work(struct work_struct *work);

int wl1271_scan_sched_scan_config(struct wl1271 *wl,
				  struct wl12xx_vif *wlvif,
				  struct cfg80211_sched_scan_request *req,
				  struct ieee80211_sched_scan_ies *ies);
void wl1271_scan_sched_scan_stop(struct wl1271 *wl, struct wl12xx_vif *wlvif);
/*
int wl1271_scan_sched_scan_start(struct wl1271 *wl, struct wl12xx_vif *wlvif);
void wl1271_scan_sched_scan_results(struct wl1271 *wl);
*/


#define WL1271_SCAN_MAX_CHANNELS       24
#define WL1271_SCAN_DEFAULT_TAG        1
#define WL1271_SCAN_CURRENT_TX_PWR     0
#define WL1271_SCAN_OPT_ACTIVE         0
#define WL1271_SCAN_OPT_PASSIVE	       1
#define WL1271_SCAN_OPT_SPLIT_SCAN     2
#define WL1271_SCAN_OPT_PRIORITY_HIGH  4
/* scan even if we fail to enter psm */
#define WL1271_SCAN_OPT_FORCE          8
#define WL1271_SCAN_BAND_2_4_GHZ 0
#define WL1271_SCAN_BAND_5_GHZ 1

#define WL1271_SCAN_TIMEOUT    30000 /* msec */

enum {
	WL1271_SCAN_STATE_IDLE,
	WL1271_SCAN_STATE_2GHZ_ACTIVE,
	WL1271_SCAN_STATE_2GHZ_PASSIVE,
	WL1271_SCAN_STATE_5GHZ_ACTIVE,
	WL1271_SCAN_STATE_5GHZ_PASSIVE,
	WL1271_SCAN_STATE_DONE
};
#if 0
struct basic_scan_params {
	/* Scan option flags (WL1271_SCAN_OPT_*) */
	u8 scan_options;
	u8 role_id;
	/* Number of scan channels in the list (maximum 30) */
	u8 n_ch;
	/* This field indicates the number of probe requests to send
	   per channel for an active scan */
	u8 n_probe_reqs;
	//u8 tid_trigger;
	u8 ssid_len;
	u8 use_ssid_list;

	/* TODO: padding */
	/* Rate bit field for sending the probes */
	__le32 tx_rate;

	u8 ssid[IEEE80211_MAX_SSID_LEN];
	/* Band to scan */
	u8 band;

	u8 scan_tag;
	u8 padding2[2];
} __packed;

struct basic_scan_channel_params {
	/* Duration in TU to wait for frames on a channel for active scan */
	__le32 min_duration;
	__le32 max_duration;
	__le32 bssid_lsb;
	__le16 bssid_msb;
	u8 early_termination;
	u8 tx_power_att;
	u8 channel;
	/* FW internal use only! */
	u8 dfs_candidate;
	u8 activity_detected;
	u8 pad;
} __packed;

struct wl1271_cmd_scan {
	struct wl1271_cmd_header header;

	struct basic_scan_params params;
	struct basic_scan_channel_params channels[WL1271_SCAN_MAX_CHANNELS];
} __packed;
#endif
struct wl1271_cmd_trigger_scan_to {
	struct wl1271_cmd_header header;

	__le32 timeout;
} __packed;

#define MAX_CHANNELS_2GHZ	14
#define MAX_CHANNELS_5GHZ	24
#define MAX_CHANNELS_4GHZ	4

#define SCAN_MAX_CYCLE_INTERVALS 16

/* The FW intervals can take up to 16 entries.
 * The 1st entry isn't used (scan is immediate). The last
 * entry should be used for the long_interval
 */
#define SCAN_MAX_SHORT_INTERVALS (SCAN_MAX_CYCLE_INTERVALS - 2)
#define SCAN_MAX_BANDS 3

enum {
	SCAN_SSID_FILTER_ANY      = 0,
	SCAN_SSID_FILTER_SPECIFIC = 1,
	SCAN_SSID_FILTER_LIST     = 2,
	SCAN_SSID_FILTER_DISABLED = 3
};

enum {
	SCAN_BSS_TYPE_INDEPENDENT,
	SCAN_BSS_TYPE_INFRASTRUCTURE,
	SCAN_BSS_TYPE_ANY,
};

#define SCAN_CHANNEL_FLAGS_DFS		BIT(0) /* channel is passive until an
						  activity is detected on it */
#define SCAN_CHANNEL_FLAGS_DFS_ENABLED	BIT(1)

struct conn_scan_ch_params {
	__le16 min_duration;
	__le16 max_duration;
	__le16 passive_duration;

	u8  channel;
	u8  tx_power_att;

	/* bit 0: DFS channel; bit 1: DFS enabled */
	u8  flags;

	u8  padding[3];
} __packed;

struct tracking_ch_params {
	struct conn_scan_ch_params channel;

	__le32 bssid_lsb;
	__le16 bssid_msb;

	u8 padding[2];
} __packed;

enum
{
	SCAN_TYPE_SEARCH	= 0,
	SCAN_TYPE_PERIODIC	= 1,
	SCAN_TYPE_TRACKING	= 2,
};

/* probe request rate */
enum
{
	WLCORE_SCAN_RATE_1	= 0,
	WLCORE_SCAN_RATE_5_5	= 1,
	WLCORE_SCAN_RATE_6	= 2,
};

struct wl1271_cmd_scan_params {
	struct wl1271_cmd_header header;

	u8 role_id;
	u8 scan_type;

	s8 rssi_threshold; /* for filtering (in dBm) */
	s8 snr_threshold;  /* for filtering (in dB) */

	u8 bss_type;	   /* for filtering */
	u8 ssid_from_list; /* use ssid from configured ssid list */
	u8 filter;	   /* forward only results with matching ssids */

	/*
	 * add broadcast ssid in addition to the configured ssids.
	 * the driver should add dummy entry for it (?).
	 */
	u8 add_broadcast;

	u8 urgency;
	u8 protect;	 /* ??? */
	u8 n_probe_reqs;    /* Number of probes requests per channel */
	u8 terminate_after; /* early terminate scan operation */

	u8 passive[SCAN_MAX_BANDS]; /* number of passive scan channels */
	u8 active[SCAN_MAX_BANDS];  /* number of active scan channels */
	u8 dfs;		   /* number of dfs channels in 5ghz */
	u8 passive_active; /* number of passive before active channels 2.4ghz */

	__le16 short_cycles_sec;
	__le16 long_cycles_sec;
	u8 short_cycles_count;
	u8 total_cycles; /* 0 - infinite */
	u8 padding[2];

	union {
		struct {
			struct conn_scan_ch_params channels_2[MAX_CHANNELS_2GHZ];
			struct conn_scan_ch_params channels_5[MAX_CHANNELS_5GHZ];
			struct conn_scan_ch_params channels_4[MAX_CHANNELS_4GHZ];
		};
		struct tracking_ch_params channels_tracking[WL1271_SCAN_MAX_CHANNELS];
	} ;

	u8 ssid[IEEE80211_MAX_SSID_LEN];
	u8 ssid_len;	 /* For SCAN_SSID_FILTER_SPECIFIC */
	u8 tag;
	u8 rate;
	u8 padding1[1];
} __packed;


#define SCHED_SCAN_MAX_SSIDS 16

enum {
	SCAN_SSID_TYPE_PUBLIC = 0,
	SCAN_SSID_TYPE_HIDDEN = 1,
};

struct wl1271_ssid {
	u8 type;
	u8 len;
	u8 ssid[IEEE80211_MAX_SSID_LEN];
	/* u8 padding[2]; */
} __packed;

struct wl1271_cmd_sched_scan_ssid_list {
	struct wl1271_cmd_header header;

	u8 n_ssids;
	struct wl1271_ssid ssids[SCHED_SCAN_MAX_SSIDS];
	u8 role_id;
	u8 padding[2];
} __packed;

struct wl1271_cmd_sched_scan_start {
	struct wl1271_cmd_header header;

	u8 tag;
	u8 role_id;
	u8 padding[2];
} __packed;

struct wl1271_cmd_sched_scan_stop {
	struct wl1271_cmd_header header;

	u8 tag;
	u8 role_id;
	u8 padding[2];
} __packed;

struct wl12xx_cmd_scan_stop {
	struct wl1271_cmd_header header;

	u8 role_id;
	u8 scan_type;
	u8 padding[2];
} __packed;

#endif /* __WL1271_SCAN_H__ */
