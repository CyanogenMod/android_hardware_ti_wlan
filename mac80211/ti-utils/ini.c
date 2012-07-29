/*
 * PLT utility for wireless chip supported by TI's driver wl12xx
 *
 * See README and COPYING for more details.
 */

#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/wireless.h>
#include "nl80211.h"

#include "calibrator.h"
#include "plt.h"
#include "ini.h"
#include "nvs.h"

static char *ini_get_line(char *s, int size, FILE *stream, int *line,
				  char **_pos)
{
	char *pos, *end, *sstart;

	while (fgets(s, size, stream)) {
		s[size - 1] = '\0';
		pos = s;

		/* Skip white space from the beginning of line. */
		while (*pos == ' ' || *pos == '\t' || *pos == '\r')
			pos++;

		/* Skip comment lines and empty lines */
		if (*pos == '#' || *pos == '\n' || *pos == '\0')
			continue;

		/*
		 * Remove # comments unless they are within a double quoted
		 * string.
		 */
		sstart = strchr(pos, '"');
		if (sstart)
			sstart = strrchr(sstart + 1, '"');
		if (!sstart)
			sstart = pos;
		end = strchr(sstart, '#');
		if (end)
			*end-- = '\0';
		else
			end = pos + strlen(pos) - 1;

		/* Remove trailing white space. */
		while (end > pos &&
		       (*end == '\n' || *end == ' ' || *end == '\t' ||
			*end == '\r'))
			*end-- = '\0';

		if (*pos == '\0')
			continue;

		(*line)++;

		if (_pos)
			*_pos = pos;
		return pos;
	}

	if (_pos)
		*_pos = NULL;

	return NULL;
}

static int split_line(char *line, char **name, char **value)
{
	char *pos = line;

	*value = strchr(pos, '=');
	if (!*value) {
		fprintf(stderr, "Wrong format of line\n");
		return 1;
	}

	*name = *value;

	(*name)--;
	while (**name == ' ' || **name == '\t' || **name == '\r')
		(*name)--;

	*++(*name) = '\0';

	(*value)++;
	while (**value == ' ' || **value == '\t' || **value == '\r')
		(*value)++;

	return 0;
}

/* Parse array of unsigned chars */
static int parse_uc_a(char* name, char *val, unsigned char *out, size_t exp_size)
{
	size_t i = 0;
	long v;
	char *endval;

	while (*val) {
		/* Advance to next token */
		while (*val == ' ' || *val == ',')
			val++;

		if (i >= exp_size) {
			fprintf(stderr, "Too many params for %s\n", name);
			return 1;
		}
		v = strtol(val, &endval, 16);

		if (endval == val) {
			fprintf(stderr, "Syntax error parsing %s\n", name);
			return 1;
		}
		if (v > (long) UCHAR_MAX) {
			fprintf(stderr, "Overflow parsing %s\n", name);
			return 1;
		}
		out[i++] = v;
		val = endval;
	}

	if(exp_size != i) {
		fprintf(stderr, "Too few parameters for %s\n", name);
		return 1;
	}

	return 0;
}

/* Parse array of __le16 */
static int parse_ui_a(char* name, char *val, __le16 *out, size_t exp_size)
{
	size_t i = 0;
	long v;
	char *endval;

	while (*val) {
		/* Advance to next token */
		while (*val == ' ' || *val == ',')
			val++;

		if (i >= exp_size) {
			fprintf(stderr, "Too many params for %s\n", name);
			return 1;
		}
		v = strtol(val, &endval, 16);

		if (endval == val) {
			fprintf(stderr, "Syntax error parsing %s\n", name);
			return 1;
		}
		if (v > (long) INT16_MAX) {
			fprintf(stderr, "Overflow parsing %s\n", name);
			return 1;
		}
		out[i++] = v;

		val = endval;
	}

	if(exp_size != i) {
		fprintf(stderr, "Too few parameters for %s\n", name);
		return 1;
	}

	return 0;
}

#define COMPARE_N_ADD(temp, str, val, ptr)			\
	if (strncmp(temp, str, sizeof(temp)) == 0) {		\
		return parse_uc_a(temp, val, (unsigned char*) ptr, sizeof(*ptr)); \
	}

#define COMPARE_N_ADD2(temp, str, val, ptr)		\
	if (strncmp(temp, str, sizeof(temp)) == 0) {		\
		return parse_ui_a(temp, val, (__le16*) ptr, sizeof(*ptr)/2); \
	}

static int parse_general_prms(char *l, struct wl12xx_common *cmn,
	struct wl12xx_ini *p)
{
	char *name, *val;
	struct wl1271_ini_general_params *gp = &(p->ini1271.general_params);

	if (split_line(l, &name, &val))
		return 1;

	COMPARE_N_ADD("TXBiPFEMAutoDetect", l, val,
		&gp->tx_bip_fem_auto_detect);

	cmn->auto_fem = gp->tx_bip_fem_auto_detect;

	COMPARE_N_ADD("TXBiPFEMManufacturer", l, val,
		&gp->tx_bip_fem_manufacturer);

	COMPARE_N_ADD("RefClk", l, val, &gp->ref_clock);

	COMPARE_N_ADD("SettlingTime", l, val, &gp->settling_time);

	COMPARE_N_ADD("ClockValidOnWakeup", l, val,
		&gp->clk_valid_on_wakeup);

	COMPARE_N_ADD("DC2DCMode", l, val, &gp->dc2dc_mode);

