#include <stdio.h>
#include <stddef.h>
#include <net/if.h>
#include <linux/ethtool.h>

struct cmd_context {
	const char *devname;	/* net device name */
	int fd;			/* socket suitable for ethtool ioctl */
	struct ifreq ifr;	/* ifreq suitable for ethtool ioctl */
	int argc;		/* number of arguments to the sub-command */
	char **argp;		/* arguments to the sub-command */
};

struct nic_load {
	const char *devname;
	unsigned int tx_packet;
	unsigned int rx_packet;
	unsigned int tx_bytes;
	unsigned int rx_bytes;
};

int send_ioctl(struct cmd_context *ctx, void *cmd);
struct ethtool_gstrings *get_stringset(struct cmd_context *ctx, enum ethtool_stringset set_id, 
		ptrdiff_t drvinfo_offset, int null_terminate);

int nic_investigation(char *dev_name, struct nic_load *nic);