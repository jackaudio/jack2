/*
 * ALSA SEQ < - > JACK MIDI bridge
 *
 * Copyright (c) 2006,2007 Dmitry S. Baikov <c0ff@konstruktiv.org>
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

/*
 * alsa_seqmidi_read:
 *	add new ports
 * 	reads queued snd_seq_event's
 * 	if PORT_EXIT: mark port as dead
 * 	if PORT_ADD, PORT_CHANGE: send addr to port_thread (it also may mark port as dead)
 * 	else process input event
 * 	remove dead ports and send them to port_thread
 *
 * alsa_seqmidi_write:
 * 	remove dead ports and send them to port_thread
 * 	add new ports
 * 	queue output events
 *
 * port_thread:
 * 	wait for port_sem
 * 	free deleted ports
 * 	create new ports or mark existing as dead
 */

#include <alsa/asoundlib.h>

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>

#include "midiport.h"
#include "ringbuffer.h"
#include "alsa_midi_impl.h"
#include "JackError.h"

#define NSEC_PER_SEC ((int64_t)1000*1000*1000)

enum {
	MAX_PORTS = 64,
	MAX_EVENT_SIZE = 1024,
};

typedef struct port_t port_t;

enum {
	PORT_HASH_BITS = 4,
	PORT_HASH_SIZE = 1 << PORT_HASH_BITS
};

typedef port_t* port_hash_t[PORT_HASH_SIZE];

struct port_t {
	port_t *next;
	int is_dead;
	char name[64];
	snd_seq_addr_t remote;
	jack_port_t *jack_port;

	jack_ringbuffer_t *early_events; // alsa_midi_event_t + data
	int64_t last_out_time;

	void *jack_buf;
};

typedef struct {
	snd_midi_event_t *codec;

	jack_ringbuffer_t *new_ports;

	port_t *ports[MAX_PORTS];
} stream_t;

typedef struct alsa_seqmidi {
	alsa_midi_t ops;
	jack_client_t *jack;

	snd_seq_t *seq;
	int client_id;
	int port_id;
	int queue;

	int keep_walking;

	pthread_t port_thread;
	sem_t port_sem;
	jack_ringbuffer_t *port_add; // snd_seq_addr_t
	jack_ringbuffer_t *port_del; // port_t*

	stream_t stream[2];

	char alsa_name[32];
	int midi_in_cnt;
	int midi_out_cnt;
} alsa_seqmidi_t;

struct alsa_midi_event {
	int64_t time;
	int size;
};
typedef struct alsa_midi_event alsa_midi_event_t;

struct process_info {
	int dir;
	jack_nframes_t nframes;
	jack_nframes_t period_start;
	jack_nframes_t sample_rate;
	jack_nframes_t cur_frames;
	int64_t alsa_time;
};

enum PortType { PORT_INPUT = 0, PORT_OUTPUT = 1 };

typedef void (*port_jack_func)(alsa_seqmidi_t *self, port_t *port,struct process_info* info);
static void do_jack_input(alsa_seqmidi_t *self, port_t *port, struct process_info* info);
static void do_jack_output(alsa_seqmidi_t *self, port_t *port, struct process_info* info);

typedef struct {
	int alsa_mask;
	int jack_caps;
	char name[9];
	port_jack_func jack_func;
} port_type_t;

static port_type_t port_type[2] = {
	{
		SND_SEQ_PORT_CAP_SUBS_READ,
		JackPortIsOutput,
		"playback",
		do_jack_input
	},
	{
		SND_SEQ_PORT_CAP_SUBS_WRITE,
		JackPortIsInput,
		"capture",
		do_jack_output
	}
};

static void alsa_seqmidi_delete(alsa_midi_t *m);
static int alsa_seqmidi_attach(alsa_midi_t *m);
static int alsa_seqmidi_detach(alsa_midi_t *m);
static int alsa_seqmidi_start(alsa_midi_t *m);
static int alsa_seqmidi_stop(alsa_midi_t *m);
static void alsa_seqmidi_read(alsa_midi_t *m, jack_nframes_t nframes);
static void alsa_seqmidi_write(alsa_midi_t *m, jack_nframes_t nframes);

