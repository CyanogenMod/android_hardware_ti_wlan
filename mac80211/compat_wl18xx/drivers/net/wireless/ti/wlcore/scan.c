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

#include <linux/ieee80211.h>

#include "wlcore.h"
#include "debug.h"
#include "cmd.h"
#include "scan.h"
#include "acx.h"
#include "ps.h"
#include "tx.h"

void wl1271_scan_complete_work(struct work_struct *work)
{
	struct delayed_work *dwork;
	struct wl1271 *wl;
	struct ieee80211_vif *vif;
	struct wl12xx_vif *wlvif;
	int ret;

	dwork = container_of(work, struct delayed_work, work);
	wl = container_of(dwork, struct wl1271, scan_complete_work);

	wl1271_debug(DEBUG_SCAN, "Scanning complete");

	mutex_lock(&wl->mutex);

	if (unlikely(wl->state != WLCORE_STATE_ON))
		goto out;

	if (wl->scan.state == WL1271_SCAN_STATE_IDLE)
		goto out;

	vif = wl->scan_vif;
	wlvif = wl12xx_vif_to_data(vif);

	/*
	 * Rearm the tx watchdog just before idling scan. This
	 * prevents just-finished scans from triggering the watchdog
	 */
	wl12xx_rearm_tx_watchdog_locked(wl);

	wl->scan.state = WL1271_SCAN_STATE_IDLE;
	memset(wl->scan.scanned_ch, 0, sizeof(wl->scan.scanned_ch));
	wl->scan.req = NULL;
	wl->scan_vif = NULL;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	if (test_bit(WLVIF_FLAG_STA_ASSOCIATED, &wlvif->flags)) {
		/* restore hardware connection monitoring template */
		wl1271_cmd_build_ap_probe_req(wl, wlvif, wlvif->probereq);
	}

	wl1271_ps_elp_sleep(wl);

	if (wl->scan.failed) {
		wl1271_info("Scan completed due to error.");
		wl12xx_queue_recovery_work(wl);
	}

	ieee80211_scan_completed(wl->hw, false);

out:
	mutex_unlock(&wl->mutex);

}

