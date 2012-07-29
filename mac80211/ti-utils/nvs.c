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
#include <netinet/in.h>
#include <time.h>

#include <netlink/netlink.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/wireless.h>
#include "nl80211.h"

#include "calibrator.h"
#include "plt.h"
#include "ini.h"
/* 2048 - it should be enough for any chip, until... 22dec2010 */
#define BUF_SIZE_4_NVS_FILE	2048

static const char if_name_fmt[] = "wlan%d";

static char* get_opt_file(int argc, char **argv, char *dir, char *def)
{
	char *name = NULL;
	if (argc < 0)
		return NULL;
	else if (argc == 0) {
		name = def;
		fprintf(stderr, "\nThe path to input %s file not provided, "
		       "use default (%s)\n", dir, name);
	}
	else
		name = *argv;
	return name;
}


char *get_opt_nvsinfile(int argc, char **argv)
{
	char *name = get_opt_file(argc, argv, "input", CURRENT_NVS_NAME);
	if (file_exist(name) < 0) {
		fprintf(stderr, "File not found %s\n", name);
		return NULL;
	}
	return name;
}

char *get_opt_nvsoutfile(int argc, char **argv)
{
	char *name = get_opt_file(argc, argv, "output", NEW_NVS_NAME);
	return name;
}

int nvs_set_mac(char *nvsfile, char *mac)
{
	unsigned char mac_buff[12];
	unsigned char in_mac[6];
	int fd;
	unsigned int lower;

	if (mac) {
		int ret =
		sscanf(mac, "%2x:%2x:%2x:%2x:%2x:%2x",
		(unsigned int *)&in_mac[0], (unsigned int *)&in_mac[1],
		(unsigned int *)&in_mac[2], (unsigned int *)&in_mac[3],
		(unsigned int *)&in_mac[4], (unsigned int *)&in_mac[5]);
		if (ret != 6) {
			fprintf(stderr, "MAC address is not valid: %s\n", mac);
			return -1;
		}
	}
	else {
		fprintf(stderr, "No MAC address specified\n");
		return -1;
	}

	fd = open(nvsfile, O_RDWR);
	if (fd < 0) {
		perror("Error opening file for reading");
		return 1;
	}

	read(fd, mac_buff, 12);
	mac_buff[11] = in_mac[0];
	mac_buff[10] = in_mac[1];
	mac_buff[6]  = in_mac[2];
	mac_buff[5]  = in_mac[3];
	mac_buff[4]  = in_mac[4];
	mac_buff[3]  = in_mac[5];

	lseek(fd, 0L, 0);

	/* we need at least two valid NIC addresses */
	lower = (in_mac[3] << 16) + (in_mac[4] << 8) + in_mac[5];
	if (lower + 1 > 0xffffff)
		fprintf(stderr,
			"WARNING: NIC part of the MAC address wraps around!\n");

	printf("Writing mac address %s to file %s\n", mac, nvsfile);
	write(fd, mac_buff, 12);

	close(fd);
	return 0;
}



int nvs_fill_radio_params(int fd, struct wl12xx_ini *ini, char *buf)
{
	size_t size;
	struct wl1271_ini *gp;

	if (ini)
		gp = &ini->ini1271;

	size  = sizeof(struct wl1271_ini);

	if (ini) {	/* for reference NVS */
		unsigned char *c = (unsigned char *)gp;
		size_t i;

		for (i = 0; i < size; i++)
			write(fd, c++, 1);
	} else {
		char *p = buf + 0x1D4;
		write(fd, (const void *)p, size);
	}

	return 0;
}

static int nvs_fill_radio_params_128x(int fd, struct wl12xx_ini *ini, char *buf)
{
	int size;
	struct wl128x_ini *gp = &ini->ini128x;

	size  = sizeof(struct wl128x_ini);

	if (ini) {	/* for reference NVS */
		unsigned char *c = (unsigned char *)gp;
		int i;

		for (i = 0; i < size; i++)
			write(fd, c++, 1);

	} else {
		char *p = buf + 0x1D4;
		write(fd, p, size);
	}

	return 0;
}

int nvs_set_autofem(int fd, char *buf, unsigned char val)
{
	size_t size, i;
	struct wl1271_ini *gp;
	unsigned char *c;

	if (buf == NULL)
		return 1;

	gp = (struct wl1271_ini *)(buf+0x1d4);
	gp->general_params.tx_bip_fem_auto_detect = val;

	size  = sizeof(struct wl1271_ini);

	c = (unsigned char *)gp;

	for (i = 0; i < size; i++)
		write(fd, c++, 1);

	return 0;
}

int nvs_set_autofem_128x(int fd, char *buf, unsigned char val)
{
	size_t size, i;
	struct wl128x_ini *gp;
	unsigned char *c;

	if (buf == NULL)
		return 1;

	gp = (struct wl128x_ini *)(buf+0x1d4);
	gp->general_params.tx_bip_fem_auto_detect = val;

	size  = sizeof(struct wl128x_ini);

	c = (unsigned char *)gp;

	for (i = 0; i < size; i++)
		write(fd, c++, 1);

	return 0;
}