static
void stream_init(alsa_seqmidi_t *self, int dir)
{
	stream_t *str = &self->stream[dir];

	str->new_ports = jack_ringbuffer_create(MAX_PORTS*sizeof(port_t*));
	snd_midi_event_new(MAX_EVENT_SIZE, &str->codec);
}

static void port_free(alsa_seqmidi_t *self, port_t *port);
static void free_ports(alsa_seqmidi_t *self, jack_ringbuffer_t *ports);

static
void stream_attach(alsa_seqmidi_t *self, int dir)
{
}

static
void stream_detach(alsa_seqmidi_t *self, int dir)
{
	stream_t *str = &self->stream[dir];
	int i;

	free_ports(self, str->new_ports);

	// delete all ports from hash
	for (i=0; i<PORT_HASH_SIZE; ++i) {
		port_t *port = str->ports[i];
		while (port) {
			port_t *next = port->next;
			port_free(self, port);
			port = next;
		}
		str->ports[i] = NULL;
	}
}

static
void stream_close(alsa_seqmidi_t *self, int dir)
{
	stream_t *str = &self->stream[dir];

	if (str->codec)
		snd_midi_event_free(str->codec);
	if (str->new_ports)
		jack_ringbuffer_free(str->new_ports);
}

alsa_midi_t* alsa_seqmidi_new(jack_client_t *client, const char* alsa_name)
{
	alsa_seqmidi_t *self = calloc(1, sizeof(alsa_seqmidi_t));
	debug_log("midi: new");
	if (!self)
		return NULL;
	self->jack = client;
	if (!alsa_name)
		alsa_name = "jack_midi";
	snprintf(self->alsa_name, sizeof(self->alsa_name), "%s", alsa_name);

	self->port_add = jack_ringbuffer_create(2*MAX_PORTS*sizeof(snd_seq_addr_t));
	self->port_del = jack_ringbuffer_create(2*MAX_PORTS*sizeof(port_t*));
	sem_init(&self->port_sem, 0, 0);

	stream_init(self, PORT_INPUT);
	stream_init(self, PORT_OUTPUT);

	self->midi_in_cnt = 0;
	self->midi_out_cnt = 0;
	self->ops.destroy = alsa_seqmidi_delete;
	self->ops.attach = alsa_seqmidi_attach;
	self->ops.detach = alsa_seqmidi_detach;
	self->ops.start = alsa_seqmidi_start;
	self->ops.stop = alsa_seqmidi_stop;
	self->ops.read = alsa_seqmidi_read;
	self->ops.write = alsa_seqmidi_write;
	return &self->ops;
}

static
void alsa_seqmidi_delete(alsa_midi_t *m)
{
	alsa_seqmidi_t *self = (alsa_seqmidi_t*) m;

	debug_log("midi: delete");
	alsa_seqmidi_detach(m);

	stream_close(self, PORT_OUTPUT);
	stream_close(self, PORT_INPUT);

	jack_ringbuffer_free(self->port_add);
	jack_ringbuffer_free(self->port_del);
	sem_close(&self->port_sem);

	free(self);
}

static
int alsa_seqmidi_attach(alsa_midi_t *m)
{
	alsa_seqmidi_t *self = (alsa_seqmidi_t*) m;
	int err;

	debug_log("midi: attach");

	if (self->seq)
		return -EALREADY;

	if ((err = snd_seq_open(&self->seq, "hw", SND_SEQ_OPEN_DUPLEX, 0)) < 0) {
		error_log("failed to open alsa seq");
		return err;
	}
	snd_seq_set_client_name(self->seq, self->alsa_name);
	self->port_id = snd_seq_create_simple_port(self->seq, "port",
		SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_WRITE
#ifndef JACK_MIDI_DEBUG
		|SND_SEQ_PORT_CAP_NO_EXPORT
#endif
		,SND_SEQ_PORT_TYPE_APPLICATION);
	self->client_id = snd_seq_client_id(self->seq);

  	self->queue = snd_seq_alloc_queue(self->seq);
  	snd_seq_start_queue(self->seq, self->queue, 0);

	stream_attach(self, PORT_INPUT);
	stream_attach(self, PORT_OUTPUT);

	snd_seq_nonblock(self->seq, 1);

	return 0;
}

