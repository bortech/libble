#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "libble.h"

static const char *dev_addr = "84:DD:20:F0:86:AB";

#define VECS_CHAR_MPU_TEMPERATURE 0x0032

int main(int argc, char **argv)
{
	float temp;	
	int16_t raw_temp;
	uint8_t data[2];

	printf("connecting to %s\n", dev_addr);
	lble_connect(dev_addr);
	if (lble_get_state() != STATE_CONNECTED) {
		fprintf(stderr, "error: connection failed\n");
		return -1;
	}
	printf("connection successful\n");

	lble_read(VECS_CHAR_MPU_TEMPERATURE, data);

	raw_temp = (data[0] << 8) | data[1];
	temp = raw_temp / 340.0 + 35.0;

	printf("temperature: %.1f *C\n", temp);

	lble_disconnect();
	return 0;
}