static int
wlcore_scan_get_channels(struct wl1271 *wl,
			 struct ieee80211_channel *req_channels[],
			 u32 n_channels,
			 u32 n_ssids,
			 struct conn_scan_ch_params *channels,
			 u32 band, bool radar, bool passive,
			 int start, int max_channels,
			 int scan_type)
{
	int i, j;
	u32 flags;
	bool force_passive = !n_ssids;
	u32 min_dwell_time_active, max_dwell_time_active, delta_per_probe;
	u32 dwell_time_passive, dwell_time_dfs;

	if (scan_type == SCAN_TYPE_SEARCH) {
		struct conf_scan_settings *c = &wl->conf.scan;

		min_dwell_time_active = c->min_dwell_time_active;
		max_dwell_time_active = c->max_dwell_time_active;
		/* TODO: convert conf from min/max to a single value */
		dwell_time_passive = c->dwell_time_passive;
		dwell_time_dfs = c->dwell_time_dfs;
	} else {
		struct conf_sched_scan_settings *c = &wl->conf.sched_scan;

		if (band == IEEE80211_BAND_5GHZ)
			delta_per_probe = c->dwell_time_delta_per_probe_5;
		else
			delta_per_probe = c->dwell_time_delta_per_probe;

		min_dwell_time_active = c->base_dwell_time +
			 n_ssids * c->num_probe_reqs * delta_per_probe;

		max_dwell_time_active = min_dwell_time_active +
					c->max_dwell_time_delta;
		dwell_time_passive = c->dwell_time_passive;
		dwell_time_dfs = c->dwell_time_dfs;
	}
	/* TODO: use msec in conf... */
	min_dwell_time_active = DIV_ROUND_UP(min_dwell_time_active, 1000);
	max_dwell_time_active = DIV_ROUND_UP(max_dwell_time_active, 1000);
	dwell_time_passive = DIV_ROUND_UP(dwell_time_passive, 1000);
	dwell_time_dfs = DIV_ROUND_UP(dwell_time_dfs, 1000);

	for (i = 0, j = start;
	     i < n_channels && j < max_channels;
	     i++) {
		flags = req_channels[i]->flags;

		if (force_passive)
			flags |= IEEE80211_CHAN_PASSIVE_SCAN;

		if ((req_channels[i]->band == band) &&
		    !(flags & IEEE80211_CHAN_DISABLED) &&
		    (!!(flags & IEEE80211_CHAN_RADAR) == radar) &&
		    /* if radar is set, we ignore the passive flag */
		    (radar ||
		     !!(flags & IEEE80211_CHAN_PASSIVE_SCAN) == passive)) {
			wl1271_debug(DEBUG_SCAN, "band %d, center_freq %d ",
				     req_channels[i]->band,
				     req_channels[i]->center_freq);
			wl1271_debug(DEBUG_SCAN, "hw_value %d, flags %X",
				     req_channels[i]->hw_value,
				     req_channels[i]->flags);
			wl1271_debug(DEBUG_SCAN, "max_power %d",
				     req_channels[i]->max_power);
			wl1271_debug(DEBUG_SCAN, "min_dwell_time %d max dwell time %d",
				     min_dwell_time_active,
				     max_dwell_time_active);

			if (flags & IEEE80211_CHAN_RADAR) {
				channels[j].flags |= SCAN_CHANNEL_FLAGS_DFS;

				channels[j].passive_duration =
					cpu_to_le16(dwell_time_dfs);
			} else {
				channels[j].passive_duration =
					cpu_to_le16(dwell_time_passive);
			}

			channels[j].min_duration =
				cpu_to_le16(min_dwell_time_active);
			channels[j].max_duration =
				cpu_to_le16(max_dwell_time_active);

			channels[j].tx_power_att = req_channels[i]->max_power;
			channels[j].channel = req_channels[i]->hw_value;
#if 0
			if ((band == IEEE80211_BAND_2GHZ) &&
			    (channels[j].channel >= 12) &&
			    (channels[j].channel <= 14) &&
			    (flags & IEEE80211_CHAN_PASSIVE_SCAN) &&
			    !force_passive) {
				/* pactive channels treated as DFS */
				channels[j].flags = SCAN_CHANNEL_FLAGS_DFS;

				/*
				 * n_pactive_ch is counted down from the end of
				 * the passive channel list
				 */
				(*n_pactive_ch)++;
				wl1271_debug(DEBUG_SCAN, "n_pactive_ch = %d",
					     *n_pactive_ch);
			}
#endif
			j++;
		}
	}

	return j - start;
}