	COMPARE_N_ADD("Single_Dual_Band_Solution", l, val,
		&gp->dual_mode_select);

	COMPARE_N_ADD("Settings", l, val, &gp->general_settings);

	COMPARE_N_ADD("SRState", l, val, &gp->sr_state);

	COMPARE_N_ADD("SRF1", l, val, &gp->srf1);

	COMPARE_N_ADD("SRF2", l, val, &gp->srf2);

	COMPARE_N_ADD("SRF3", l, val, &gp->srf3);

	fprintf(stderr, "Unable to parse: (%s)\n", l);

	return 1;
}

static int parse_general_prms_128x(char *l, struct wl12xx_common *cmn,
	struct wl12xx_ini *p)
{
	char *name, *val;
	struct wl128x_ini_general_params *gp =
		&(p->ini128x.general_params);

	if (split_line(l, &name, &val))
		return 1;


	COMPARE_N_ADD("TXBiPFEMAutoDetect", l, val,
		&gp->tx_bip_fem_auto_detect);

	COMPARE_N_ADD("TXBiPFEMManufacturer", l, val,
		&gp->tx_bip_fem_manufacturer);

	cmn->auto_fem = gp->tx_bip_fem_auto_detect;

	COMPARE_N_ADD("RefClk", l, val, &gp->ref_clock);

	COMPARE_N_ADD("SettlingTime", l, val, &gp->settling_time);

	COMPARE_N_ADD("ClockValidOnWakeup", l, val, &gp->clk_valid_on_wakeup);

	COMPARE_N_ADD("TCXO_Clk", l, val, &gp->tcxo_ref_clock);

	COMPARE_N_ADD("TCXO_SettlingTime", l, val, &gp->tcxo_settling_time);

	COMPARE_N_ADD("TCXO_ClockValidOnWakeup", l, val,
		&gp->tcxo_valid_on_wakeup);

	COMPARE_N_ADD("TCXO_LDO_Voltage", l, val, &gp->tcxo_ldo_voltage);

	COMPARE_N_ADD("Platform_configuration", l, val, &gp->platform_conf);

	COMPARE_N_ADD("Single_Dual_Band_Solution", l, val,
		&gp->dual_mode_select);

	COMPARE_N_ADD("Settings", l, val, &gp->general_settings);

	COMPARE_N_ADD("XTALItrimVal", l, val, &gp->xtal_itrim_val);

	COMPARE_N_ADD("SRState", l, val, &gp->sr_state);

	COMPARE_N_ADD("SRF1", l, val, &gp->srf1);

	COMPARE_N_ADD("SRF2", l, val, &gp->srf2);

	COMPARE_N_ADD("SRF3", l, val, &gp->srf3);

	fprintf(stderr, "Unable to parse: (%s)\n", l);

	return 1;
}

static int parse_band2_prms(char *l, struct wl12xx_ini *p)
{
	char *name, *val;
	struct wl1271_ini_band_params_2 *gp =
		&(p->ini1271.stat_radio_params_2);

	if (split_line(l, &name, &val))
		return 1;

	COMPARE_N_ADD("RxTraceInsertionLoss_2_4G", l, val,
		&gp->rx_trace_insertion_loss);

	COMPARE_N_ADD("TXTraceLoss_2_4G", l, val,
		&gp->tx_trace_loss);

	COMPARE_N_ADD("RxRssiAndProcessCompensation_2_4G", l, val,
		&gp->rx_rssi_process_compens);

	fprintf(stderr, "Unable to parse: (%s)\n", l);

	return 1;
}

static int parse_band2_prms_128x(char *l, struct wl12xx_ini *p)
{
	char *name, *val;
	struct wl128x_ini_band_params_2 *gp = &(p->ini128x.stat_radio_params_2);

	if (split_line(l, &name, &val))
		return 1;

	COMPARE_N_ADD("RxTraceInsertionLoss_2_4G", l, val,
		&gp->rx_trace_insertion_loss);

	COMPARE_N_ADD("TxTraceLoss_2_4G", l, val, &gp->tx_trace_loss);

	fprintf(stderr, "Unable to parse: (%s)\n", l);

	return 1;
}

static int parse_band5_prms(char *l, struct wl12xx_ini *p)
{
	char *name, *val;
	struct wl1271_ini_band_params_5 *gp =
		&(p->ini1271.stat_radio_params_5);

	if (split_line(l, &name, &val))
		return 1;

	COMPARE_N_ADD("RxTraceInsertionLoss_5G", l, val,
		&gp->rx_trace_insertion_loss);

	COMPARE_N_ADD("TXTraceLoss_5G", l, val,
		&gp->tx_trace_loss);

	COMPARE_N_ADD("RxRssiAndProcessCompensation_5G", l, val,
		&gp->rx_rssi_process_compens);

	fprintf(stderr, "Unable to parse: (%s)\n", l);

	return 1;
}

static int parse_band5_prms_128x(char *l, struct wl12xx_ini *p)
{
	char *name, *val;
	struct wl128x_ini_band_params_5 *gp = &(p->ini128x.stat_radio_params_5);

	if (split_line(l, &name, &val))
		return 1;

	COMPARE_N_ADD("RxTraceInsertionLoss_5G", l, val,
		&gp->rx_trace_insertion_loss);

	COMPARE_N_ADD("TxTraceLoss_5G", l, val,
		&gp->tx_trace_loss);

	fprintf(stderr, "Unable to parse: (%s)\n", l);

	return 1;
}

