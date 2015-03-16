#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "libble.h"

static const char *dev_addr = "84:DD:20:F0:86:AB";

#define VECS_CHAR_BATT_LEVEL 0x0033

int main(int argc, char **argv)
{
	uint8_t bat_level;

	printf("connecting to %s\n", dev_addr);
	lble_connect(dev_addr);
	if (lble_get_state() != STATE_CONNECTED) {
		fprintf(stderr, "error: connection failed\n");
		return -1;
	}
	printf("connection successful\n");

	lble_read(VECS_CHAR_BATT_LEVEL, (uint8_t *)&bat_level);
	printf("battery: %d%%\n", bat_level);

	lble_disconnect();
	return 0;
}