static bool
wlcore_set_scan_chan_params(struct wl1271 *wl,
			    struct wl1271_cmd_scan_params *cfg,
			    struct ieee80211_channel *channels[],
			    u32 n_channels,
			    u32 n_ssids,
			    int scan_type)
{
	u8 n_pactive_ch = 0;

	cfg->passive[0] =
		wlcore_scan_get_channels(wl,
					 channels,
					 n_channels,
					 n_ssids,
					 cfg->channels_2,
					 IEEE80211_BAND_2GHZ,
					 false, true, 0,
					 MAX_CHANNELS_2GHZ,
					 scan_type);
	cfg->active[0] =
		wlcore_scan_get_channels(wl,
					 channels,
					 n_channels,
					 n_ssids,
					 cfg->channels_2,
					 IEEE80211_BAND_2GHZ,
					 false, false,
					 cfg->passive[0],
					 MAX_CHANNELS_2GHZ,
					 scan_type);
	cfg->passive[1] =
		wlcore_scan_get_channels(wl,
					 channels,
					 n_channels,
					 n_ssids,
					 cfg->channels_5,
					 IEEE80211_BAND_5GHZ,
					 false, true, 0,
					 MAX_CHANNELS_5GHZ,
					 scan_type);
	cfg->dfs =
		wlcore_scan_get_channels(wl,
					 channels,
					 n_channels,
					 n_ssids,
					 cfg->channels_5,
					 IEEE80211_BAND_5GHZ,
					 true, true,
					 cfg->passive[1],
					 MAX_CHANNELS_5GHZ,
					 scan_type);
	cfg->active[1] =
		wlcore_scan_get_channels(wl,
					 channels,
					 n_channels,
					 n_ssids,
					 cfg->channels_5,
					 IEEE80211_BAND_5GHZ,
					 false, false,
					 cfg->passive[1] + cfg->dfs,
					 MAX_CHANNELS_5GHZ,
					 scan_type);
	/* TODO: add passive-after-active configuration */
	/* 802.11j channels are not supported yet */
	cfg->passive[2] = 0;
	cfg->active[2] = 0;

	cfg->passive_active = n_pactive_ch;

	wl1271_debug(DEBUG_SCAN, "    2.4GHz: active %d passive %d",
		     cfg->active[0], cfg->passive[0]);
	wl1271_debug(DEBUG_SCAN, "    5GHz: active %d passive %d",
		     cfg->active[1], cfg->passive[1]);
	wl1271_debug(DEBUG_SCAN, "    DFS: %d", cfg->dfs);

	return  cfg->passive[0] || cfg->active[0] ||
		cfg->passive[1] || cfg->active[1] || cfg->dfs ||
		cfg->passive[2] || cfg->active[2];
}

static int wl1271_scan_send(struct wl1271 *wl, struct ieee80211_vif *vif,
			    struct cfg80211_scan_request *req)
{
	struct wl12xx_vif *wlvif = wl12xx_vif_to_data(vif);
	struct wl1271_cmd_scan_params *cmd;
	int ret;

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	/* TODO: delete split_scan_timeout conf */

	/* scan on the dev role if the regular one is not started */
	if (wlvif->role_id == WL12XX_INVALID_ROLE_ID)
		cmd->role_id = wlvif->dev_role_id;
	else
		cmd->role_id = wlvif->role_id;

	if (WARN_ON(cmd->role_id == WL12XX_INVALID_ROLE_ID)) {
		ret = -EINVAL;
		goto out;
	}

	cmd->scan_type = SCAN_TYPE_SEARCH;
	cmd->rssi_threshold = -127;
	cmd->snr_threshold = 0;

	/* TODO: use wlvif->type instead? */
	cmd->bss_type = SCAN_BSS_TYPE_ANY;

	cmd->ssid_from_list = 0;
	cmd->filter = 0;
	cmd->add_broadcast = 0;

	/* TODO: figure this ones out */
	cmd->urgency = 0;
	cmd->protect = 0;

	/* TODO: take num_probe from req if available */
	cmd->n_probe_reqs = wl->conf.scan.num_probe_reqs;
	cmd->terminate_after = 0;

	/* configure channels */
	WARN_ON(req->n_ssids > 1); /* TODO: support multi ssid */
	wlcore_set_scan_chan_params(wl, cmd, req->channels,
				    req->n_channels, req->n_ssids,
				    SCAN_TYPE_SEARCH);

	/*
	 * all the cycles params (except total cycles) should
	 * remain 0 for normal scan
	 */
	cmd->total_cycles = 1;

	if (req->no_cck)
		cmd->rate = WLCORE_SCAN_RATE_6;

	cmd->tag = WL1271_SCAN_DEFAULT_TAG;

	if (req->n_ssids) {
		cmd->ssid_len = req->ssids[0].ssid_len;
		memcpy(cmd->ssid, req->ssids[0].ssid, cmd->ssid_len);
	}

	/* TODO: per-band ies? */
	if (cmd->active[0]) {
		u8 band = IEEE80211_BAND_2GHZ;
		ret = wl12xx_cmd_build_probe_req(wl, wlvif,
						 cmd->role_id, band,
						 req->ssids[0].ssid,
						 req->ssids[0].ssid_len,
						 req->ie,
						 req->ie_len,
						 false);
		if (ret < 0) {
			wl1271_error("2.4GHz PROBE request template failed");
			goto out;
		}
	}