static
int alsa_seqmidi_detach(alsa_midi_t *m)
{
	alsa_seqmidi_t *self = (alsa_seqmidi_t*) m;

	debug_log("midi: detach");

	if (!self->seq)
		return -EALREADY;

	alsa_seqmidi_stop(m);

	jack_ringbuffer_reset(self->port_add);
	free_ports(self, self->port_del);

	stream_detach(self, PORT_INPUT);
	stream_detach(self, PORT_OUTPUT);

	snd_seq_close(self->seq);
	self->seq = NULL;

	return 0;
}

static void* port_thread(void *);

static void add_existing_ports(alsa_seqmidi_t *self);
static void update_ports(alsa_seqmidi_t *self);
static void add_ports(stream_t *str);

static
int alsa_seqmidi_start(alsa_midi_t *m)
{
	alsa_seqmidi_t *self = (alsa_seqmidi_t*) m;
	int err;

	debug_log("midi: start");

	if (!self->seq)
		return -EBADF;

	if (self->keep_walking)
		return -EALREADY;

	snd_seq_connect_from(self->seq, self->port_id, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE);
	snd_seq_drop_input(self->seq);

	add_existing_ports(self);
	update_ports(self);
	add_ports(&self->stream[PORT_INPUT]);
	add_ports(&self->stream[PORT_OUTPUT]);

	self->keep_walking = 1;

	if ((err = pthread_create(&self->port_thread, NULL, port_thread, self))) {
		self->keep_walking = 0;
		return -errno;
	}

	return 0;
}

static
int alsa_seqmidi_stop(alsa_midi_t *m)
{
	alsa_seqmidi_t *self = (alsa_seqmidi_t*) m;

	debug_log("midi: stop");

	if (!self->keep_walking)
		return -EALREADY;

	snd_seq_disconnect_from(self->seq, self->port_id, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE);

	self->keep_walking = 0;

	sem_post(&self->port_sem);
	pthread_join(self->port_thread, NULL);
	self->port_thread = 0;

	return 0;
}

static
int alsa_connect_from(alsa_seqmidi_t *self, int client, int port)
{
	snd_seq_port_subscribe_t* sub;
	snd_seq_addr_t seq_addr;
	int err;

	snd_seq_port_subscribe_alloca(&sub);
	seq_addr.client = client;
	seq_addr.port = port;
	snd_seq_port_subscribe_set_sender(sub, &seq_addr);
	seq_addr.client = self->client_id;
	seq_addr.port = self->port_id;
	snd_seq_port_subscribe_set_dest(sub, &seq_addr);

	snd_seq_port_subscribe_set_time_update(sub, 1);
	snd_seq_port_subscribe_set_queue(sub, self->queue);
	snd_seq_port_subscribe_set_time_real(sub, 1);

	if ((err=snd_seq_subscribe_port(self->seq, sub)))
		error_log("can't subscribe to %d:%d - %s", client, port, snd_strerror(err));
	return err;
}

/*
 * ==================== Port routines =============================
 */
static inline
int port_hash(snd_seq_addr_t addr)
{
	return (addr.client + addr.port) % PORT_HASH_SIZE;
}

static
port_t* port_get(port_hash_t hash, snd_seq_addr_t addr)
{
	port_t **pport = &hash[port_hash(addr)];
	while (*pport) {
		port_t *port = *pport;
		if (port->remote.client == addr.client && port->remote.port == addr.port)
			return port;
		pport = &port->next;
	}
	return NULL;
}

static
void port_insert(port_hash_t hash, port_t *port)
{
	port_t **pport = &hash[port_hash(port->remote)];
	port->next = *pport;
	*pport = port;
}

static
void port_setdead(port_hash_t hash, snd_seq_addr_t addr)
{
	port_t *port = port_get(hash, addr);
	if (port)
		port->is_dead = 1; // see jack_process
	else {
		debug_log("port_setdead: not found (%d:%d)", addr.client, addr.port);
	}
}

static
void port_free(alsa_seqmidi_t *self, port_t *port)
{
	//snd_seq_disconnect_from(self->seq, self->port_id, port->remote.client, port->remote.port);
	//snd_seq_disconnect_to(self->seq, self->port_id, port->remote.client, port->remote.port);
	if (port->early_events)
		jack_ringbuffer_free(port->early_events);
	if (port->jack_port)
		jack_port_unregister(self->jack, port->jack_port);
	info_log("port deleted: %s", port->name);

	free(port);
}

