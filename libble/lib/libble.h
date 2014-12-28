#ifndef _LIBBLE_H
#define _LIBBLE_H

#include <stdint.h>

enum state {
	STATE_DISCONNECTED,
	STATE_CONNECTING,
	STATE_CONNECTED
};

typedef void (*lble_event_handler)(uint16_t handle, uint8_t len, const uint8_t *data);

extern void lble_connect(const char *addr);
extern void lble_disconnect(void);

extern void lble_listen(lble_event_handler handler);

extern uint8_t lble_read(uint16_t handle, uint8_t *data);
extern void lble_write(uint16_t handle, uint8_t len, uint8_t *data);

extern enum state lble_get_state(void);

#endif