	if (cmd->active[1]) {
		u8 band = IEEE80211_BAND_5GHZ;
		ret = wl12xx_cmd_build_probe_req(wl, wlvif,
						 cmd->role_id, band,
						 req->ssids[0].ssid,
						 req->ssids[0].ssid_len,
						 req->ie,
						 req->ie_len,
						 false);
		if (ret < 0) {
			wl1271_error("5GHz PROBE request template failed");
			goto out;
		}
	}

	wl1271_dump(DEBUG_SCAN, "SCAN: ", cmd, sizeof(*cmd));

	ret = wl1271_cmd_send(wl, CMD_SCAN, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wl1271_error("SCAN failed");
		goto out;
	}

out:
	kfree(cmd);
	return ret;
}

void wl12xx_scan_completed(struct wl1271 *wl)
{
	wl->scan.failed = false;
	cancel_delayed_work(&wl->scan_complete_work);
	ieee80211_queue_delayed_work(wl->hw, &wl->scan_complete_work,
				     msecs_to_jiffies(0));
}

int wl1271_scan(struct wl1271 *wl, struct ieee80211_vif *vif,
		const u8 *ssid, size_t ssid_len,
		struct cfg80211_scan_request *req)
{
	/*
	 * cfg80211 should guarantee that we don't get more channels
	 * than what we have registered.
	 */
	BUG_ON(req->n_channels > WL1271_MAX_CHANNELS);

	if (wl->scan.state != WL1271_SCAN_STATE_IDLE)
		return -EBUSY;

	wl->scan.state = WL1271_SCAN_STATE_2GHZ_ACTIVE;

	if (ssid_len && ssid) {
		wl->scan.ssid_len = ssid_len;
		memcpy(wl->scan.ssid, ssid, ssid_len);
	} else {
		wl->scan.ssid_len = 0;
	}

	wl->scan_vif = vif;
	wl->scan.req = req;
	memset(wl->scan.scanned_ch, 0, sizeof(wl->scan.scanned_ch));

	/* we assume failure so that timeout scenarios are handled correctly */
	wl->scan.failed = true;
	ieee80211_queue_delayed_work(wl->hw, &wl->scan_complete_work,
				     msecs_to_jiffies(WL1271_SCAN_TIMEOUT));

	wl1271_scan_send(wl, vif, req);

	return 0;
}

int wl1271_scan_stop(struct wl1271 *wl)
{
	struct wl1271_cmd_header *cmd = NULL;
	int ret = 0;

	if (WARN_ON(wl->scan.state == WL1271_SCAN_STATE_IDLE))
		return -EINVAL;

	wl1271_debug(DEBUG_CMD, "cmd scan stop");

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	ret = wl1271_cmd_send(wl, CMD_STOP_SCAN, cmd,
			      sizeof(*cmd), 0);
	if (ret < 0) {
		wl1271_error("cmd stop_scan failed");
		goto out;
	}
out:
	kfree(cmd);
	return ret;
}

/* Returns the scan type to be used or a negative value on error */
static int
wl12xx_scan_set_ssid_list(struct wl1271 *wl,
				struct cfg80211_sched_scan_request *req)
{
	struct wl1271_cmd_sched_scan_ssid_list *cmd = NULL;
	struct cfg80211_match_set *sets = req->match_sets;
	struct cfg80211_ssid *ssids = req->ssids;
	int ret = 0, type, i, j, n_match_ssids = 0;

	wl1271_debug(DEBUG_CMD, "cmd scan ssid list");

	/* count the match sets that contain SSIDs */
	for (i = 0; i < req->n_match_sets; i++)
		if (sets[i].ssid.ssid_len > 0)
			n_match_ssids++;