static
port_t* port_create(alsa_seqmidi_t *self, int type, snd_seq_addr_t addr, const snd_seq_port_info_t *info)
{
	snd_seq_client_info_t* client_info;
	port_t *port;
	char *c;
	int err;
	int jack_caps;
        char name[128];

	port = calloc(1, sizeof(port_t));
	if (!port)
		return NULL;

	port->remote = addr;

	snd_seq_client_info_alloca (&client_info);
	snd_seq_get_any_client_info (self->seq, addr.client, client_info);

	snprintf(port->name, sizeof(port->name), "alsa_pcm:%s/midi_%s_%d",
		 snd_seq_client_info_get_name(client_info), port_type[type].name, addr.port+1);

	// replace all offending characters by -
	for (c = port->name; *c; ++c)
		if (!isalnum(*c) && *c != '/' && *c != '_' && *c != ':' && *c != '(' && *c != ')')
			*c = '-';

	jack_caps = port_type[type].jack_caps;

	/* mark anything that looks like a hardware port as physical&terminal */

	if (snd_seq_port_info_get_type (info) & (SND_SEQ_PORT_TYPE_HARDWARE|SND_SEQ_PORT_TYPE_PORT|SND_SEQ_PORT_TYPE_SPECIFIC)) {
		jack_caps |= (JackPortIsPhysical | JackPortIsTerminal);
	}

	if (jack_caps & JackPortIsOutput)
		snprintf(name, sizeof(name), "system:midi_capture_%d", ++self->midi_in_cnt);
	else
		snprintf(name, sizeof(name), "system:midi_playback_%d", ++self->midi_out_cnt);

	port->jack_port = jack_port_register(self->jack,
		name, JACK_DEFAULT_MIDI_TYPE, jack_caps, 0);
	if (!port->jack_port)
		goto failed;

	jack_port_set_alias (port->jack_port, port->name);

	/* generate an alias */

	snprintf(port->name, sizeof(port->name), "%s:midi/%s_%d",
		 snd_seq_client_info_get_name (client_info), port_type[type].name, addr.port+1);

	// replace all offending characters by -
	for (c = port->name; *c; ++c)
		if (!isalnum(*c) && *c != '/' && *c != '_' && *c != ':' && *c != '(' && *c != ')')
			*c = '-';

	jack_port_set_alias (port->jack_port, port->name);

	if (type == PORT_INPUT)
		err = alsa_connect_from(self, port->remote.client, port->remote.port);
	else
		err = snd_seq_connect_to(self->seq, self->port_id, port->remote.client, port->remote.port);
	if (err)
		goto failed;

	port->early_events = jack_ringbuffer_create(MAX_EVENT_SIZE*16);

	info_log("port created: %s", port->name);
	return port;

 failed:
 	port_free(self, port);
	return NULL;
}

/*
 * ==================== Port add/del handling thread ==============================
 */
static
void update_port_type(alsa_seqmidi_t *self, int type, snd_seq_addr_t addr, int caps, const snd_seq_port_info_t *info)
{
	stream_t *str = &self->stream[type];
	int alsa_mask = port_type[type].alsa_mask;
	port_t *port = port_get(str->ports, addr);

	debug_log("update_port_type(%d:%d)", addr.client, addr.port);

	if (port && (caps & alsa_mask)!=alsa_mask) {
		debug_log("setdead: %s", port->name);
		port->is_dead = 1;
	}

	if (!port && (caps & alsa_mask)==alsa_mask) {
		assert (jack_ringbuffer_write_space(str->new_ports) >= sizeof(port));
		port = port_create(self, type, addr, info);
		if (port)
			jack_ringbuffer_write(str->new_ports, (char*)&port, sizeof(port));
	}
}

static
void update_port(alsa_seqmidi_t *self, snd_seq_addr_t addr, const snd_seq_port_info_t *info)
{
	unsigned int port_caps = snd_seq_port_info_get_capability(info);
	if (port_caps & SND_SEQ_PORT_CAP_NO_EXPORT)
		return;
	update_port_type(self, PORT_INPUT, addr, port_caps, info);
	update_port_type(self, PORT_OUTPUT,addr, port_caps, info);
}

