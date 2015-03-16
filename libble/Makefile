BLUEZ_PATH=../bluez
BLUEZ_SRCS  = lib/bluetooth.c lib/hci.c lib/sdp.c lib/uuid.c
BLUEZ_SRCS += attrib/att.c attrib/gatt.c attrib/gattrib.c attrib/utils.c
BLUEZ_SRCS += btio/btio.c
BLUEZ_SRCS += src/log.c
BLUEZ_SRCS += src/shared/att.c src/shared/crypto.c src/shared/queue.c src/shared/timeout-glib.c src/shared/io-glib.c src/shared/util.c
IMPORT_SRCS = $(addprefix $(BLUEZ_PATH)/, $(BLUEZ_SRCS))

LIB_SRCS = lib/libble.c

CC = gcc
CFLAGS =

CPPFLAGS = -DVERSION=\"libble-test\"
CPPFLAGS += -I$(BLUEZ_PATH)/attrib -I$(BLUEZ_PATH) -I$(BLUEZ_PATH)/lib -I$(BLUEZ_PATH)/src -I$(BLUEZ_PATH)/gdbus -I$(BLUEZ_PATH)/btio

CPPFLAGS += `pkg-config glib-2.0 --cflags`
LDLIBS += `pkg-config glib-2.0 --libs`

all: libble.so motion  battery button beep 

libble.so: $(LIB_SRCS) $(IMPORT_SRCS)
	$(CC) -shared -L. $(CFLAGS) $(CPPFLAGS) -fPIC -o $@ $(LIB_SRCS) $(IMPORT_SRCS) $(LDLIBS)

motion: src/motion.c
	$(CC) -L. -Wl,-rpath=. -Wall -I./lib -o $@ $< -lble

battery: src/battery.c
	$(CC) -L. -Wl,-rpath=. -Wall -I./lib -o $@ $< -lble

button: src/button.c
	$(CC) -L. -Wl,-rpath=. -Wall -I./lib -o $@ $< -lble

beep: src/beep.c
	$(CC) -L. -Wl,-rpath=. -Wall -I./lib -o $@ $< -lble

clean:
	rm -f libble.so motion battery button beep

