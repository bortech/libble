#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "libble.h"

#define VECS_BUTTON_NOTI_VAL	0x003b
#define VECS_BUTTON_NOTI_CFG	0x003c

static const char *vecs_addr = "84:DD:20:F0:86:AB";

void noti_handler(uint16_t handle, uint8_t len, const uint8_t *data)
{
	if (handle == VECS_BUTTON_NOTI_VAL && len == 1) {
		switch (*data) {
			case 1:
				printf("NOTIFICATION: single click\n");
				break;
			case 2:
				printf("NOTIFICATION: double click\n");
				break;
			case 3:
				printf("NOTIFICATION: long click\n");
				break;
		}
	} else
		printf("unnokwn notification (handle: 0x%x)\n", handle);
}

int main(int argc, char **argv)
{
	printf("connecting to %s\n", vecs_addr);

	lble_connect(vecs_addr);

	if (lble_get_state() != STATE_CONNECTED) {
		fprintf(stderr, "error: connection failed\n");
		return -1;
	}
	printf("connection successful\n");

	printf("enabling notifications for button events...\n");
	lble_write(VECS_BUTTON_NOTI_CFG, 2, (uint8_t *)"\x01\x00");

	printf("listening for notifications\n");
	lble_listen(noti_handler);

	lble_disconnect();
	return 0;
}