int nvs_set_fem_manuf(int fd, char *buf, unsigned char val)
{
	size_t size, i;
	struct wl1271_ini *gp;
	unsigned char *c;

	if (buf == NULL)
		return 1;

	gp = (struct wl1271_ini *)(buf+0x1d4);
	gp->general_params.tx_bip_fem_manufacturer = val;

	size  = sizeof(struct wl1271_ini);

	c = (unsigned char *)gp;

	for (i = 0; i < size; i++)
		write(fd, c++, 1);

	return 0;
}

int nvs_set_fem_manuf_128x(int fd, char *buf, unsigned char val)
{
	size_t size, i;
	struct wl128x_ini *gp;
	unsigned char *c;

	if (buf == NULL)
		return 1;

	gp = (struct wl128x_ini *)(buf+0x1d4);
	gp->general_params.tx_bip_fem_manufacturer = val;

	size  = sizeof(struct wl128x_ini);

	c = (unsigned char *)gp;

	for (i = 0; i < size; i++)
		write(fd, c++, 1);

	return 0;
}

static struct wl12xx_nvs_ops wl1271_nvs_ops = {
	.nvs_fill_radio_prms = nvs_fill_radio_params,
	.nvs_set_autofem = nvs_set_autofem,
	.nvs_set_fem_manuf = nvs_set_fem_manuf,
};

static struct wl12xx_nvs_ops wl128x_nvs_ops = {
	.nvs_fill_radio_prms = nvs_fill_radio_params_128x,
	.nvs_set_autofem = nvs_set_autofem_128x,
	.nvs_set_fem_manuf = nvs_set_fem_manuf_128x,
};