static
void free_ports(alsa_seqmidi_t *self, jack_ringbuffer_t *ports)
{
	port_t *port;
	int sz;
	while ((sz = jack_ringbuffer_read(ports, (char*)&port, sizeof(port)))) {
		assert (sz == sizeof(port));
		port_free(self, port);
	}
}

static
void update_ports(alsa_seqmidi_t *self)
{
	snd_seq_addr_t addr;
	snd_seq_port_info_t *info;
	int size;

	snd_seq_port_info_alloca(&info);

	while ((size = jack_ringbuffer_read(self->port_add, (char*)&addr, sizeof(addr)))) {

		int err;

		assert (size == sizeof(addr));
		assert (addr.client != self->client_id);
		if ((err=snd_seq_get_any_port_info(self->seq, addr.client, addr.port, info))>=0) {
			update_port(self, addr, info);
		} else {
			//port_setdead(self->stream[PORT_INPUT].ports, addr);
			//port_setdead(self->stream[PORT_OUTPUT].ports, addr);
		}
	}
}

static
void* port_thread(void *arg)
{
	alsa_seqmidi_t *self = arg;

	while (self->keep_walking) {
		sem_wait(&self->port_sem);
		free_ports(self, self->port_del);
		update_ports(self);
	}
	debug_log("port_thread exited");
	return NULL;
}

static
void add_existing_ports(alsa_seqmidi_t *self)
{
	snd_seq_addr_t addr;
	snd_seq_client_info_t *client_info;
	snd_seq_port_info_t *port_info;

	snd_seq_client_info_alloca(&client_info);
	snd_seq_port_info_alloca(&port_info);
	snd_seq_client_info_set_client(client_info, -1);
	while (snd_seq_query_next_client(self->seq, client_info) >= 0)
	{
		addr.client = snd_seq_client_info_get_client(client_info);
		if (addr.client == SND_SEQ_CLIENT_SYSTEM || addr.client == self->client_id)
			continue;
		snd_seq_port_info_set_client(port_info, addr.client);
		snd_seq_port_info_set_port(port_info, -1);
		while (snd_seq_query_next_port(self->seq, port_info) >= 0)
		{
			addr.port = snd_seq_port_info_get_port(port_info);
			update_port(self, addr, port_info);
		}
	}
}

/*
 * =================== Input/output port handling =========================
 */
static
void set_process_info(struct process_info *info, alsa_seqmidi_t *self, int dir, jack_nframes_t nframes)
{
	const snd_seq_real_time_t* alsa_time;
	snd_seq_queue_status_t *status;

	snd_seq_queue_status_alloca(&status);

	info->dir = dir;

	info->period_start = jack_last_frame_time(self->jack);
	info->nframes = nframes;
	info->sample_rate = jack_get_sample_rate(self->jack);

	info->cur_frames = jack_frame_time(self->jack);

	// immediately get alsa'a real time (uhh, why everybody has their own 'real' time)
	snd_seq_get_queue_status(self->seq, self->queue, status);
	alsa_time = snd_seq_queue_status_get_real_time(status);
	info->alsa_time = alsa_time->tv_sec * NSEC_PER_SEC + alsa_time->tv_nsec;

	if (info->period_start + info->nframes < info->cur_frames) {
		int periods_lost = (info->cur_frames - info->period_start) / info->nframes;
		info->period_start += periods_lost * info->nframes;
		debug_log("xrun detected: %d periods lost\n", periods_lost);
	}
}

static
void add_ports(stream_t *str)
{
	port_t *port;
	while (jack_ringbuffer_read(str->new_ports, (char*)&port, sizeof(port))) {
		debug_log("jack: inserted port %s\n", port->name);
		port_insert(str->ports, port);
	}
}

