/*
 *
 *  libble - library for accessing Bluetooth Low-Energy devices over GATT level
 *
 *  Copyright (C) 2014  ICS-Micont
 *  Copyright (C) 2014  Dmitry Nikolaev <borealis@permlug.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib.h>

#include "lib/uuid.h"
#include "btio/btio.h"
#include "att.h"
#include "gattrib.h"
#include "gatt.h"
#include "gatttool.h"
#include "shared/util.h"

#include "libble.h"

static GMainLoop *event_loop;
static GIOChannel *iochannel = NULL;
static GAttrib *attrib = NULL;

static char *opt_dst = NULL;
static char *opt_dst_type = NULL;
static char *opt_sec_level = NULL;

static int opt_psm = 0;
static int opt_mtu = 0;

static enum state conn_state;

static void set_state(enum state st)
{
	conn_state = st;
}

static lble_event_handler lble_evh = NULL;

static void events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data)
{
	if (pdu[0] != ATT_OP_HANDLE_NOTIFY)
		return;

// calling out hi-level handler
	if (lble_evh != NULL)
		lble_evh(get_le16(&pdu[1]), len - 3, &pdu[3]);
}

static void connect_cb(GIOChannel *io, GError *err, gpointer user_data)
{
	uint16_t mtu, cid;

	if (err) {
		set_state(STATE_DISCONNECTED);
		fprintf(stderr, "LBLE::ERROR %s\n", err->message);
		goto done;
	}

	bt_io_get(io, &err, BT_IO_OPT_IMTU, &mtu, BT_IO_OPT_CID, &cid, BT_IO_OPT_INVALID);
	if (err) {
		fprintf(stderr, "LBLE::ERROR %s\n", err->message);
		g_error_free(err);
		mtu = ATT_DEFAULT_LE_MTU;
	}

	if (cid == ATT_CID)
		mtu = ATT_DEFAULT_LE_MTU;

	attrib = g_attrib_new(io, mtu);
	set_state(STATE_CONNECTED);
done:
	g_main_loop_quit(event_loop);
}

static gboolean channel_watcher(GIOChannel *ch, GIOCondition cond, gpointer user_data)
{
	lble_disconnect();
	return FALSE;
}


/* exported functions */

/* connect to BLE device */
void lble_connect(const char *addr)
{
	GError *gerr = NULL;

	if (conn_state != STATE_DISCONNECTED)
		return;

	opt_sec_level = g_strdup("low");
	opt_dst_type = g_strdup("public");
	opt_dst = g_strdup(addr);
	event_loop = g_main_loop_new(NULL, FALSE);

	set_state(STATE_CONNECTING);

	iochannel = gatt_connect(NULL, opt_dst, opt_dst_type, opt_sec_level, opt_psm, opt_mtu, connect_cb, &gerr);
	if (iochannel == NULL) {
		set_state(STATE_DISCONNECTED);
		fprintf(stderr, "LBLE::ERROR %s\n", gerr->message);
		g_error_free(gerr);
	} else {
		g_io_add_watch(iochannel, G_IO_HUP, channel_watcher, NULL);
	}

	g_main_loop_run(event_loop);
}

/* listen for notifications/indications */
void lble_listen(lble_event_handler handler)
{
	if (conn_state != STATE_CONNECTED)
		return;

	g_attrib_register(attrib, ATT_OP_HANDLE_NOTIFY, GATTRIB_ALL_HANDLES, events_handler, attrib, NULL);
//	g_attrib_register(attrib, ATT_OP_HANDLE_IND, GATTRIB_ALL_HANDLES, events_handler, attrib, NULL);

	// setting our own notify/ind high-level handler
	lble_evh = handler;

	g_main_loop_run(event_loop);
}

/* disconnect from BLE device */
void lble_disconnect(void)
{
	if (conn_state == STATE_DISCONNECTED)
		return;

	if (conn_state == STATE_CONNECTED) {
		g_attrib_unref(attrib);
		attrib = NULL;
		opt_mtu = 0;
	}

	g_io_channel_shutdown(iochannel, FALSE, NULL);
	g_io_channel_unref(iochannel);
	iochannel = NULL;

	g_main_loop_unref(event_loop);
	event_loop = NULL;
	g_free(opt_sec_level);
	opt_sec_level = NULL;
	g_free(opt_dst_type);
	opt_dst_type = NULL;
	g_free(opt_dst);
	opt_dst = NULL;

	set_state(STATE_DISCONNECTED);
}

/* get connection state */
enum state lble_get_state(void)
{
	return conn_state;
}

struct cb_buffer {
	uint8_t len;
	uint8_t *buffer;
};

static void char_read_cb(guint8 status, const guint8 *pdu, guint16 plen, gpointer user_data)
{
	struct cb_buffer *pbuf = (struct cb_buffer *)user_data;

	if (status != 0) {
		fprintf(stderr, "LBLE::ERROR value read failed: %s\n", att_ecode2str(status));
		pbuf->len = 0;
		return;
	}

	pbuf->len = dec_read_resp(pdu, plen, pbuf->buffer, plen);

	g_main_loop_quit(event_loop);
}

/* read characteristic by handle */
uint8_t lble_read(uint16_t handle, uint8_t *data)
{
	struct cb_buffer buf;

	buf.len = 0;
	buf.buffer = data;

	if (conn_state != STATE_CONNECTED)
		return;

	gatt_read_char(attrib, handle, char_read_cb, &buf);

	g_main_loop_run(event_loop);

	return buf.len;
}

/* write characteristic by handle */
void lble_write(uint16_t handle, uint8_t len, uint8_t *data)
{
	if (conn_state != STATE_CONNECTED)
		return;

	if (data != NULL && len > 0)
		gatt_write_cmd(attrib, handle, data, len, NULL, NULL);
}