	/* No filter, no ssids or only bcast ssid */
	if (!n_match_ssids &&
	    (!req->n_ssids ||
	     (req->n_ssids == 1 && req->ssids[0].ssid_len == 0))) {
		type = SCAN_SSID_FILTER_ANY;
		goto out;
	}

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	if (!n_match_ssids) {
		/* No filter, with ssids */
		type = SCAN_SSID_FILTER_DISABLED;

		for (i = 0; i < req->n_ssids; i++) {
			cmd->ssids[cmd->n_ssids].type = (ssids[i].ssid_len) ?
				SCAN_SSID_TYPE_HIDDEN : SCAN_SSID_TYPE_PUBLIC;
			cmd->ssids[cmd->n_ssids].len = ssids[i].ssid_len;
			memcpy(cmd->ssids[cmd->n_ssids].ssid, ssids[i].ssid,
			       ssids[i].ssid_len);
			cmd->n_ssids++;
		}
	} else {
		type = SCAN_SSID_FILTER_LIST;

		/* Add all SSIDs from the filters */
		for (i = 0; i < req->n_match_sets; i++) {
			/* ignore sets without SSIDs */
			if (!sets[i].ssid.ssid_len)
				continue;

			cmd->ssids[cmd->n_ssids].type = SCAN_SSID_TYPE_PUBLIC;
			cmd->ssids[cmd->n_ssids].len = sets[i].ssid.ssid_len;
			memcpy(cmd->ssids[cmd->n_ssids].ssid,
			       sets[i].ssid.ssid, sets[i].ssid.ssid_len);
			cmd->n_ssids++;
		}
		if ((req->n_ssids > 1) ||
		    (req->n_ssids == 1 && req->ssids[0].ssid_len > 0)) {
			/*
			 * Mark all the SSIDs passed in the SSID list as HIDDEN,
			 * so they're used in probe requests.
			 */
			for (i = 0; i < req->n_ssids; i++) {
				if (!req->ssids[i].ssid_len)
					continue;

				for (j = 0; j < cmd->n_ssids; j++)
					if ((req->ssids[i].ssid_len ==
					     cmd->ssids[j].len) &&
					    !memcmp(req->ssids[i].ssid,
						   cmd->ssids[j].ssid,
						   req->ssids[i].ssid_len)) {
						cmd->ssids[j].type =
							SCAN_SSID_TYPE_HIDDEN;
						break;
					}
				/* Fail if SSID isn't present in the filters */
				if (j == cmd->n_ssids) {
					ret = -EINVAL;
					goto out_free;
				}
			}
		}
	}

	wl1271_dump(DEBUG_SCAN, "SSID_LIST: ", cmd, sizeof(*cmd));

	ret = wl1271_cmd_send(wl, CMD_CONNECTION_SCAN_SSID_CFG, cmd,
			      sizeof(*cmd), 0);
	if (ret < 0) {
		wl1271_error("cmd sched scan ssid list failed");
		goto out_free;
	}

out_free:
	kfree(cmd);
out:
	if (ret < 0)
		return ret;
	return type;
}


int wl1271_scan_sched_scan_config(struct wl1271 *wl,
				  struct wl12xx_vif *wlvif,
				  struct cfg80211_sched_scan_request *req,
				  struct ieee80211_sched_scan_ies *ies)
{
	struct wl1271_cmd_scan_params *cmd;
	struct conf_sched_scan_settings *c = &wl->conf.sched_scan;
	int ret;
	int filter_type;

	wl1271_debug(DEBUG_CMD, "cmd sched_scan scan config");

	if (req->n_short_intervals > SCAN_MAX_SHORT_INTERVALS) {
		wl1271_warning("Number of short intervals requested (%d)"
			       "exceeds limit (%d)", req->n_short_intervals,
			       SCAN_MAX_SHORT_INTERVALS);
		return -EINVAL;
	}

	filter_type = wl12xx_scan_set_ssid_list(wl,req);
	if (filter_type < 0)
		return filter_type;

	cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
	if (!cmd) {
		ret = -ENOMEM;
		goto out;
	}

	cmd->role_id = wlvif->role_id;
	/* report APs when at least 1 is found */
	//cfg->report_after = 1;