static
void jack_process(alsa_seqmidi_t *self, struct process_info *info)
{
	stream_t *str = &self->stream[info->dir];
	port_jack_func process = port_type[info->dir].jack_func;
	int i, del=0;

	add_ports(str);

	// process ports
	for (i=0; i<PORT_HASH_SIZE; ++i) {
		port_t **pport = &str->ports[i];
		while (*pport) {
			port_t *port = *pport;
			port->jack_buf = jack_port_get_buffer(port->jack_port, info->nframes);
			if (info->dir == PORT_INPUT)
				jack_midi_clear_buffer(port->jack_buf);

			if (!port->is_dead)
				(*process)(self, port, info);
			else if (jack_ringbuffer_write_space(self->port_del) >= sizeof(port)) {
				debug_log("jack: removed port %s", port->name);
				*pport = port->next;
				jack_ringbuffer_write(self->port_del, (char*)&port, sizeof(port));
				del++;
				continue;
			}

			pport = &port->next;
		}
	}

	if (del)
		sem_post(&self->port_sem);
}

/*
 * ============================ Input ==============================
 */
static
void do_jack_input(alsa_seqmidi_t *self, port_t *port, struct process_info *info)
{
	// process port->early_events
	alsa_midi_event_t ev;
	while (jack_ringbuffer_read(port->early_events, (char*)&ev, sizeof(ev))) {
		jack_midi_data_t* buf;
		int64_t time = ev.time - info->period_start;
		if (time < 0)
			time = 0;
		else if (time >= info->nframes)
			time = info->nframes - 1;
		buf = jack_midi_event_reserve(port->jack_buf, (jack_nframes_t)time, ev.size);
		if (buf)
			jack_ringbuffer_read(port->early_events, (char*)buf, ev.size);
		else
			jack_ringbuffer_read_advance(port->early_events, ev.size);
		debug_log("input: it's time for %d bytes at %lld", ev.size, time);
	}
}

static
void port_event(alsa_seqmidi_t *self, snd_seq_event_t *ev)
{
	const snd_seq_addr_t addr = ev->data.addr;

	if (addr.client == self->client_id)
		return;

	if (ev->type == SND_SEQ_EVENT_PORT_START || ev->type == SND_SEQ_EVENT_PORT_CHANGE) {
		assert (jack_ringbuffer_write_space(self->port_add) >= sizeof(addr));

		debug_log("port_event: add/change %d:%d", addr.client, addr.port);
		jack_ringbuffer_write(self->port_add, (char*)&addr, sizeof(addr));
		sem_post(&self->port_sem);
	} else if (ev->type == SND_SEQ_EVENT_PORT_EXIT) {
		debug_log("port_event: del %d:%d", addr.client, addr.port);
		port_setdead(self->stream[PORT_INPUT].ports, addr);
		port_setdead(self->stream[PORT_OUTPUT].ports, addr);
	}
}

static
void input_event(alsa_seqmidi_t *self, snd_seq_event_t *alsa_event, struct process_info* info)
{
	jack_midi_data_t data[MAX_EVENT_SIZE];
	stream_t *str = &self->stream[PORT_INPUT];
	long size;
	int64_t alsa_time, time_offset;
	int64_t frame_offset, event_frame;
	port_t *port;

	port = port_get(str->ports, alsa_event->source);
	if (!port)
		return;

	/*
	 * RPNs, NRPNs, Bank Change, etc. need special handling
	 * but seems, ALSA does it for us already.
	 */
	snd_midi_event_reset_decode(str->codec);
	if ((size = snd_midi_event_decode(str->codec, data, sizeof(data), alsa_event))<0)
		return;

	// fixup NoteOn with vel 0
	if ((data[0] & 0xF0) == 0x90 && data[2] == 0x00) {
		data[0] = 0x80 + (data[0] & 0x0F);
		data[2] = 0x40;
	}

	alsa_time = alsa_event->time.time.tv_sec * NSEC_PER_SEC + alsa_event->time.time.tv_nsec;
	time_offset = info->alsa_time - alsa_time;
	frame_offset = (info->sample_rate * time_offset) / NSEC_PER_SEC;
	event_frame = (int64_t)info->cur_frames - info->period_start - frame_offset + info->nframes;

	debug_log("input: %d bytes at event_frame = %d", (int)size, (int)event_frame);

	if (event_frame >= info->nframes &&
	    jack_ringbuffer_write_space(port->early_events) >= (sizeof(alsa_midi_event_t) + size)) {
		alsa_midi_event_t ev;
		ev.time = event_frame + info->period_start;
		ev.size = size;
		jack_ringbuffer_write(port->early_events, (char*)&ev, sizeof(ev));
		jack_ringbuffer_write(port->early_events, (char*)data, size);
		debug_log("postponed to next frame +%d", (int) (event_frame - info->nframes));
		return;
	}

	if (event_frame < 0)
		event_frame = 0;
	else if (event_frame >= info->nframes)
		event_frame = info->nframes - 1;

	jack_midi_event_write(port->jack_buf, event_frame, data, size);
}

