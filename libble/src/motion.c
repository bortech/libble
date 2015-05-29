#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "libble.h"

// Адрес брелка VECS
static const char *dev_addr = "84:DD:20:F0:86:AB";

// Переменные для расчета частоты получения пакетов
uint32_t counter = 0;
double speed = 0;
double time = 0;
struct timeval t_old, t_new;

// Желаемая частота пакетов
uint8_t pps = 200;

// Обработчик приходящих уведомлений с данным датчиков
void notify_handler(uint16_t handle, uint8_t len, const uint8_t *data)
{
	int i;
	int16_t mpu_data[7];

	counter++;

	gettimeofday(&t_new, NULL);
	time = t_new.tv_sec - t_old.tv_sec;
	time += (t_new.tv_usec - t_old.tv_usec) / 1000000.0;
	if (time >= 1.0) {
		speed = counter / time;
		counter = 0;
		t_old = t_new;
	}

	for (i = 0; i < 7; i++) {
		mpu_data[i] = data[i << 1] << 8;
		mpu_data[i] |= data[(i << 1) + 1];
	}

	printf("[0x%04x] accelX: %6d accelY: %6d accelZ: %6d idx: %6d gyroX: %6d gyroY: %6d gyroZ: %6d PPS: %.0f Hz\n",
														handle,
														mpu_data[0],
														mpu_data[1],
														mpu_data[2],
														(uint16_t)mpu_data[3],
														mpu_data[4],
														mpu_data[5],
														mpu_data[6], 
														speed);
}

int main(int argc, char **argv)
{
	printf("connecting to %s\n", dev_addr);

	lble_connect(dev_addr);

	if (lble_get_state() != STATE_CONNECTED) {
		fprintf(stderr, "error: connection failed\n");
		return -1;
	}
	printf("connection successful\n");

// Включение сенсоров (вывод из спящего режима) и задание частоты опроса
	printf("wake up MPU6000 ans setting %d Hz data rate\n", pps);
	lble_write(0x002b, 1, &pps);

/*
 * Диапазон шкалы акселерометра
 * 0 - +-2g
 * 1 - +-4g
 * 2 - +-8g
 * 3 - +-16g
 */
	printf("setting accel range...\n");
	lble_write(0x0025, 1, (uint8_t *)"\x00");

/*
 * Диапазон шкалы гироскопа
 * 0 - +-250 deg/s
 * 1 - +-500 deg/s
 * 2 - +-1000 deg/s
 * 3 - +-2000 deg/s
 */
	printf("setting gyro range...\n");
	lble_write(0x0028, 1, (uint8_t *)"\x00");

// ожидание обновления параметров соединения, запрашиваемых брелком (1 секунда)
// ОБЯЗАТЕЛЬНО, если требуется получать данные на большой скорости 
	printf("waiting for connection update...\n");
	sleep(1);

// Включение уведомлений с данными датчиков
	printf("enabling notifications on mpu6000 data...\n");
	lble_write(0x002f, 2, (uint8_t *)"\x01\x00");

// Регистрируем свой обработчик приема уведомлений и запускаем цикл прослушки	
	printf("listening for notifications\n");
	lble_listen(notify_handler);

	lble_disconnect();
	return 0;
}


