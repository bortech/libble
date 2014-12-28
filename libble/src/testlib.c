#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "libble.h"

//static const char *dev_addr = "84:DD:20:F0:86:AB";
static const char *keyfob_addr = "84:DD:20:C5:70:43";

#define KEYFOB_BAT_LEVEL 	0x2f
#define KEYFOB_ENABLE_ACCEL	0x34

#define KEYFOB_ACCEL_X	0x3a
#define KEYFOB_ACCEL_Y	0x3e
#define KEYFOB_ACCEL_Z	0x42

int8_t accX, accY, accZ;

uint32_t counter = 0;
double speed = 0;
double time = 0;

struct timeval t_old, t_new;

void keyfob_handler(uint16_t handle, uint8_t len, const uint8_t *data)
{
	int i;

	counter += len;

	gettimeofday(&t_new, NULL);
	time = t_new.tv_sec - t_old.tv_sec;
	time += (t_new.tv_usec - t_old.tv_usec) / 1000000.0;
	if (time >= 1.0) {
		speed = (counter << 3) / time;
		counter = 0;
		t_old = t_new;
	}

	switch (handle) {
		case KEYFOB_ACCEL_X:
			accX = (int8_t)data[0];
			accY = (int8_t)data[1];
			accZ = (int8_t)data[2];
			break;
//		case KEYFOB_ACCEL_Y:
//			accY = (int8_t)data[0];
//			break;
//		case KEYFOB_ACCEL_Z:
//			accZ = (int8_t)data[0];
//			break;
		default:
			printf("[%04x] data: ", handle);
			for (i = 0; i < len; i++)
				printf("%02x ", data[i]);
			printf("\n");
	}

	printf("x: %4d y: %4d z: %4d speed: %5.1f bps len: %d\n", accX, accY, accZ, speed, len);
}

int main(int argc, char **argv)
{
	uint8_t bat_level;

	printf("connecting to %s\n", keyfob_addr);

	lble_connect(keyfob_addr);

	if (lble_get_state() != STATE_CONNECTED) {
		fprintf(stderr, "error: connection failed\n");
		return -1;
	}
	
	printf("connection successful\n");

	lble_read(KEYFOB_BAT_LEVEL, &bat_level);
	printf("battery: %d\n", bat_level);

// enable accel
	lble_write(KEYFOB_ENABLE_ACCEL, 2, (uint8_t *)"\x01\x00");

// wait for connection parameters update
	printf("waiting for connection update...\n");
	sleep(5);

// enable notifications for X axis
	lble_write(KEYFOB_ACCEL_X + 1, 2, (uint8_t *)"\x01\x00");
// enable notifications for Y axis
//	lble_write(KEYFOB_ACCEL_Y + 1, 2, (uint8_t *)"\x01\x00");
// enable notifications for Z axis
//	lble_write(KEYFOB_ACCEL_Z + 1, 2, (uint8_t *)"\x01\x00");

	printf("listening for notifications\n");
	lble_listen(keyfob_handler);

	lble_disconnect();
	return 0;
}