int get_mac_addr(int ifc_num, unsigned char *mac_addr)
{
	int s;
	struct ifreq ifr;
#if 0
	if (ifc_num < 0 || ifc_num >= ETH_DEV_MAX)
		return 1;
#endif
	s = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (s < 0) {
		fprintf(stderr, "unable to socket (%s)\n", strerror(errno));
		return 1;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	sprintf(ifr.ifr_name, if_name_fmt, ifc_num) ;
	if (ioctl(s, SIOCGIFHWADDR, &ifr) < 0) {
		fprintf(stderr, "unable to ioctl (%s)\n", strerror(errno));
		close(s);
		return 1;
	}

	close(s);

	memcpy(mac_addr, &ifr.ifr_ifru.ifru_hwaddr.sa_data[0], 6);

	return 0;
}

int file_exist(const char *filename)
{
	struct stat buf;
	int ret;

	if (filename == NULL) {
		fprintf(stderr, "wrong parameter\n");
		return -1;
	}

	ret = stat(filename, &buf);
	if (ret != 0) {
		return -1;
	}

	return (int)buf.st_size;
}

void cfg_nvs_ops(struct wl12xx_common *cmn)
{
	if (cmn->arch == WL1271_ARCH)
		cmn->nvs_ops = &wl1271_nvs_ops;
	else
		cmn->nvs_ops = &wl128x_nvs_ops;
}

static int read_from_current_nvs(const char *nvs_file,
	char *buf, int size, int *nvs_sz)
{
	int curr_nvs, ret;

	curr_nvs = open(nvs_file, O_RDONLY, S_IRUSR | S_IWUSR);
	if (curr_nvs < 0) {
		fprintf(stderr, "%s> Unable to open NVS file for reference "
			"(%s)\n", __func__, strerror(errno));
		return 1;
	}

	ret = read(curr_nvs, buf, size);
	if (ret < 0) {
		fprintf(stderr, "Fail to read file %s (%s)", nvs_file,
			strerror(errno));
		close(curr_nvs);
		return 1;
	}

	if (nvs_sz)
		*nvs_sz = ret;

	close(curr_nvs);

	//printf("Read NVS file (%s) of size %d\n", nvs_file, ret);

	return 0;
}

static int read_nvs(const char *nvs_file, char *buf,
	int size, int *nvs_sz)
{
	int fl_sz;
	fl_sz = file_exist(nvs_file);
	if (fl_sz < 0) {
		fprintf(stderr, "File %s not exists\n", nvs_file);
		return 1;
	}

	return read_from_current_nvs(nvs_file, buf, size, nvs_sz);
}

static int fill_nvs_def_rx_params(int fd)
{
	unsigned char type = eNVS_RADIO_RX_PARAMETERS;
	unsigned short length = NVS_RX_PARAM_LENGTH;
	int i;

	/* Rx type */
	write(fd, &type, 1);

	/* Rx length */
	write(fd, &length, 2);

	type = DEFAULT_EFUSE_VALUE; /* just reuse of var */
	for (i = 0; i < NVS_RX_PARAM_LENGTH; i++)
		write(fd, &type, 1);

	return 0;
}

static void nvs_parse_data(const unsigned char *buf,
	struct wl1271_cmd_cal_p2g *pdata, unsigned int *pver)
{
#define BUFFER_INDEX    (buf_idx + START_PARAM_INDEX + info_idx)
	unsigned short buf_idx;
	unsigned char tlv_type;
	unsigned short tlv_len;
	unsigned short info_idx;
	unsigned int nvsTypeInfo = 0;
	unsigned char nvs_ver_oct_idx;
	unsigned char shift;

	for (buf_idx = 0; buf_idx < NVS_TOTAL_LENGTH;) {
		tlv_type = buf[buf_idx];

		/* fill the correct mode to fill the NVS struct buffer */
		/* if the tlv_type is the last type break from the loop */
		switch (tlv_type) {
		case eNVS_RADIO_TX_PARAMETERS:
			nvsTypeInfo = eNVS_RADIO_TX_TYPE_PARAMETERS_INFO;
			break;
		case eNVS_RADIO_RX_PARAMETERS:
			nvsTypeInfo = eNVS_RADIO_RX_TYPE_PARAMETERS_INFO;
			break;
		case eNVS_VERSION:
			for (*pver = 0, nvs_ver_oct_idx = 0;
				nvs_ver_oct_idx < NVS_VERSION_PARAMETER_LENGTH;
				nvs_ver_oct_idx++) {
				shift = 8 * (NVS_VERSION_PARAMETER_LENGTH -
					1 - nvs_ver_oct_idx);
				*pver += ((buf[buf_idx + START_PARAM_INDEX +
					nvs_ver_oct_idx]) << shift);
			}
			break;
		case eTLV_LAST:
		default:
			return;
		}

		tlv_len = (buf[buf_idx + START_LENGTH_INDEX  + 1] << 8) +
			buf[buf_idx + START_LENGTH_INDEX];

		/* if TLV type is not NVS ver fill the NVS according */
		/* to the mode TX/RX */
		if ((eNVS_RADIO_TX_PARAMETERS == tlv_type) ||
			(eNVS_RADIO_RX_PARAMETERS == tlv_type)) {
			pdata[nvsTypeInfo].type = tlv_type;
			pdata[nvsTypeInfo].len = tlv_len;

			for (info_idx = 0; (info_idx < tlv_len) &&
				(BUFFER_INDEX < NVS_TOTAL_LENGTH);
					info_idx++) {
				pdata[nvsTypeInfo].buf[info_idx] =
					buf[BUFFER_INDEX];
			}
		}

		/* increment to the next TLV */
		buf_idx += START_PARAM_INDEX + tlv_len;
	}
}

static int nvs_fill_version(int fd, unsigned int *pdata)
{
	unsigned char tmp = eNVS_VERSION;
	unsigned short tmp2 = NVS_VERSION_PARAMETER_LENGTH;

	write(fd, &tmp, 1);

	write(fd, &tmp2, 2);

	tmp = (*pdata >> 16) & 0xff;
	write(fd, &tmp, 1);

	tmp = (*pdata >> 8) & 0xff;
	write(fd, &tmp, 1);

	tmp = *pdata & 0xff;
	write(fd, &tmp, 1);

	return 0;
}

static int nvs_fill_old_rx_data(int fd, const unsigned char *buf,
	unsigned short len)
{
	unsigned short idx;
	unsigned char rx_type;

	/* RX BiP type */
	rx_type = eNVS_RADIO_RX_PARAMETERS;
	write(fd, &rx_type, 1);

	/* RX BIP Length */
	write(fd, &len, 2);

	for (idx = 0; idx < len; idx++)
		write(fd, &(buf[idx]), 1);

	return 0;
}

static int nvs_upd_nvs_part(int fd, char *buf)
{
	char *p = buf;

	write(fd, p, 0x1D4);

	return 0;
}

static int nvs_fill_nvs_part(int fd)
{
	int i;
	unsigned char mac_addr[MAC_ADDR_LEN] = {
		 0x0b, 0xad, 0xde, 0xad, 0xbe, 0xef
	};
	__le16 nvs_tx_sz = NVS_TX_PARAM_LENGTH;
	__le32 nvs_ver = 0x0;
	const unsigned char vals[] = {
		0x0, 0x1, 0x6d, 0x54, 0x71, eTLV_LAST, eNVS_RADIO_TX_PARAMETERS
	};

	write(fd, &vals[1], 1);
	write(fd, &vals[2], 1);
	write(fd, &vals[3], 1);
#if 0
	if (get_mac_addr(0, mac_addr)) {
		fprintf(stderr, "%s> Fail to get mac address\n", __func__);
		return 1;
	}
#endif
	/* write down MAC address in new NVS file */
	write(fd, &mac_addr[5], 1);
	write(fd, &mac_addr[4], 1);
	write(fd, &mac_addr[3], 1);
	write(fd, &mac_addr[2], 1);

	write(fd, &vals[1], 1);
	write(fd, &vals[4], 1);
	write(fd, &vals[3], 1);

	write(fd, &mac_addr[1], 1);
	write(fd, &mac_addr[0], 1);

	write(fd, &vals[0], 1);
	write(fd, &vals[0], 1);

	/* fill end burst transaction zeros */
	for (i = 0; i < NVS_END_BURST_TRANSACTION_LENGTH; i++)
		write(fd, &vals[0], 1);

	/* fill zeros to Align TLV start address */
	for (i = 0; i < NVS_ALING_TLV_START_ADDRESS_LENGTH; i++)
		write(fd, &vals[0], 1);

	/* Fill Tx calibration part */
	write(fd, &vals[6], 1);
	write(fd, &nvs_tx_sz, 2);

	for (i = 0; i < nvs_tx_sz; i++)
		write(fd, &vals[0], 1);

	/* Fill Rx calibration part */
	fill_nvs_def_rx_params(fd);

	/* fill NVS version */
	if (nvs_fill_version(fd, &nvs_ver))
		fprintf(stderr, "Fail to fill version\n");

	/* fill end of NVS */
	write(fd, &vals[5], 1); /* eTLV_LAST */
	write(fd, &vals[5], 1); /* eTLV_LAST */
	write(fd, &vals[0], 1);
	write(fd, &vals[0], 1);

	return 0;
}

int prepare_nvs_file(void *arg, char *file_name)
{
	int new_nvs, i, nvs_size;
	unsigned char mac_addr[MAC_ADDR_LEN];
	struct wl1271_cmd_cal_p2g *pdata;
	struct wl1271_cmd_cal_p2g old_data[eNUMBER_RADIO_TYPE_PARAMETERS_INFO];
	char buf[2048];
	unsigned char *p;
	struct wl12xx_common cmn = {
		.arch = UNKNOWN_ARCH,
		.parse_ops = NULL
	};

	const unsigned char vals[] = {
		0x0, 0x1, 0x6d, 0x54, 0x71, eTLV_LAST, eNVS_RADIO_TX_PARAMETERS
	};

	if (arg == NULL) {
		fprintf(stderr, "%s> Missing args\n", __func__);
		return 1;
	}

	if (read_nvs(file_name, buf, BUF_SIZE_4_NVS_FILE, &nvs_size))
		return 1;

	switch (nvs_size) {
		case NVS_FILE_SIZE_127X:
			cmn.arch = WL1271_ARCH;
		break;
		case NVS_FILE_SIZE_128X:
			cmn.arch = WL128X_ARCH;
		break;
		default:
			fprintf(stderr, "%s> Wrong file size\n", __func__);
		return 1;
	}

	cfg_nvs_ops(&cmn);

	/* create new NVS file */
	new_nvs = open(file_name,
		O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (new_nvs < 0) {
		fprintf(stderr, "%s> Unable to open new NVS file\n", __func__);
		return 1;
	}

	write(new_nvs, &vals[1], 1);
	write(new_nvs, &vals[2], 1);
	write(new_nvs, &vals[3], 1);

	if (get_mac_addr(0, mac_addr)) {
		fprintf(stderr, "%s> Fail to get mac addr\n", __func__);
		close(new_nvs);
		return 1;
	}

	/* write down MAC address in new NVS file */
	write(new_nvs, &mac_addr[5], 1);
	write(new_nvs, &mac_addr[4], 1);
	write(new_nvs, &mac_addr[3], 1);
	write(new_nvs, &mac_addr[2], 1);

	write(new_nvs, &vals[1], 1);
	write(new_nvs, &vals[4], 1);
	write(new_nvs, &vals[3], 1);

	write(new_nvs, &mac_addr[1], 1);
	write(new_nvs, &mac_addr[0], 1);

	write(new_nvs, &vals[0], 1);
	write(new_nvs, &vals[0], 1);

	/* fill end burst transaction zeros */
	for (i = 0; i < NVS_END_BURST_TRANSACTION_LENGTH; i++)
		write(new_nvs, &vals[0], 1);

	/* fill zeros to Align TLV start address */
	for (i = 0; i < NVS_ALING_TLV_START_ADDRESS_LENGTH; i++)
		write(new_nvs, &vals[0], 1);

	/* Fill TxBip */
	pdata = (struct wl1271_cmd_cal_p2g *)arg;

	write(new_nvs, &vals[6], 1);
	write(new_nvs, &pdata->len, 2);

	p = (unsigned char *)&(pdata->buf);
	for (i = 0; i < pdata->len; i++)
		write(new_nvs, p++, 1);

	{
		unsigned int old_ver;
#if 0
		{
			unsigned char *p = (unsigned char *)buf;
			for (old_ver = 0; old_ver < 1024; old_ver++) {
				if (old_ver%16 == 0)
					printf("\n");
				printf("%02x ", *p++);
			}
		}
#endif
		memset(old_data, 0,
			sizeof(struct wl1271_cmd_cal_p2g)*
				eNUMBER_RADIO_TYPE_PARAMETERS_INFO);
		nvs_parse_data((const unsigned char *)&buf[NVS_PRE_PARAMETERS_LENGTH],
			old_data, &old_ver);

		nvs_fill_old_rx_data(new_nvs,
			old_data[eNVS_RADIO_RX_TYPE_PARAMETERS_INFO].buf,
			old_data[eNVS_RADIO_RX_TYPE_PARAMETERS_INFO].len);
	}

	/* fill NVS version */
	if (nvs_fill_version(new_nvs, &pdata->ver))
		fprintf(stderr, "Fail to fill version\n");

	/* fill end of NVS */
	write(new_nvs, &vals[5], 1); /* eTLV_LAST */
	write(new_nvs, &vals[5], 1); /* eTLV_LAST */
	write(new_nvs, &vals[0], 1);
	write(new_nvs, &vals[0], 1);

	/* fill radio params */
	if (cmn.nvs_ops->nvs_fill_radio_prms(new_nvs, NULL, buf))
		fprintf(stderr, "Fail to fill radio params\n");

	close(new_nvs);

	return 0;
}

int create_nvs_file(struct wl12xx_common *cmn)
{
	int new_nvs, res = 0;
	char buf[2048];

	/* create new NVS file */
	new_nvs = open(cmn->nvs_name,
		O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (new_nvs < 0) {
		fprintf(stderr, "%s> Unable to open new NVS file\n", __func__);
		return 1;
	}

	/* fill nvs part */
	if (nvs_fill_nvs_part(new_nvs)) {
		fprintf(stderr, "Fail to fill NVS part\n");
		res = 1;

		goto out;
	}

	/* fill radio params */
	if (cmn->nvs_ops->nvs_fill_radio_prms(new_nvs, &cmn->ini, buf)) {
		fprintf(stderr, "Fail to fill radio params\n");
		res = 1;
	}

out:
	close(new_nvs);

	return res;
}

int update_nvs_file(const char *nvs_infile, const char *nvs_outfile, struct wl12xx_common *cmn)
{
	int new_nvs, res = 0;
	char buf[2048];

	res = read_nvs(nvs_infile, buf, BUF_SIZE_4_NVS_FILE, NULL);
	if (res)
		return 1;

	/* create new NVS file */
	new_nvs = open(nvs_outfile,
		O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (new_nvs < 0) {
		fprintf(stderr, "%s> Unable to open new NVS file\n", __func__);
		return 1;
	}

	/* fill nvs part */
	if (nvs_upd_nvs_part(new_nvs, buf)) {
		fprintf(stderr, "Fail to fill NVS part\n");
		res = 1;

		goto out;
	}

	/* fill radio params */
	if (cmn->nvs_ops->nvs_fill_radio_prms(new_nvs, &cmn->ini, buf)) {
		printf("Fail to fill radio params\n");
		res = 1;
	}

out:
	close(new_nvs);

	return res;
}

int dump_nvs_file(const char *nvs_file)
{
	int sz=0, size;
	char buf[2048];
	unsigned char *p = (unsigned char *)buf;

	if (read_nvs(nvs_file, buf, BUF_SIZE_4_NVS_FILE, &size))
		return 1;

	printf("\nThe size is %d bytes\n", size);

	for ( ; sz < size; sz++) {
		if (sz%16 == 0)
			printf("\n %04X ", sz);
		printf("%02x ", *p++);
	}
	printf("\n");

	return 0;
}

int set_nvs_file_autofem(const char *nvs_file, unsigned char val,
	struct wl12xx_common *cmn)
{
	int new_nvs, res = 0;
	char buf[2048];
	int nvs_file_sz;

	res = read_nvs(nvs_file, buf, BUF_SIZE_4_NVS_FILE, &nvs_file_sz);
	if (res)
		return 1;

	if (nvs_get_arch(nvs_file_sz, cmn)) {
		fprintf(stderr, "Fail to define architecture\n");
		return 1;
	}

	cfg_nvs_ops(cmn);

	/* create new NVS file */
	new_nvs = open(nvs_file,
		O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (new_nvs < 0) {
		fprintf(stderr, "%s> Unable to open new NVS file\n", __func__);
		return 1;
	}

	/* fill nvs part */
	if (nvs_upd_nvs_part(new_nvs, buf)) {
		fprintf(stderr, "Fail to fill NVS part\n");
		res = 1;

		goto out;
	}

	/* fill radio params */
	if (cmn->nvs_ops->nvs_set_autofem(new_nvs, buf, val)) {
		printf("Fail to fill radio params\n");
		res = 1;
	}

out:
	close(new_nvs);

	return res;
}

int set_nvs_file_fem_manuf(const char *nvs_file, unsigned char val,
	struct wl12xx_common *cmn)
{
	int new_nvs, res = 0;
	char buf[2048];
	int nvs_file_sz;

	res = read_nvs(nvs_file, buf, BUF_SIZE_4_NVS_FILE, &nvs_file_sz);
	if (res)
		return 1;

	if (nvs_get_arch(nvs_file_sz, cmn)) {
		fprintf(stderr, "Fail to define architecture\n");
		return 1;
	}

	cfg_nvs_ops(cmn);

	/* create new NVS file */
	new_nvs = open(nvs_file,
		O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (new_nvs < 0) {
		fprintf(stderr, "%s> Unable to open new NVS file\n", __func__);
		return 1;
	}

	/* fill nvs part */
	if (nvs_upd_nvs_part(new_nvs, buf)) {
		fprintf(stderr, "Fail to fill NVS part\n");
		res = 1;

		goto out;
	}

	/* fill radio params */
	if (cmn->nvs_ops->nvs_set_fem_manuf(new_nvs, buf, val)) {
		printf("Fail to fill radio params\n");
		res = 1;
	}

out:
	close(new_nvs);

	return res;
}

static void _print_hexa(char *name, unsigned char *data, size_t len)
{
	size_t i;

	printf("%s = ", name);
	for (i = 0; i < len; i++) {
		printf("%02X ", *data++);
	}
	printf("\n");
}

#define print_hexa(name, data) _print_hexa(name, data, sizeof(data))


static void print_128x_general_params(struct wl128x_ini_general_params *p)
{
	printf("# SECTION 1.1: General parameters\n");
	printf("TXBiPFEMAutoDetect = %02X\n", p->tx_bip_fem_auto_detect);
	printf("TXBiPFEMManufacturer = %02X\n", p->tx_bip_fem_manufacturer);
	printf("RefClk = %02X\n", p->ref_clock);
	printf("SettlingTime = %02X\n", p->settling_time);
	printf("ClockValidOnWakeup = %02X\n", p->clk_valid_on_wakeup);
	printf("TCXO_Clk = %02X\n", p->tcxo_ref_clock);
	printf("TCXO_SettlingTime = %02X\n", p->tcxo_settling_time);
	printf("TCXO_ClockValidOnWakeup = %02X\n", p->tcxo_valid_on_wakeup);
	printf("TCXO_LDO_Voltage = %02X\n", p->tcxo_ldo_voltage);
	printf("Platform_configuration = %02X\n", p->platform_conf);
	printf("Single_Dual_Band_Solution = %02X\n", p->dual_mode_select);
	print_hexa("Settings", p->general_settings);
	printf("XTALItrimVal = %02X\n", p->xtal_itrim_val);
	printf("SRState = %02X\n", p->sr_state);
	print_hexa("SRF1", p->srf1);
	print_hexa("SRF2", p->srf2);
	print_hexa("SRF3", p->srf3);
	printf("\n");
}

static void print_127x_general_params(struct wl1271_ini_general_params *p)
{
	printf("# SECTION 1.1: General parameters\n");
	printf("TXBiPFEMAutoDetect = %02X\n", p->tx_bip_fem_auto_detect);
	printf("TXBiPFEMManufacturer = %02X\n", p->tx_bip_fem_manufacturer);
	printf("RefClk = %02X\n", p->ref_clock);
	printf("SettlingTime = %02X\n", p->settling_time);
	printf("ClockValidOnWakeup = %02X\n", p->clk_valid_on_wakeup);
	printf("DC2DCMode = %02X\n", p->dc2dc_mode);
	printf("Single_Dual_Band_Solution = %02X\n", p->dual_mode_select);
	printf("Settings = %02X\n", p->general_settings);
	printf("SRState = %02X\n", p->sr_state);
	print_hexa("SRF1", p->srf1);
	print_hexa("SRF2", p->srf2);
	print_hexa("SRF3", p->srf3);
	printf("\n");
}

static void print_127x_band2_params(struct wl1271_ini_band_params_2 *p)
{
	printf("# SECTION 1.2.1: 2.4G parameters\n");
	printf("RxTraceInsertionLoss_2_4G = %02X\n", p->rx_trace_insertion_loss);
	printf("TXTraceLoss_2_4G = %02X\n", p->tx_trace_loss);
	print_hexa("RxRssiAndProcessCompensation_2_4G", p->rx_rssi_process_compens);
	printf("\n");
}

static void print_128x_band2_params(struct wl128x_ini_band_params_2 *p)
{
	printf("# SECTION 1.2.1: 2.4G parameters\n");
	printf("RxTraceInsertionLoss_2_4G = %02X\n", p->rx_trace_insertion_loss);
	print_hexa("TXTraceLoss_2_4G", p->tx_trace_loss);
	printf("\n");
}

static void print_127x_band5_params(struct wl1271_ini_band_params_5 *p)
{
	printf("# SECTION 1.2.2: 5G parameters\n");

	print_hexa("RxTraceInsertionLoss_5G", p->rx_trace_insertion_loss);
	print_hexa("TXTraceLoss_5G", p->tx_trace_loss);
	print_hexa("RxRssiAndProcessCompensation_5G", p->rx_rssi_process_compens);
	printf("\n");
}

static void print_128x_band5_params(struct wl128x_ini_band_params_5 *p)
{
	printf("# SECTION 1.2.2: 5G parameters\n");

	print_hexa("RxTraceInsertionLoss_5G", p->rx_trace_insertion_loss);
	print_hexa("TXTraceLoss_5G", p->tx_trace_loss);
	printf("\n");
}

static void _print_femhexa(char *fem ,char *name, unsigned char *data, int size)
{
	printf("%s", fem);
	_print_hexa(name, data, size);
}

static void _print_femle16a(char *fem, char *name, __le16 *data, int len)
{
	int i;

	printf("%s%s = ", fem, name);
	for (i = 0; i < len; i++) {
		printf("%04X ", *data++);
	}
	printf("\n");
}


#define print_femhexa(fem, name, data) _print_femhexa(fem, name, data, sizeof(data))
#define print_femle16a(fem, name, data) _print_femle16a(fem, name, data, sizeof(data) / 2)

static void print_127x_fem_param2(int femnr, struct wl1271_ini_fem_params_2 *p)
{
	char fem[6];
	int ret;

	printf("# SECTION 2.1.1: 2.4G parameters\n");

	ret = snprintf(fem, sizeof(fem), "FEM%d_", femnr);
	if (ret < 0 || ret >= ((int) sizeof(fem))) {
		printf("# Invalid FEM numer %d\n", femnr);
		return;
	}

	printf("%sTXBiPReferencePDvoltage_2_4G = %04X\n", fem, p->tx_bip_ref_pd_voltage);
	printf("%sTxBiPReferencePower_2_4G = %02X\n", fem, p->tx_bip_ref_power);
	printf("%sTxBiPOffsetdB_2_4G = %02X\n", fem, p->tx_bip_ref_offset);

	print_femhexa(fem, "TxPerRatePowerLimits_2_4G_Normal", p->tx_per_rate_pwr_limits_normal);
	print_femhexa(fem, "TxPerRatePowerLimits_2_4G_Degraded", p->tx_per_rate_pwr_limits_degraded);
	print_femhexa(fem, "TxPerRatePowerLimits_2_4G_Extreme", p->tx_per_rate_pwr_limits_extreme);

	printf("%sDegradedLowToNormalThr_2_4G = %02X\n", fem, p->degraded_low_to_normal_thr);
	printf("%sNormalToDegradedHighThr_2_4G = %02X\n", fem, p->normal_to_degraded_high_thr);

	print_femhexa(fem, "TxPerChannelPowerLimits_2_4G_11b", p->tx_per_chan_pwr_limits_11b);
	print_femhexa(fem, "TxPerChannelPowerLimits_2_4G_OFDM", p->tx_per_chan_pwr_limits_ofdm);
	print_femhexa(fem, "TxPDVsRateOffsets_2_4G", p->tx_pd_vs_rate_offsets);
	print_femhexa(fem, "TxIbiasTable_2_4G", p->tx_ibias);

	printf("%sRxFemInsertionLoss_2_4G = %02X\n", fem, p->rx_fem_insertion_loss);
}

static void print_128x_fem_param2(int femnr, struct wl128x_ini_fem_params_2 *p)
{
	char fem[6];
	sprintf(fem, "FEM%d_", femnr);

	printf("# SECTION 2.1.1: 2.4G parameters\n");

	printf("%sTXBiPReferencePDvoltage_2_4G = %04X\n", fem, p->tx_bip_ref_pd_voltage);
	printf("%sTxBiPReferencePower_2_4G = %02X\n", fem, p->tx_bip_ref_power);
	printf("%sTxBiPOffsetdB_2_4G = %02X\n", fem, p->tx_bip_ref_offset);

	print_femhexa(fem, "TxPerRatePowerLimits_2_4G_Normal", p->tx_per_rate_pwr_limits_normal);
	print_femhexa(fem, "TxPerRatePowerLimits_2_4G_Degraded", p->tx_per_rate_pwr_limits_degraded);
	print_femhexa(fem, "TxPerRatePowerLimits_2_4G_Extreme", p->tx_per_rate_pwr_limits_extreme);

	printf("%sDegradedLowToNormalThr_2_4G = %02X\n", fem, p->degraded_low_to_normal_thr);
	printf("%sNormalToDegradedHighThr_2_4G = %02X\n", fem, p->normal_to_degraded_high_thr);

	print_femhexa(fem, "TxPerChannelPowerLimits_2_4G_11b", p->tx_per_chan_pwr_limits_11b);
	print_femhexa(fem, "TxPerChannelPowerLimits_2_4G_OFDM", p->tx_per_chan_pwr_limits_ofdm);
	print_femhexa(fem, "TxPDVsRateOffsets_2_4G", p->tx_pd_vs_rate_offsets);
	print_femhexa(fem, "TxPDVsChannelOffsets_2_4G", p->tx_pd_vs_chan_offsets);
	print_femhexa(fem, "TxPDVsTemperature_2_4G", p->tx_pd_vs_temperature);
	print_femhexa(fem, "TxIbiasTable_2_4G", p->tx_ibias);

	printf("%sRxFemInsertionLoss_2_4G = %02X\n", fem, p->rx_fem_insertion_loss);
}

static void print_127x_fem_param5(int femnr, struct wl1271_ini_fem_params_5 *p)
{
	char fem[6];
	sprintf(fem, "FEM%d_", femnr);

	printf("# SECTION 2.1.2: 5G parameters\n");

	print_femle16a(fem, "TxBiPReferencePDvoltage_5G", p->tx_bip_ref_pd_voltage);

	print_femhexa(fem, "TxBiPReferencePower_5G", p->tx_bip_ref_power);
	print_femhexa(fem, "TxBiPOffsetdB_5G", p->tx_bip_ref_offset);

	print_femhexa(fem, "TxPerRatePowerLimits_5G_Normal", p->tx_per_rate_pwr_limits_normal);
	print_femhexa(fem, "TxPerRatePowerLimits_5G_Degraded", p->tx_per_rate_pwr_limits_degraded);
	print_femhexa(fem, "TxPerRatePowerLimits_5G_Extreme", p->tx_per_rate_pwr_limits_extreme);

	printf("%sDegradedLowToNormalThr_5G = %02X\n", fem, p->degraded_low_to_normal_thr);
	printf("%sNormalToDegradedHighThr_5G = %02X\n", fem, p->normal_to_degraded_high_thr);

	print_femhexa(fem, "TxPerChannelPowerLimits_5G_OFDM", p->tx_per_chan_pwr_limits_ofdm);
	print_femhexa(fem, "TxPDVsRateOffsets_5G", p->tx_pd_vs_rate_offsets);
	print_femhexa(fem, "TxIbiasTable_5G", p->tx_ibias);
	print_femhexa(fem, "RxFemInsertionLoss_5G", p->rx_fem_insertion_loss);
}

static void print_128x_fem_param5(int femnr, struct wl128x_ini_fem_params_5 *p)
{
	char fem[6];
	sprintf(fem, "FEM%d_", femnr);

	printf("# SECTION 2.1.2: 5G parameters\n");

	print_femle16a(fem, "TxBiPReferencePDvoltage_5G", p->tx_bip_ref_pd_voltage);

	print_femhexa(fem, "TxBiPReferencePower_5G", p->tx_bip_ref_power);
	print_femhexa(fem, "TxBiPOffsetdB_5G", p->tx_bip_ref_offset);

	print_femhexa(fem, "TxPerRatePowerLimits_5G_Normal", p->tx_per_rate_pwr_limits_normal);
	print_femhexa(fem, "TxPerRatePowerLimits_5G_Degraded", p->tx_per_rate_pwr_limits_degraded);
	print_femhexa(fem, "TxPerRatePowerLimits_5G_Extreme", p->tx_per_rate_pwr_limits_extreme);

	printf("%sDegradedLowToNormalThr_5G = %02X\n", fem, p->degraded_low_to_normal_thr);
	printf("%sNormalToDegradedHighThr_5G = %02X\n", fem, p->normal_to_degraded_high_thr);

	print_femhexa(fem, "TxPerChannelPowerLimits_5G_OFDM", p->tx_per_chan_pwr_limits_ofdm);
	print_femhexa(fem, "TxPDVsRateOffsets_5G", p->tx_pd_vs_rate_offsets);
	print_femhexa(fem, "TxPDVsChannelOffsets_5G", p->tx_pd_vs_chan_offsets);
	print_femhexa(fem, "TxPDVsTemperature_5G", p->tx_pd_vs_temperature);
	print_femhexa(fem, "TxIbiasTable_5G", p->tx_ibias);
	print_femhexa(fem, "RxFemInsertionLoss_5G", p->rx_fem_insertion_loss);
}

int get_fem_nr(int autodetect, int manuf, int *femcnt, int *femi)
{
	if (autodetect) {
		printf("#Fem autodetect is on. Showing both FEM datas\n");
		*femcnt = 2;
		*femi = 0;
	}
	else {
		*femcnt = 1;
		if(manuf >= WL1271_INI_FEM_MODULE_COUNT) {
			fprintf(stderr, "FEM index out of bounds (%d > %d)\n", manuf, WL1271_INI_FEM_MODULE_COUNT);
			return 1;
		}

		*femi = manuf;
		printf("#Fem autodetect is off. Fem nr used is %d\n", *femi);
	}
	return 0;
}



int info_nvs_file(const char *nvs_file)
{
	char buf[BUF_SIZE_4_NVS_FILE];
	int ret, i, femi, femcnt, maxfem;

	int fd =  open(nvs_file, O_RDONLY, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		fprintf(stderr, "Unable to open NVS %s ", nvs_file);
		return 1;
	}

	ret = read(fd, buf, BUF_SIZE_4_NVS_FILE);
	if (ret < 0) {
		fprintf(stderr, "Fail to read file %s (%s)", nvs_file,
			strerror(errno));
		close(fd);
		return 1;
	}
	close(fd);

	if (ret == sizeof(struct wl1271_nvs_file)) {
		struct wl1271_nvs_file *nvs = (struct wl1271_nvs_file *) &buf;
		printf("#Chip is 127x\n");
		print_127x_general_params(&nvs->general_params);
		print_127x_band2_params(&nvs->stat_radio_params_2);
		if (nvs->general_params.dual_mode_select)
			print_127x_band5_params(&nvs->stat_radio_params_5);

		if( get_fem_nr(nvs->general_params.tx_bip_fem_auto_detect,
		               nvs->general_params.tx_bip_fem_manufacturer,
		               &femcnt, &femi))
			return 1;

		maxfem = femcnt + femi;
		for (i = femi; i < maxfem; i++) {
			print_127x_fem_param2(i, &nvs->dyn_radio_params_2[i].params);
			printf("\n");

			if (nvs->general_params.dual_mode_select == 1) {
				print_127x_fem_param5(femi, &nvs->dyn_radio_params_5[i].params);
				printf("\n");
			}
			femi++;
		}
	}
	else if (ret == sizeof(struct wl128x_nvs_file)) {
		struct wl128x_nvs_file *nvs = (struct wl128x_nvs_file *) &buf;
		printf("#Chip is 128x\n");
		print_128x_general_params(&nvs->general_params);
		print_128x_band2_params(&nvs->stat_radio_params_2);
		if (nvs->general_params.dual_mode_select)
			print_128x_band5_params(&nvs->stat_radio_params_5);

		printf("# SECTION 2.1:   FEM parameters\n");
		printf("FemVendorAndOptions = %02X\n\n", nvs->fem_vendor_and_options);

		if( get_fem_nr(nvs->general_params.tx_bip_fem_auto_detect,
		               nvs->general_params.tx_bip_fem_manufacturer,
		               &femcnt, &femi))
			return 1;

		maxfem = femcnt + femi;
		for (i = femi; i < maxfem; i++) {
			print_128x_fem_param2(femi, &nvs->dyn_radio_params_2[femi].params);
			printf("\n");
			if (nvs->general_params.dual_mode_select == 1) {
				print_128x_fem_param5(femi, &nvs->dyn_radio_params_5[femi].params);
				printf("\n");
			}
			femi++;
		}
	}
	else {
		fprintf(stderr, "Invalid file size %d. Unable to detect chip type\n", ret);
		return 0;
	}

	return 0;
}