static int parse_fem0_band2_prms(char *l, struct wl12xx_ini *p)
{
	char *name, *val;
	struct wl1271_ini_fem_params_2 *gp =
		&(p->ini1271.dyn_radio_params_2[0].params);

	if (split_line(l, &name, &val))
		return 1;

	COMPARE_N_ADD2("FEM0_TXBiPReferencePDvoltage_2_4G", l, val,
		&gp->tx_bip_ref_pd_voltage);

	COMPARE_N_ADD("FEM0_TxBiPReferencePower_2_4G", l, val,
		&gp->tx_bip_ref_power);

	COMPARE_N_ADD("FEM0_TxBiPOffsetdB_2_4G", l, val,
		&gp->tx_bip_ref_offset);

	COMPARE_N_ADD("FEM0_TxPerRatePowerLimits_2_4G_Normal", l, val,
		&gp->tx_per_rate_pwr_limits_normal);

	COMPARE_N_ADD("FEM0_TxPerRatePowerLimits_2_4G_Degraded", l, val,
		&gp->tx_per_rate_pwr_limits_degraded);

	COMPARE_N_ADD("FEM0_TxPerRatePowerLimits_2_4G_Extreme", l, val,
		&gp->tx_per_rate_pwr_limits_extreme);

	COMPARE_N_ADD("FEM0_DegradedLowToNormalThr_2_4G", l, val,
		&gp->degraded_low_to_normal_thr);

	COMPARE_N_ADD("FEM0_NormalToDegradedHighThr_2_4G", l, val,
		&gp->normal_to_degraded_high_thr);

	COMPARE_N_ADD("FEM0_TxPerChannelPowerLimits_2_4G_11b", l, val,
		&gp->tx_per_chan_pwr_limits_11b);

	COMPARE_N_ADD("FEM0_TxPerChannelPowerLimits_2_4G_OFDM", l, val,
		&gp->tx_per_chan_pwr_limits_ofdm);

	COMPARE_N_ADD("FEM0_TxPDVsRateOffsets_2_4G", l, val,
		&gp->tx_pd_vs_rate_offsets);

	COMPARE_N_ADD("FEM0_TxIbiasTable_2_4G", l, val,
		&gp->tx_ibias);

	COMPARE_N_ADD("FEM0_RxFemInsertionLoss_2_4G", l, val,
		&gp->rx_fem_insertion_loss);

	fprintf(stderr, "Unable to parse: (%s)\n", l);

	return 1;
}

static int parse_fem0_band2_prms_128x(char *l, struct wl12xx_ini *p)
{
	char *name, *val;
	struct wl128x_ini_fem_params_2 *gp =
		&(p->ini128x.dyn_radio_params_2[0].params);

	if (split_line(l, &name, &val))
		return 1;

	COMPARE_N_ADD2("FEM0_TxBiPReferencePDvoltage_2_4G", l, val,
		&gp->tx_bip_ref_pd_voltage);

	COMPARE_N_ADD("FEM0_TxBiPReferencePower_2_4G", l, val,
		&gp->tx_bip_ref_power);

	COMPARE_N_ADD("FEM0_TxBiPOffsetdB_2_4G", l, val,
		&gp->tx_bip_ref_offset);

	COMPARE_N_ADD("FEM0_TxPerRatePowerLimits_2_4G_Normal", l, val,
		&gp->tx_per_rate_pwr_limits_normal);

	COMPARE_N_ADD("FEM0_TxPerRatePowerLimits_2_4G_Degraded", l, val,
		&gp->tx_per_rate_pwr_limits_degraded);

	COMPARE_N_ADD("FEM0_TxPerRatePowerLimits_2_4G_Extreme", l, val,
		&gp->tx_per_rate_pwr_limits_extreme);

	COMPARE_N_ADD("FEM0_DegradedLowToNormalThr_2_4G", l, val,
		&gp->degraded_low_to_normal_thr);

	COMPARE_N_ADD("FEM0_NormalToDegradedHighThr_2_4G", l, val,
		&gp->normal_to_degraded_high_thr);

	COMPARE_N_ADD("FEM0_TxPerChannelPowerLimits_2_4G_11b", l, val,
		&gp->tx_per_chan_pwr_limits_11b);

	COMPARE_N_ADD("FEM0_TxPerChannelPowerLimits_2_4G_OFDM", l, val,
		&gp->tx_per_chan_pwr_limits_ofdm);

	COMPARE_N_ADD("FEM0_TxPDVsRateOffsets_2_4G", l, val,
		&gp->tx_pd_vs_rate_offsets);

	COMPARE_N_ADD("FEM0_TxPDVsChannelOffsets_2_4G", l, val,
		&gp->tx_pd_vs_chan_offsets);

	COMPARE_N_ADD("FEM0_TxPDVsTemperature_2_4G", l, val,
		&gp->tx_pd_vs_temperature);

	COMPARE_N_ADD("FEM0_TxIbiasTable_2_4G", l, val,
		&gp->tx_ibias);

	COMPARE_N_ADD("FEM0_RxFemInsertionLoss_2_4G", l, val,
		&gp->rx_fem_insertion_loss);

	fprintf(stderr, "Unable to parse: (%s)\n", l);

	return 1;
}