	if (WARN_ON(cmd->role_id == WL12XX_INVALID_ROLE_ID)) {
		ret = -EINVAL;
		goto out;
	}

	cmd->scan_type = SCAN_TYPE_PERIODIC;
	cmd->rssi_threshold = c->rssi_threshold;
	cmd->snr_threshold = c->snr_threshold;

	/* don't filter on BSS type */
	cmd->bss_type = SCAN_BSS_TYPE_ANY;

	cmd->ssid_from_list = 1;
	if (filter_type == SCAN_SSID_FILTER_LIST)
		cmd->filter = 1;
	cmd->add_broadcast = 0;

	/* TODO: figure this ones out */
	cmd->urgency = 0;
	cmd->protect = 0;

	cmd->n_probe_reqs = c->num_probe_reqs;
	/* don't stop scanning automatically when something is found */
	cmd->terminate_after = 0;

	/* configure channels */
	wlcore_set_scan_chan_params(wl, cmd, req->channels,
				    req->n_channels, req->n_ssids,
				    SCAN_TYPE_PERIODIC);

	/* TODO: check params */
	cmd->short_cycles_sec = req->short_interval;
	cmd->long_cycles_sec = req->long_interval;
	cmd->short_cycles_count = req->n_short_intervals;
	cmd->total_cycles = 0;

	/* TODO: how to set tx rate? */

	cmd->tag = WL1271_SCAN_DEFAULT_TAG;

	if (cmd->active[0]) {
		u8 band = IEEE80211_BAND_2GHZ;
		ret = wl12xx_cmd_build_probe_req(wl, wlvif,
						 cmd->role_id, band,
						 req->ssids[0].ssid,
						 req->ssids[0].ssid_len,
						 ies->ie[band],
						 ies->len[band],
						 true);
		if (ret < 0) {
			wl1271_error("2.4GHz PROBE request template failed");
			goto out;
		}
	}

	if (cmd->active[1]) {
		u8 band = IEEE80211_BAND_5GHZ;
		ret = wl12xx_cmd_build_probe_req(wl, wlvif,
						 cmd->role_id, band,
						 req->ssids[0].ssid,
						 req->ssids[0].ssid_len,
						 ies->ie[band],
						 ies->len[band],
						 true);
		if (ret < 0) {
			wl1271_error("5GHz PROBE request template failed");
			goto out;
		}
	}

	wl1271_dump(DEBUG_SCAN, "SCAN: ", cmd, sizeof(*cmd));

	wl1271_info("scan size: %d", sizeof(*cmd));
	ret = wl1271_cmd_send(wl, CMD_SCAN, cmd, sizeof(*cmd), 0);
	if (ret < 0) {
		wl1271_error("SCAN failed");
		goto out;
	}

out:
	kfree(cmd);
	return ret;
}

void wl1271_scan_sched_scan_results(struct wl1271 *wl)
{
	wl1271_debug(DEBUG_SCAN, "got periodic scan results");

	ieee80211_sched_scan_results(wl->hw);
}

/* TODO: unify stop functions */
void wl1271_scan_sched_scan_stop(struct wl1271 *wl, struct wl12xx_vif *wlvif)
{
	struct wl12xx_cmd_scan_stop *stop;
	int ret = 0;

	wl1271_debug(DEBUG_CMD, "cmd periodic scan stop");

	/* FIXME: what to do if alloc'ing to stop fails? */
	stop = kzalloc(sizeof(*stop), GFP_KERNEL);
	if (!stop) {
		wl1271_error("failed to alloc memory to send sched scan stop");
		return;
	}

	stop->role_id = wlvif->role_id;
	stop->scan_type = SCAN_TYPE_PERIODIC;

	ret = wl1271_cmd_send(wl, CMD_STOP_SCAN, stop, sizeof(*stop), 0);
	if (ret < 0) {
		wl1271_error("failed to send sched scan stop command");
		goto out_free;
	}

out_free:
	kfree(stop);
}

