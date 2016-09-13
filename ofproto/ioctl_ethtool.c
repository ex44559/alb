#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>

#include "ioctl_ethtool.h"

#define ETHTOOL_FWVERS_LEN	32
#define ETHTOOL_BUSINFO_LEN	32
#define ETHTOOL_EROMVERS_LEN	32

typedef unsigned long long u64;
typedef uint32_t u32;
typedef uint16_t u16;

int send_ioctl(struct cmd_context *ctx, void *cmd)
{
	ctx->ifr.ifr_data = cmd;
	return ioctl(ctx->fd, SIOCETHTOOL, &ctx->ifr);
}

struct ethtool_gstrings *
get_stringset(struct cmd_context *ctx, enum ethtool_stringset set_id, 
		ptrdiff_t drvinfo_offset, int null_terminate)
{
	struct {
		struct ethtool_sset_info hdr;
		u32 buf[1];
	} sset_info;
	struct ethtool_drvinfo drvinfo;
	u32 len, i;
	struct ethtool_gstrings *strings;

	sset_info.hdr.cmd = ETHTOOL_GSSET_INFO;
	sset_info.hdr.reserved = 0;
	sset_info.hdr.sset_mask = 1ULL << set_id;

	if (send_ioctl(ctx, &sset_info) == 0) {
		len = sset_info.hdr.sset_mask ? sset_info.hdr.data[0] : 0;
	} else if (errno == EOPNOTSUPP && drvinfo_offset != 0) {
		/* Fallback for old kernel versions */
		drvinfo.cmd = ETHTOOL_GDRVINFO;
		if (send_ioctl(ctx, &drvinfo))
			return NULL;
		len = *(u32 *)((char *)&drvinfo + drvinfo_offset);
	} else {
		return NULL;
	}

	strings = malloc(sizeof(*strings) + len * ETH_GSTRING_LEN);
	if (!strings)
		return NULL;

	strings->cmd = ETHTOOL_GSTRINGS;
	strings->string_set = set_id;
	strings->len = len;
	if (len != 0 && send_ioctl(ctx, strings)) {
		free(strings);
		return NULL;
	}

	if (null_terminate)
		for (i = 0; i < len; i++)
			strings->data[(i + 1) * ETH_GSTRING_LEN - 1] = 0;

	return strings;
}

/*
require:
@dev_name: name of nic
@nic: a struct for nic load
*/
int nic_investigation(char *dev_name, struct nic_load *nic)
{
	/* code */
	struct cmd_context ctx;

	struct ethtool_gstrings *strings;
	struct ethtool_stats *stats;
	unsigned int n_stats, sz_stats, i;
	int err;

	//TODO: for ioctl, file descriptor and socket are needed.
	ctx.devname = dev_name;
	ctx.fd = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&ctx.ifr, 0, sizeof(ctx.ifr));
	strcpy(ctx.ifr.ifr_name, dev_name);

	strings = get_stringset(&ctx, ETH_SS_STATS, 
					offsetof(struct ethtool_drvinfo, n_stats), 0);

	if (!strings) {
		perror("Cannot get stats strings information");
		return 96;
	}

	n_stats = strings->len;
	if (n_stats < 1) {
		fprintf(stderr, "no stats available\n");
		free(strings);
		return 94;
	}

	sz_stats = n_stats * sizeof(u64);

	stats = malloc(sz_stats + sizeof(struct ethtool_stats));
	if (!stats) {
		fprintf(stderr, "no memory available\n");
		free(strings);
		return 95;
	}

	stats->cmd = ETHTOOL_GSTATS;
	stats->n_stats = n_stats;
	err = send_ioctl(&ctx, stats);
	if (err < 0) {
		perror("Cannot get stats information");
		free(strings);
		free(stats);
		return 97;
	}

	fprintf(stdout, "NIC statistics:\n");
	for (i = 0; i < n_stats; i++) {
		fprintf(stdout, "     %.*s: %llu\n",
			ETH_GSTRING_LEN,
			&strings->data[i * ETH_GSTRING_LEN],
			stats->data[i]);
	}

	nic->devname = dev_name;
	nic->rx_packet = stats->data[0];
	nic->tx_packet = stats->data[1];
	nic->rx_bytes = stats->data[2];
	nic->tx_bytes = stats->data[3];

	free(strings);
	free(stats);

	return 0;
}

/*
int main(int argc, char const *argv[])
{
	char *dev_name = "ens5";
	struct nic_load nic;
	int err;

	err = nic_investigation(dev_name, &nic);

	printf("rx packet %d\n", nic.rx_packet);

	return 0;
}*/