static
void alsa_seqmidi_read(alsa_midi_t *m, jack_nframes_t nframes)
{
	alsa_seqmidi_t *self = (alsa_seqmidi_t*) m;
	int res;
	snd_seq_event_t *event;
	struct process_info info;

	if (!self->keep_walking)
		return;

	set_process_info(&info, self, PORT_INPUT, nframes);
	jack_process(self, &info);

	while ((res = snd_seq_event_input(self->seq, &event))>0) {
		if (event->source.client == SND_SEQ_CLIENT_SYSTEM)
			port_event(self, event);
		else
			input_event(self, event, &info);
	}
}

/*
 * ============================ Output ==============================
 */

static
void do_jack_output(alsa_seqmidi_t *self, port_t *port, struct process_info* info)
{
	stream_t *str = &self->stream[info->dir];
	int nevents = jack_midi_get_event_count(port->jack_buf);
	int i;
	for (i=0; i<nevents; ++i) {
		jack_midi_event_t jack_event;
		snd_seq_event_t alsa_event;
		int64_t frame_offset;
		int64_t out_time;
		snd_seq_real_time_t out_rt;
		int err;

		jack_midi_event_get(&jack_event, port->jack_buf, i);

		snd_seq_ev_clear(&alsa_event);
		snd_midi_event_reset_encode(str->codec);
		if (!snd_midi_event_encode(str->codec, jack_event.buffer, jack_event.size, &alsa_event))
			continue; // invalid event

		snd_seq_ev_set_source(&alsa_event, self->port_id);
		snd_seq_ev_set_dest(&alsa_event, port->remote.client, port->remote.port);

		/* NOTE: in case of xrun it could become negative, so it is essential to use signed type! */
		frame_offset = (int64_t)jack_event.time + info->period_start + info->nframes - info->cur_frames;
		if (frame_offset < 0) {
			frame_offset = info->nframes + jack_event.time;
			error_log("internal xrun detected: frame_offset = %"PRId64"\n", frame_offset);
		}
		/* Ken Ellinwood reported problems with this assert.
		 * Seems, magic 2 should be replaced with nperiods. */
		//FIXME: assert (frame_offset < info->nframes*2);
		//if (frame_offset < info->nframes * info->nperiods)
		//        debug_log("alsa_out: BLAH-BLAH-BLAH");

		out_time = info->alsa_time + (frame_offset * NSEC_PER_SEC) / info->sample_rate;

		debug_log("alsa_out: frame_offset = %lld, info->alsa_time = %lld, out_time = %lld, port->last_out_time = %lld",
			frame_offset, info->alsa_time, out_time, port->last_out_time);

		// we should use absolute time to prevent reordering caused by rounding errors
		if (out_time < port->last_out_time) {
			debug_log("alsa_out: limiting out_time %lld at %lld", out_time, port->last_out_time);
			out_time = port->last_out_time;
		} else
			port->last_out_time = out_time;

		out_rt.tv_nsec = out_time % NSEC_PER_SEC;
		out_rt.tv_sec = out_time / NSEC_PER_SEC;
		snd_seq_ev_schedule_real(&alsa_event, self->queue, 0, &out_rt);

		err = snd_seq_event_output(self->seq, &alsa_event);
		debug_log("alsa_out: written %d bytes to %s at %+d (%lld): %d", (int)jack_event.size, port->name, (int)frame_offset, out_time, err);
	}
}

static
void alsa_seqmidi_write(alsa_midi_t *m, jack_nframes_t nframes)
{
	alsa_seqmidi_t *self = (alsa_seqmidi_t*) m;
	struct process_info info;

	if (!self->keep_walking)
		return;

	set_process_info(&info, self, PORT_OUTPUT, nframes);
	jack_process(self, &info);
	snd_seq_drain_output(self->seq);
}