static int parse_fem1_band2_prms(char *l, struct wl12xx_ini *p)
{
	char *name, *val;
	struct wl1271_ini_fem_params_2 *gp =
		&(p->ini1271.dyn_radio_params_2[1].params);

	if (split_line(l, &name, &val))
		return 1;

	COMPARE_N_ADD2("FEM1_TXBiPReferencePDvoltage_2_4G", l, val,
		&gp->tx_bip_ref_pd_voltage);

	COMPARE_N_ADD("FEM1_TxBiPReferencePower_2_4G", l, val,
		&gp->tx_bip_ref_power);

	COMPARE_N_ADD("FEM1_TxBiPOffsetdB_2_4G", l, val,
		&gp->tx_bip_ref_offset);

	COMPARE_N_ADD("FEM1_TxPerRatePowerLimits_2_4G_Normal", l, val,
		&gp->tx_per_rate_pwr_limits_normal);

	COMPARE_N_ADD("FEM1_TxPerRatePowerLimits_2_4G_Degraded", l, val,
		&gp->tx_per_rate_pwr_limits_degraded);

	COMPARE_N_ADD("FEM1_TxPerRatePowerLimits_2_4G_Extreme", l, val,
		&gp->tx_per_rate_pwr_limits_extreme);

	COMPARE_N_ADD("FEM1_DegradedLowToNormalThr_2_4G", l, val,
		&gp->degraded_low_to_normal_thr);

	COMPARE_N_ADD("FEM1_NormalToDegradedHighThr_2_4G", l, val,
		&gp->normal_to_degraded_high_thr);

	COMPARE_N_ADD("FEM1_TxPerChannelPowerLimits_2_4G_11b", l, val,
		&gp->tx_per_chan_pwr_limits_11b);

	COMPARE_N_ADD("FEM1_TxPerChannelPowerLimits_2_4G_OFDM", l, val,
		&gp->tx_per_chan_pwr_limits_ofdm);

	COMPARE_N_ADD("FEM1_TxPDVsRateOffsets_2_4G", l, val,
		&gp->tx_pd_vs_rate_offsets);

	COMPARE_N_ADD("FEM1_TxIbiasTable_2_4G", l, val,
		&gp->tx_ibias);

	COMPARE_N_ADD("FEM1_RxFemInsertionLoss_2_4G", l, val,
		&gp->rx_fem_insertion_loss);

	fprintf(stderr, "Unable to parse: (%s)\n", l);

	return 1;
}

static int parse_fem1_band2_prms_128x(char *l, struct wl12xx_ini *p)
{
	char *name, *val;
	struct wl128x_ini_fem_params_2 *gp =
		&(p->ini128x.dyn_radio_params_2[1].params);

	if (split_line(l, &name, &val))
		return 1;

	COMPARE_N_ADD2("FEM1_TxBiPReferencePDvoltage_2_4G", l, val,
		&gp->tx_bip_ref_pd_voltage);

	COMPARE_N_ADD("FEM1_TxBiPReferencePower_2_4G", l, val,
		&gp->tx_bip_ref_power);

	COMPARE_N_ADD("FEM1_TxBiPOffsetdB_2_4G", l, val,
		&gp->tx_bip_ref_offset);

	COMPARE_N_ADD("FEM1_TxPerRatePowerLimits_2_4G_Normal", l, val,
		&gp->tx_per_rate_pwr_limits_normal);

	COMPARE_N_ADD("FEM1_TxPerRatePowerLimits_2_4G_Degraded", l, val,
		&gp->tx_per_rate_pwr_limits_degraded);

	COMPARE_N_ADD("FEM1_TxPerRatePowerLimits_2_4G_Extreme", l, val,
		&gp->tx_per_rate_pwr_limits_extreme);

	COMPARE_N_ADD("FEM1_DegradedLowToNormalThr_2_4G", l, val,
		&gp->degraded_low_to_normal_thr);

	COMPARE_N_ADD("FEM1_NormalToDegradedHighThr_2_4G", l, val,
		&gp->normal_to_degraded_high_thr);

	COMPARE_N_ADD("FEM1_TxPerChannelPowerLimits_2_4G_11b", l, val,
		&gp->tx_per_chan_pwr_limits_11b);

	COMPARE_N_ADD("FEM1_TxPerChannelPowerLimits_2_4G_OFDM", l, val,
		&gp->tx_per_chan_pwr_limits_ofdm);

	COMPARE_N_ADD("FEM1_TxPDVsRateOffsets_2_4G", l, val,
		&gp->tx_pd_vs_rate_offsets);

	COMPARE_N_ADD("FEM1_TxPDVsChannelOffsets_2_4G", l, val,
		&gp->tx_pd_vs_chan_offsets);

	COMPARE_N_ADD("FEM1_TxPDVsTemperature_2_4G", l, val,
		&gp->tx_pd_vs_temperature);

	COMPARE_N_ADD("FEM1_TxIbiasTable_2_4G", l, val,
		&gp->tx_ibias);

	COMPARE_N_ADD("FEM1_RxFemInsertionLoss_2_4G", l, val,
		&gp->rx_fem_insertion_loss);

	fprintf(stderr, "Unable to parse: (%s)\n", l);

	return 1;
}

