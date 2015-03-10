#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "libble.h"

static const char *dev_addr = "84:DD:20:F0:86:AB";
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

uint8_t pps = 100;


uint16_t voltage, dcr, ccr;
uint16_t dcr0 = 0, ccr0 = 0;
int state = 0;
float iacc;
float rm = 0;
int rm_delay = 3;

void keyfob_handler(uint16_t handle, uint8_t len, const uint8_t *data)
{
	int i;
	int16_t mpu_data[7];

//	counter += len;
	counter++;

	gettimeofday(&t_new, NULL);
	time = t_new.tv_sec - t_old.tv_sec;
	time += (t_new.tv_usec - t_old.tv_usec) / 1000000.0;
	if (time >= 1.0) {
//		speed = (counter << 3) / time;
		speed = counter / time;
		counter = 0;
		t_old = t_new;
	}

	for (i = 0; i < 7; i++) {
		mpu_data[i] = data[i << 1] << 8;
		mpu_data[i] |= data[(i << 1) + 1];
	}

	printf("[0x%04x] accelX: %6d accelY: %6d accelZ: %6d temp: %6d gyroX: %6d gyroY: %6d gyroZ: %6d pps: %.1f\n", handle,
														mpu_data[0],
														mpu_data[1],
														mpu_data[2],
														mpu_data[3] / 340 + 35,
														mpu_data[4],
														mpu_data[5],
														mpu_data[6], speed);
}

int main(int argc, char **argv)
{
	uint16_t bat_level;

	printf("connecting to %s\n", keyfob_addr);

	lble_connect(dev_addr);

	if (lble_get_state() != STATE_CONNECTED) {
		fprintf(stderr, "error: connection failed\n");
		return -1;
	}

	printf("connection successful\n");


// wait for connection parameters update
	printf("waiting for connection update...\n");
	sleep(1);

/*
 * 0 - +-2g
 * 1 - +-4g
 * 2 - +-8g
 * 3 - +-16g
 */
	printf("setting accel range...\n");
	lble_write(0x0025, 1, (uint8_t *)"\x03");
/*
 * 0 - +-250 deg/s
 * 1 - +-500 deg/s
 * 2 - +-1000 deg/s
 * 3 - +-2000 deg/s
 */
	printf("setting gyro range...\n");
	lble_write(0x0028, 1, (uint8_t *)"\x03");

//	printf("enabling notifications on mpu6000 data...\n");
//	lble_write(0x002f, 2, (uint8_t *)"\x01\x00");

	printf("wake up MPU6000...\n");
	lble_write(0x002b, 1, &pps);

	while (1 ) {
		lble_read(0x33, (uint8_t *)&bat_level);

		switch (state) {
			case 0:
				voltage = bat_level;
				break;
			case 1:
				dcr = bat_level;
				break;
			case 2:
				ccr = bat_level;
				break;
		}


		iacc = ((ccr - ccr0) - (dcr - dcr0)) / (91.0 * 0.1);
		rm += iacc;
		if (rm_delay) {
			rm_delay--;
			rm = 0;
		}

		printf("voltage: %d\tDCR: %d\tCCR: %d\tIACC: %.3f\tRM: %.3f\n", voltage, dcr, ccr, iacc, rm);

		ccr0 = ccr;
		dcr0 = dcr;

		state++;
		state %= 3;
		sleep(1);
	}

	printf("listening for notifications\n");
	lble_listen(keyfob_handler);

	lble_disconnect();
	return 0;
}


