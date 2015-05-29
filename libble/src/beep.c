#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libble.h"

static const char *dev_addr = "84:DD:20:F0:86:AB";

#define VECS_CHAR_BEEP_REQUEST	0x003f

int main(int argc, char **argv)
{
    uint8_t delay = 1;

	if (argc == 2) {
			delay = atoi(argv[1]);
	} else {
			printf("\nusage: %s <delay between beeps>\n\n", argv[0]);
	}

	printf("connecting to %s\n", dev_addr);
	lble_connect(dev_addr);

	if (lble_get_state() != STATE_CONNECTED) {
		fprintf(stderr, "error: connection failed\n");
		return -1;
	}
	printf("connection successful\n");

	printf("asking to beep every %d seconds...\n", delay);
	lble_write(VECS_CHAR_BEEP_REQUEST, 1, &delay);

	printf("disconnect\n");
	lble_disconnect();
	return 0;
}