static int parse_fem0_band5_prms(char *l, struct wl12xx_ini *p)
{
	char *name, *val;
	struct wl1271_ini_fem_params_5 *gp =
		&(p->ini1271.dyn_radio_params_5[0].params);

	if (split_line(l, &name, &val))
		return 1;

	COMPARE_N_ADD2("FEM0_TXBiPReferencePDvoltage_5G", l, val,
		&gp->tx_bip_ref_pd_voltage);

	COMPARE_N_ADD("FEM0_TxBiPReferencePower_5G", l, val,
		&gp->tx_bip_ref_power);

	COMPARE_N_ADD("FEM0_TxBiPOffsetdB_5G", l, val,
		&gp->tx_bip_ref_offset);

	COMPARE_N_ADD("FEM0_TxPerRatePowerLimits_5G_Normal", l, val,
		&gp->tx_per_rate_pwr_limits_normal);

	COMPARE_N_ADD("FEM0_TxPerRatePowerLimits_5G_Degraded", l, val,
		&gp->tx_per_rate_pwr_limits_degraded);

	COMPARE_N_ADD("FEM0_TxPerRatePowerLimits_5G_Extreme", l, val,
		&gp->tx_per_rate_pwr_limits_extreme);

	COMPARE_N_ADD("FEM0_DegradedLowToNormalThr_5G", l, val,
		&gp->degraded_low_to_normal_thr);

	COMPARE_N_ADD("FEM0_NormalToDegradedHighThr_5G", l, val,
		&gp->normal_to_degraded_high_thr);

	COMPARE_N_ADD("FEM0_TxPerChannelPowerLimits_5G_OFDM", l, val,
		&gp->tx_per_chan_pwr_limits_ofdm);

	COMPARE_N_ADD("FEM0_TxPDVsRateOffsets_5G", l, val,
		&gp->tx_pd_vs_rate_offsets);

	COMPARE_N_ADD("FEM0_TxIbiasTable_5G", l, val,
		&gp->tx_ibias);

	COMPARE_N_ADD("FEM0_RxFemInsertionLoss_5G", l, val,
		&gp->rx_fem_insertion_loss);

	fprintf(stderr, "Unable to parse: (%s)\n", l);

	return 1;
}

static int parse_fem1_band5_prms(char *l, struct wl12xx_ini *p)
{
	char *name, *val;
	struct wl1271_ini_fem_params_5 *gp =
		&(p->ini1271.dyn_radio_params_5[1].params);

	if (split_line(l, &name, &val))
		return 1;

	COMPARE_N_ADD2("FEM1_TXBiPReferencePDvoltage_5G", l, val,
		&gp->tx_bip_ref_pd_voltage);

	COMPARE_N_ADD("FEM1_TxBiPReferencePower_5G", l, val,
		&gp->tx_bip_ref_power);

	COMPARE_N_ADD("FEM1_TxBiPOffsetdB_5G", l, val,
		&gp->tx_bip_ref_offset);

	COMPARE_N_ADD("FEM1_TxPerRatePowerLimits_5G_Normal", l, val,
		&gp->tx_per_rate_pwr_limits_normal);

	COMPARE_N_ADD("FEM1_TxPerRatePowerLimits_5G_Degraded", l, val,
		&gp->tx_per_rate_pwr_limits_degraded);

	COMPARE_N_ADD("FEM1_TxPerRatePowerLimits_5G_Extreme", l, val,
		&gp->tx_per_rate_pwr_limits_extreme);

	COMPARE_N_ADD("FEM1_DegradedLowToNormalThr_5G", l, val,
		&gp->degraded_low_to_normal_thr);

	COMPARE_N_ADD("FEM1_NormalToDegradedHighThr_5G", l, val,
		&gp->normal_to_degraded_high_thr);

	COMPARE_N_ADD("FEM1_TxPerChannelPowerLimits_5G_OFDM", l, val,
		&gp->tx_per_chan_pwr_limits_ofdm);

	COMPARE_N_ADD("FEM1_TxPDVsRateOffsets_5G", l, val,
		&gp->tx_pd_vs_rate_offsets);

	COMPARE_N_ADD("FEM1_TxIbiasTable_5G", l, val,
		&gp->tx_ibias);

	COMPARE_N_ADD("FEM1_RxFemInsertionLoss_5G", l, val,
		&gp->rx_fem_insertion_loss);

	fprintf(stderr, "Unable to parse: (%s)\n", l);

	return 1;
}

static int parse_fem1_band5_prms_128x(char *l, struct wl12xx_ini *p)
{
	char *name, *val;
	struct wl128x_ini_fem_params_5 *gp =
		&(p->ini128x.dyn_radio_params_5[1].params);

	if (split_line(l, &name, &val))
		return 1;

	COMPARE_N_ADD2("FEM1_TxBiPReferencePDvoltage_5G", l, val,
		&gp->tx_bip_ref_pd_voltage);

	COMPARE_N_ADD("FEM1_TxBiPReferencePower_5G", l, val,
		&gp->tx_bip_ref_power);

	COMPARE_N_ADD("FEM1_TxBiPOffsetdB_5G", l, val,
		&gp->tx_bip_ref_offset);

	COMPARE_N_ADD("FEM1_TxPerRatePowerLimits_5G_Normal", l, val,
		&gp->tx_per_rate_pwr_limits_normal);

	COMPARE_N_ADD("FEM1_TxPerRatePowerLimits_5G_Degraded", l, val,
		&gp->tx_per_rate_pwr_limits_degraded);

	COMPARE_N_ADD("FEM1_TxPerRatePowerLimits_5G_Extreme", l, val,
		&gp->tx_per_rate_pwr_limits_extreme);

	COMPARE_N_ADD("FEM1_DegradedLowToNormalThr_5G", l, val,
		&gp->degraded_low_to_normal_thr);

	COMPARE_N_ADD("FEM1_NormalToDegradedHighThr_5G", l, val,
		&gp->normal_to_degraded_high_thr);

	COMPARE_N_ADD("FEM1_TxPerChannelPowerLimits_5G_OFDM", l, val,
		&gp->tx_per_chan_pwr_limits_ofdm);

	COMPARE_N_ADD("FEM1_TxPDVsRateOffsets_5G", l, val,
		&gp->tx_pd_vs_rate_offsets);

	COMPARE_N_ADD("FEM1_TxPDVsChannelOffsets_5G", l, val,
		&gp->tx_pd_vs_chan_offsets);

	COMPARE_N_ADD("FEM1_TxPDVsTemperature_5G", l, val,
		&gp->tx_pd_vs_temperature);

	COMPARE_N_ADD("FEM1_TxIbiasTable_5G", l, val,
		&gp->tx_ibias);

	COMPARE_N_ADD("FEM1_RxFemInsertionLoss_5G", l, val,
		&gp->rx_fem_insertion_loss);

	fprintf(stderr, "Unable to parse: (%s)\n", l);

	return 1;
}

static int parse_fem0_band5_prms_128x(char *l, struct wl12xx_ini *p)
{
	char *name, *val;
	struct wl128x_ini_fem_params_5 *gp =
		&(p->ini128x.dyn_radio_params_5[0].params);

	if (split_line(l, &name, &val))
		return 1;

	COMPARE_N_ADD2("FEM0_TxBiPReferencePDvoltage_5G", l, val,
		&gp->tx_bip_ref_pd_voltage);

	COMPARE_N_ADD("FEM0_TxBiPReferencePower_5G", l, val,
		&gp->tx_bip_ref_power);

	COMPARE_N_ADD("FEM0_TxBiPOffsetdB_5G", l, val,
		&gp->tx_bip_ref_offset);

	COMPARE_N_ADD("FEM0_TxPerRatePowerLimits_5G_Normal", l, val,
		&gp->tx_per_rate_pwr_limits_normal);

	COMPARE_N_ADD("FEM0_TxPerRatePowerLimits_5G_Degraded", l, val,
		&gp->tx_per_rate_pwr_limits_degraded);

	COMPARE_N_ADD("FEM0_TxPerRatePowerLimits_5G_Extreme", l, val,
		&gp->tx_per_rate_pwr_limits_extreme);

	COMPARE_N_ADD("FEM0_DegradedLowToNormalThr_5G", l, val,
		&gp->degraded_low_to_normal_thr);

	COMPARE_N_ADD("FEM0_NormalToDegradedHighThr_5G", l, val,
		&gp->normal_to_degraded_high_thr);

	COMPARE_N_ADD("FEM0_TxPerChannelPowerLimits_5G_OFDM", l, val,
		&gp->tx_per_chan_pwr_limits_ofdm);

	COMPARE_N_ADD("FEM0_TxPDVsRateOffsets_5G", l, val,
		&gp->tx_pd_vs_rate_offsets);

	COMPARE_N_ADD("FEM0_TxPDVsChannelOffsets_5G", l, val,
		&gp->tx_pd_vs_chan_offsets);

	COMPARE_N_ADD("FEM0_TxPDVsTemperature_5G", l, val,
		&gp->tx_pd_vs_temperature);

	COMPARE_N_ADD("FEM0_TxIbiasTable_5G", l, val,
		&gp->tx_ibias);

	COMPARE_N_ADD("FEM0_RxFemInsertionLoss_5G", l, val,
		&gp->rx_fem_insertion_loss);

	fprintf(stderr, "Unable to parse: (%s)\n", l);

	return 1;
}

static int parse_fem_prms_128x(char *l, struct wl12xx_ini *p)
{
	char *name, *val;
	struct wl128x_ini *gp = &p->ini128x;

	if (split_line(l, &name, &val))
		return 1;

	COMPARE_N_ADD("FemVendorAndOptions", l, val,
		&gp->fem_vendor_and_options);

	fprintf(stderr, "Unable to parse: (%s)\n", l);

	return 1;
}

static int find_section(const char *l, enum wl1271_ini_section *st, int *cntr,
	struct wl12xx_common *cmn)
{
	enum wl12xx_arch arch = cmn->arch;
	if (strncmp("TXBiPFEMAutoDetect", l, 18) == 0) {
		*st = GENERAL_PRMS;
		if (arch == WL128X_ARCH)
			*cntr = 17;
		else
			*cntr = 12;

		return 0;
	}

	if (strncmp("RxTraceInsertionLoss_2_4G", l, 25) == 0) {
		*st = BAND2_PRMS;
		if (arch == WL128X_ARCH)
			*cntr = 2;
		else
			*cntr = 3;

		return 0;
	}

	if (strncmp("FemVendorAndOptions", l, 19) == 0) {
		*st = FEM_PRMS;
		*cntr = 1;
		return 0;
	}

	if (strncmp("RxTraceInsertionLoss_5G", l, 23) == 0) {
		*st = BAND5_PRMS;
		if (arch == WL128X_ARCH)
			*cntr = 2;
		else
			*cntr = 3;

		return 0;
	}

	if (strncmp("FEM0_TXBiPReferencePDvoltage_2_4G", l, 33) == 0 ||
		strncmp("FEM0_TxBiPReferencePDvoltage_2_4G", l, 33) == 0) {
		*st = FEM0_BAND2_PRMS;
		cmn->fem0_bands++;
		if (arch == WL128X_ARCH)
			*cntr = 15;
		else
			*cntr = 13;

		return 0;
	}

	if (strncmp("FEM1_TXBiPReferencePDvoltage_2_4G", l, 33) == 0 ||
		strncmp("FEM1_TxBiPReferencePDvoltage_2_4G", l, 33) == 0) {
		*st = FEM1_BAND2_PRMS;
		cmn->fem1_bands++;
		if (arch == WL128X_ARCH)
			*cntr = 15;
		else
			*cntr = 13;

		return 0;
	}

	if (strncmp("FEM1_TXBiPReferencePDvoltage_5G", l, 31) == 0 ||
		strncmp("FEM1_TxBiPReferencePDvoltage_5G", l, 31) == 0) {
		*st = FEM1_BAND5_PRMS;
		cmn->fem1_bands++;
		if (arch == WL128X_ARCH)
			*cntr = 14;
		else
			*cntr = 12;

		return 0;
	}

	if (strncmp("FEM0_TXBiPReferencePDvoltage_5G", l, 31) == 0 ||
		strncmp("FEM0_TxBiPReferencePDvoltage_5G", l, 31) == 0) {
		*st = FEM0_BAND5_PRMS;
		cmn->fem0_bands++;
		if (arch == WL128X_ARCH)
			*cntr = 14;
		else
			*cntr = 12;

		return 0;
	}

	return 1;
}

static int ini_parse_line(char *l, struct wl12xx_common *cmn)
{
	static enum wl1271_ini_section status;
	static int cntr;

	if (!cntr && find_section(l, &status, &cntr, cmn)) {
		fprintf(stderr, "Uknown ini section %s\n", l);
		return 1;
	}

	switch (status) {
	case GENERAL_PRMS:	/* general parameters */
		cntr--;
		return cmn->parse_ops->prs_general_prms(l, cmn, &cmn->ini);
	case FEM_PRMS:	/* FEM parameters */
		if (cmn->arch == WL1271_ARCH) {
			fprintf(stderr, "The parameter not from 127x architecture\n");
			return 1;
		}
		cntr--;
		return parse_fem_prms_128x(l, &cmn->ini);
	case BAND2_PRMS:	/* band 2.4GHz parameters */
		cntr--;
		return cmn->parse_ops->prs_band2_prms(l, &cmn->ini);
	case BAND5_PRMS:	/* band 5GHz parameters */
		cntr--;
		return cmn->parse_ops->prs_band5_prms(l, &cmn->ini);
	case FEM0_BAND2_PRMS:	/* FEM0 band 2.4GHz parameters */
		cntr--;
		return cmn->parse_ops->prs_fem0_band2_prms(l, &cmn->ini);
	case FEM1_BAND2_PRMS:	/* FEM1 band 2.4GHz parameters */
		cntr--;
		return cmn->parse_ops->prs_fem1_band2_prms(l, &cmn->ini);
	case FEM0_BAND5_PRMS:	/* FEM0 band 5GHz parameters */
		cntr--;
		return cmn->parse_ops->prs_fem0_band5_prms(l, &cmn->ini);
	case FEM1_BAND5_PRMS:	/* FEM1 band 5GHz parameters */
		cntr--;
		return cmn->parse_ops->prs_fem1_band5_prms(l, &cmn->ini);
	case UKNOWN_SECTION:
		/* added because of compilation warning. handeled in find_section() */
		break;
	}

	return 1;
}

#if 0
static void ini_dump(struct wl1271_ini *ini)
{
	int i;

	printf("\n");
	printf("General params:\n");
	printf("ref clock:                 %02X\n",
		ini->general_params.ref_clock);
	printf("settling time:             %02X\n",
		ini->general_params.settling_time);
	printf("clk valid on wakeup:       %02X\n",
		ini->general_params.clk_valid_on_wakeup);
	printf("dc2dc mode:                %02X\n",
		ini->general_params.dc2dc_mode);
	printf("dual band mode:            %02X\n",
		ini->general_params.dual_mode_select);
	printf("tx bip fem auto detect:    %02X\n",
		ini->general_params.tx_bip_fem_auto_detect);
	printf("tx bip fem manufacturer:   %02X\n",
		ini->general_params.tx_bip_fem_manufacturer);
	printf("general settings:          %02X\n",
		ini->general_params.general_settings);
	printf("sr state:                  %02X\n",
		ini->general_params.sr_state);

	printf("srf1:");
	for (i = 0; i < WL1271_INI_MAX_SMART_REFLEX_PARAM; i++)
		printf(" %02X", ini->general_params.srf1[i]);
	printf("\n");

	printf("srf2:");
	for (i = 0; i < WL1271_INI_MAX_SMART_REFLEX_PARAM; i++)
		printf(" %02X", ini->general_params.srf2[i]);
	printf("\n");

	printf("srf3:");
	for (i = 0; i < WL1271_INI_MAX_SMART_REFLEX_PARAM; i++)
		printf(" %02X", ini->general_params.srf3[i]);
	printf("\n");

	printf("Static 2.4 band params:\n");

	printf("rx trace insertion loss: %02X\n",
		ini->stat_radio_params_2.rx_trace_insertion_loss);

	printf("rx rssi n process compensation:");
	for (i = 0; i < WL1271_INI_RSSI_PROCESS_COMPENS_SIZE; i++)
		printf(" %02X",
			ini->stat_radio_params_2.rx_rssi_process_compens[i]);
	printf("\n");

	printf("tx trace: %02X\n",
		ini->stat_radio_params_2.tx_trace_loss);

	printf("Dynamic 2.4 band params for FEM\n");

	printf("Static 5 band params:\n");

	printf("rx trace insertion loss:");
	for (i = 0; i < WL1271_INI_SUB_BAND_COUNT_5; i++)
		printf(" %02X",
			ini->stat_radio_params_5.rx_rssi_process_compens[i]);
	printf("\n");

	printf("rx rssi n process compensation:");
	for (i = 0; i < WL1271_INI_RSSI_PROCESS_COMPENS_SIZE; i++)
		printf(" %02X",
			ini->stat_radio_params_5.rx_rssi_process_compens[i]);
	printf("\n");

	printf("tx trace:");
	for (i = 0; i < WL1271_INI_SUB_BAND_COUNT_5; i++)
		printf(" %02X",
			ini->stat_radio_params_5.tx_trace_loss[i]);
	printf("\n");

	printf("Dynamic 5 band params for FEM\n");

}
#endif


static int is_dual_mode(struct wl12xx_ini *p)
{
	struct wl1271_ini_general_params *gp = &(p->ini1271.general_params);
	return gp->dual_mode_select;
}

static int is_dual_mode_128x(struct wl12xx_ini *p)
{
	struct wl128x_ini_general_params *gp = &(p->ini128x.general_params);
	return gp->dual_mode_select;
}


static struct wl12xx_parse_ops wl1271_parse_ops = {
	.prs_general_prms       = parse_general_prms,
	.prs_band2_prms         = parse_band2_prms,
	.prs_band5_prms         = parse_band5_prms,
	.prs_fem0_band2_prms    = parse_fem0_band2_prms,
	.prs_fem1_band2_prms    = parse_fem1_band2_prms,
	.prs_fem0_band5_prms    = parse_fem0_band5_prms,
	.prs_fem1_band5_prms    = parse_fem1_band5_prms,
	.is_dual_mode		= is_dual_mode,
};

static struct wl12xx_parse_ops wl128x_parse_ops = {
	.prs_general_prms       = parse_general_prms_128x,
	.prs_band2_prms         = parse_band2_prms_128x,
	.prs_band5_prms         = parse_band5_prms_128x,
	.prs_fem0_band2_prms    = parse_fem0_band2_prms_128x,
	.prs_fem1_band2_prms    = parse_fem1_band2_prms_128x,
	.prs_fem0_band5_prms    = parse_fem0_band5_prms_128x,
	.prs_fem1_band5_prms    = parse_fem1_band5_prms_128x,
	.is_dual_mode		= is_dual_mode_128x,
};

int ini_get_dual_mode(struct wl12xx_common *cmn)
{
	return cmn->parse_ops->is_dual_mode(&cmn->ini);
}

int nvs_get_arch(int file_size, struct wl12xx_common *cmn)
{
	enum wl12xx_arch arch = UNKNOWN_ARCH;

	switch (file_size) {
		case WL127X_NVS_FILE_SZ:
			arch = WL1271_ARCH;
			cmn->parse_ops = &wl1271_parse_ops;
			break;
		case WL128X_NVS_FILE_SZ:
			arch = WL128X_ARCH;
			cmn->parse_ops = &wl128x_parse_ops;
			break;
	}

	if (cmn->arch != UNKNOWN_ARCH && cmn->arch != arch) {
		cmn->parse_ops = NULL;
		return 1;
	}

	cmn->arch = arch;

	return 0;
}

static int ini_get_arch(FILE *f, struct wl12xx_common *cmn)
{
	char buf[1024], *pos;
	int line = 0;
	enum wl12xx_arch arch = UNKNOWN_ARCH;

	while (ini_get_line(buf, sizeof(buf), f, &line, &pos)) {
		if (strncmp("TCXO_Clk", pos, 8) == 0) {
			arch = WL128X_ARCH;
			break;
		}
	}

	if (arch == UNKNOWN_ARCH)
		arch = WL1271_ARCH;

	if (cmn->arch != UNKNOWN_ARCH && cmn->arch != arch)
		return 1;

	cmn->arch = arch;

	if (cmn->arch == WL1271_ARCH)
		cmn->parse_ops = &wl1271_parse_ops;
	else
		cmn->parse_ops = &wl128x_parse_ops;

	fseek(f, 0L, SEEK_SET);

	return 0;
}

int read_ini(const char *filename, struct wl12xx_common *cmn)
{
	FILE *f;
	char buf[1024], *pos;
	int ret = 0, line = 0;

	cmn->auto_fem = 0;
	cmn->fem0_bands = 0;
	cmn->fem1_bands = 0;

	f = fopen(filename, "r");
	if (f == NULL) {
		fprintf(stderr, "Unable to open file %s (%s)\n",
			filename, strerror(errno));
		return 1;
	}

	/* check if it 127x or 128x */
	if (ini_get_arch(f, cmn)) {
		fprintf(stderr, "Unable to define wireless architecture\n");
		ret = 1;
		goto out;
	}

	/* start parsing */
	while (ini_get_line(buf, sizeof(buf), f, &line, &pos)) {
		ret = ini_parse_line(pos, cmn);
		if (ret) break;
	}

out:
	fclose(f);
#if 0
	ini_dump(ini);
#endif
	return ret;
}
