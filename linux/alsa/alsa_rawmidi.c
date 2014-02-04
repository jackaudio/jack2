/*
 * ALSA RAWMIDI < - > JACK MIDI bridge
 *
 * Copyright (c) 2006,2007 Dmitry S. Baikov
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

/* Required for clock_nanosleep(). Thanks, Nedko */
#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <limits.h>
#include <ctype.h>
#include <alsa/asoundlib.h>

#include "ringbuffer.h"
#include "midiport.h"
#include "alsa_midi_impl.h"
#include "midi_pack.h"
#include "midi_unpack.h"
#include "JackError.h"

extern int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *req, struct timespec *rem);

enum {
	NANOSLEEP_RESOLUTION = 7000
};

#define NFRAMES_INF INT_MAX

enum {
#ifndef JACK_MIDI_DEBUG
	MAX_PFDS = 64,
	MAX_PORTS = MAX_PFDS-1,
	MAX_EVENTS = 4096,
	MAX_DATA = 64*1024,
	MIDI_THREAD_PRIO = 80
#else
	MAX_PFDS = 6,
	MAX_PORTS = MAX_PFDS-1,
	MAX_EVENTS = 16,
	MAX_DATA = 64,
	MIDI_THREAD_PRIO = 80
#endif
};

enum PortState {
	PORT_DESTROYED,
	PORT_CREATED,
	PORT_ADDED_TO_JACK,
	PORT_ADDED_TO_MIDI,
	PORT_REMOVED_FROM_MIDI,
	PORT_REMOVED_FROM_JACK,
	PORT_ZOMBIFIED,
};

typedef struct {
	int id[4]; //card, dev, dir, sub;
} alsa_id_t;

typedef struct {
	jack_time_t time;
	int size;
	int overruns;
} event_head_t;

typedef struct midi_port_t midi_port_t;
struct midi_port_t {
	midi_port_t *next;

	enum PortState state;

	alsa_id_t id;
	char dev[16];
	char name[64];

	jack_port_t *jack;
	snd_rawmidi_t *rawmidi;
	int npfds;
	int is_ready;

	jack_ringbuffer_t *event_ring;
	jack_ringbuffer_t *data_ring;

};

typedef struct input_port_t {
	midi_port_t base;

	// jack
	midi_unpack_t unpack;

	// midi
	int overruns;
} input_port_t;

typedef struct output_port_t {
	midi_port_t base;

	// jack
	midi_pack_t packer;

	// midi
	event_head_t next_event;
	int todo;
} output_port_t;

typedef struct alsa_rawmidi_t alsa_rawmidi_t;

typedef struct {
	alsa_rawmidi_t *midi;
	midi_port_t *port;
	void *buffer;
	jack_time_t frame_time;
	jack_nframes_t nframes;
} process_jack_t;

typedef struct {
	alsa_rawmidi_t *midi;
	int mode;
	midi_port_t *port;
	struct pollfd *rpfds;
	struct pollfd *wpfds;
	int max_pfds;
	jack_nframes_t cur_frames;
	jack_time_t cur_time;
	jack_time_t next_time;
} process_midi_t;

typedef struct midi_stream_t {
	alsa_rawmidi_t *owner;
	int mode;
	const char *name;
	pthread_t thread;
	int wake_pipe[2];

	struct {
		jack_ringbuffer_t *new_ports;
		int nports;
		midi_port_t *ports[MAX_PORTS];
	} jack, midi;

	size_t port_size;
	int (*port_init)(alsa_rawmidi_t *midi, midi_port_t *port);
	void (*port_close)(alsa_rawmidi_t *midi, midi_port_t *port);
	void (*process_jack)(process_jack_t *j);
	int (*process_midi)(process_midi_t *m);
} midi_stream_t;


struct alsa_rawmidi_t {
	alsa_midi_t ops;

	jack_client_t *client;
	int keep_walking;

	struct {
		pthread_t thread;
		midi_port_t *ports;
		int wake_pipe[2];
	} scan;

	midi_stream_t in;
	midi_stream_t out;
	int midi_in_cnt;
	int midi_out_cnt;
};

static int input_port_init(alsa_rawmidi_t *midi, midi_port_t *port);
static void input_port_close(alsa_rawmidi_t *midi, midi_port_t *port);

static void do_jack_input(process_jack_t *j);
static int do_midi_input(process_midi_t *m);

static int output_port_init(alsa_rawmidi_t *midi, midi_port_t *port);
static void output_port_close(alsa_rawmidi_t *midi, midi_port_t *port);

static void do_jack_output(process_jack_t *j);
static int do_midi_output(process_midi_t *m);

static
int stream_init(midi_stream_t *s, alsa_rawmidi_t *midi, const char *name)
{
	s->owner = midi;
	s->name = name;
	if (pipe(s->wake_pipe)==-1) {
		s->wake_pipe[0] = -1;
		error_log("pipe() in stream_init(%s) failed: %s", name, strerror(errno));
		return -errno;
	}
	s->jack.new_ports = jack_ringbuffer_create(sizeof(midi_port_t*)*MAX_PORTS);
	s->midi.new_ports = jack_ringbuffer_create(sizeof(midi_port_t*)*MAX_PORTS);
	if (!s->jack.new_ports || !s->midi.new_ports)
		return -ENOMEM;
	return 0;
}

static
void stream_close(midi_stream_t *s)
{
	if (s->wake_pipe[0] != -1) {
		close(s->wake_pipe[0]);
		close(s->wake_pipe[1]);
	}
	if (s->jack.new_ports)
		jack_ringbuffer_free(s->jack.new_ports);
	if (s->midi.new_ports)
		jack_ringbuffer_free(s->midi.new_ports);
}

static void alsa_rawmidi_delete(alsa_midi_t *m);
static int alsa_rawmidi_attach(alsa_midi_t *m);
static int alsa_rawmidi_detach(alsa_midi_t *m);
static int alsa_rawmidi_start(alsa_midi_t *m);
static int alsa_rawmidi_stop(alsa_midi_t *m);
static void alsa_rawmidi_read(alsa_midi_t *m, jack_nframes_t nframes);
static void alsa_rawmidi_write(alsa_midi_t *m, jack_nframes_t nframes);

alsa_midi_t* alsa_rawmidi_new(jack_client_t *jack)
{
	alsa_rawmidi_t *midi = calloc(1, sizeof(alsa_rawmidi_t));
	if (!midi)
		goto fail_0;
	midi->client = jack;
	if (pipe(midi->scan.wake_pipe)==-1) {
		error_log("pipe() in alsa_midi_new failed: %s", strerror(errno));
		goto fail_1;
	}

	if (stream_init(&midi->in, midi, "in"))
		goto fail_2;
	midi->in.mode = POLLIN;
	midi->in.port_size = sizeof(input_port_t);
	midi->in.port_init = input_port_init;
	midi->in.port_close = input_port_close;
	midi->in.process_jack = do_jack_input;
	midi->in.process_midi = do_midi_input;

	if (stream_init(&midi->out, midi, "out"))
		goto fail_3;
	midi->out.mode = POLLOUT;
	midi->out.port_size = sizeof(output_port_t);
	midi->out.port_init = output_port_init;
	midi->out.port_close = output_port_close;
	midi->out.process_jack = do_jack_output;
	midi->out.process_midi = do_midi_output;

	midi->ops.destroy = alsa_rawmidi_delete;
	midi->ops.attach = alsa_rawmidi_attach;
	midi->ops.detach = alsa_rawmidi_detach;
	midi->ops.start = alsa_rawmidi_start;
	midi->ops.stop = alsa_rawmidi_stop;
	midi->ops.read = alsa_rawmidi_read;
	midi->ops.write = alsa_rawmidi_write;
	midi->midi_in_cnt = 0;
	midi->midi_out_cnt = 0;

	return &midi->ops;
 fail_3:
 	stream_close(&midi->out);
 fail_2:
 	stream_close(&midi->in);
 	close(midi->scan.wake_pipe[1]);
 	close(midi->scan.wake_pipe[0]);
 fail_1:
 	free(midi);
 fail_0:
	return NULL;
}

static
midi_port_t** scan_port_del(alsa_rawmidi_t *midi, midi_port_t **list);

static
void alsa_rawmidi_delete(alsa_midi_t *m)
{
	alsa_rawmidi_t *midi = (alsa_rawmidi_t*)m;

	alsa_rawmidi_detach(m);

	stream_close(&midi->out);
	stream_close(&midi->in);
	close(midi->scan.wake_pipe[0]);
	close(midi->scan.wake_pipe[1]);

	free(midi);
}

static void* scan_thread(void *);
static void *midi_thread(void *arg);

static
int alsa_rawmidi_attach(alsa_midi_t *m)
{
	return 0;
}

static
int alsa_rawmidi_detach(alsa_midi_t *m)
{
	alsa_rawmidi_t *midi = (alsa_rawmidi_t*)m;
	midi_port_t **list;

	alsa_rawmidi_stop(m);

	list = &midi->scan.ports;
	while (*list) {
		(*list)->state = PORT_REMOVED_FROM_JACK;
		list = scan_port_del(midi, list);
	}
	return 0;
}

static
int alsa_rawmidi_start(alsa_midi_t *m)
{
	alsa_rawmidi_t *midi = (alsa_rawmidi_t*)m;
	int err;
	char c = 'q';
	if (midi->keep_walking == 1)
		return -EALREADY;

	midi->keep_walking = 1;
	if ((err = jack_client_create_thread(midi->client, &midi->in.thread, MIDI_THREAD_PRIO, jack_is_realtime(midi->client), midi_thread, &midi->in))) {
		midi->keep_walking = 0;
		return err;
	}
	if ((err = jack_client_create_thread(midi->client, &midi->out.thread, MIDI_THREAD_PRIO, jack_is_realtime(midi->client), midi_thread, &midi->out))) {
		midi->keep_walking = 0;
		write(midi->in.wake_pipe[1], &c, 1);
		pthread_join(midi->in.thread, NULL);
		return err;
	}
	if ((err = jack_client_create_thread(midi->client, &midi->scan.thread, 0, 0, scan_thread, midi))) {
		midi->keep_walking = 0;
		write(midi->in.wake_pipe[1], &c, 1);
		write(midi->out.wake_pipe[1], &c, 1);
		pthread_join(midi->in.thread, NULL);
		pthread_join(midi->out.thread, NULL);
		return err;
	}
	return 0;
}

static
int alsa_rawmidi_stop(alsa_midi_t *m)
{
	alsa_rawmidi_t *midi = (alsa_rawmidi_t*)m;
	char c = 'q';
	if (midi->keep_walking == 0)
		return -EALREADY;
	midi->keep_walking = 0;
	write(midi->in.wake_pipe[1], &c, 1);
	write(midi->out.wake_pipe[1], &c, 1);
	write(midi->scan.wake_pipe[1], &c, 1);
	pthread_join(midi->in.thread, NULL);
	pthread_join(midi->out.thread, NULL);
	pthread_join(midi->scan.thread, NULL);
	// ports are freed in alsa_midi_detach()
	return 0;
}

static void jack_process(midi_stream_t *str, jack_nframes_t nframes);

static
void alsa_rawmidi_read(alsa_midi_t *m, jack_nframes_t nframes)
{
	alsa_rawmidi_t *midi = (alsa_rawmidi_t*)m;
	jack_process(&midi->in, nframes);
}

static
void alsa_rawmidi_write(alsa_midi_t *m, jack_nframes_t nframes)
{
	alsa_rawmidi_t *midi = (alsa_rawmidi_t*)m;
	jack_process(&midi->out, nframes);
}

/*
 * -----------------------------------------------------------------------------
 */
static inline
int can_pass(size_t sz, jack_ringbuffer_t *in, jack_ringbuffer_t *out)
{
	return jack_ringbuffer_read_space(in) >= sz && jack_ringbuffer_write_space(out) >= sz;
}

static
void midi_port_init(const alsa_rawmidi_t *midi, midi_port_t *port, snd_rawmidi_info_t *info, const alsa_id_t *id)
{
	const char *name;
	char *c;

	port->id = *id;
	snprintf(port->dev, sizeof(port->dev), "hw:%d,%d,%d", id->id[0], id->id[1], id->id[3]);
	name = snd_rawmidi_info_get_subdevice_name(info);
	if (!strlen(name))
		name = snd_rawmidi_info_get_name(info);
	snprintf(port->name, sizeof(port->name), "%s %s %s", port->id.id[2] ? "out":"in", port->dev, name);

	// replace all offending characters with '-'
	for (c=port->name; *c; ++c)
	        if (!isalnum(*c))
			*c = '-';

	port->state = PORT_CREATED;
}

static
inline int midi_port_open_jack(alsa_rawmidi_t *midi, midi_port_t *port, int type, const char *alias)
{
	char name[128];

	if (type & JackPortIsOutput)
		snprintf(name, sizeof(name), "system:midi_capture_%d", ++midi->midi_in_cnt);
	else
		snprintf(name, sizeof(name), "system:midi_playback_%d", ++midi->midi_out_cnt);

	port->jack = jack_port_register(midi->client, name, JACK_DEFAULT_MIDI_TYPE,
		type | JackPortIsPhysical | JackPortIsTerminal, 0);

	if (port->jack)
		jack_port_set_alias(port->jack, alias);
	return port->jack == NULL;
}

static
int midi_port_open(alsa_rawmidi_t *midi, midi_port_t *port)
{
	int err;
	int type;
	char name[64];
	snd_rawmidi_t **in = NULL;
	snd_rawmidi_t **out = NULL;

	if (port->id.id[2] == 0) {
		in = &port->rawmidi;
		type = JackPortIsOutput;
	} else {
		out = &port->rawmidi;
		type = JackPortIsInput;
	}

	if ((err = snd_rawmidi_open(in, out, port->dev, SND_RAWMIDI_NONBLOCK))<0)
		return err;

	/* Some devices (emu10k1) have subdevs with the same name,
	 * and we need to generate unique port name for jack */
	snprintf(name, sizeof(name), "%s", port->name);
	if (midi_port_open_jack(midi, port, type, name)) {
		int num;
		num = port->id.id[3] ? port->id.id[3] : port->id.id[1];
		snprintf(name, sizeof(name), "%s %d", port->name, num);
		if (midi_port_open_jack(midi, port, type, name))
			return 2;
	}
	if ((port->event_ring = jack_ringbuffer_create(MAX_EVENTS*sizeof(event_head_t)))==NULL)
		return 3;
	if ((port->data_ring = jack_ringbuffer_create(MAX_DATA))==NULL)
		return 4;

	return 0;
}

static
void midi_port_close(const alsa_rawmidi_t *midi, midi_port_t *port)
{
	if (port->data_ring) {
		jack_ringbuffer_free(port->data_ring);
		port->data_ring = NULL;
	}
	if (port->event_ring) {
		jack_ringbuffer_free(port->event_ring);
		port->event_ring = NULL;
	}
	if (port->jack) {
		jack_port_unregister(midi->client, port->jack);
		port->jack = NULL;
	}
	if (port->rawmidi) {
		snd_rawmidi_close(port->rawmidi);
		port->rawmidi = NULL;
	}
}

/*
 * ------------------------- Port scanning -------------------------------
 */

static
int alsa_id_before(const alsa_id_t *p1, const alsa_id_t *p2)
{
	int i;
	for (i=0; i<4; ++i) {
		if (p1->id[i] < p2->id[i])
			return 1;
		else if (p1->id[i] > p2->id[i])
			return 0;
	}
	return 0;
}

static
void alsa_get_id(alsa_id_t *id, snd_rawmidi_info_t *info)
{
	id->id[0] = snd_rawmidi_info_get_card(info);
	id->id[1] = snd_rawmidi_info_get_device(info);
	id->id[2] = snd_rawmidi_info_get_stream(info) == SND_RAWMIDI_STREAM_OUTPUT ? 1 : 0;
	id->id[3] = snd_rawmidi_info_get_subdevice(info);
}

#include <stdio.h>

static inline
void alsa_error(const char *func, int err)
{
	error_log("%s() failed", snd_strerror(err));
}

typedef struct {
	alsa_rawmidi_t *midi;
	midi_port_t **iterator;
	snd_ctl_t *ctl;
	snd_rawmidi_info_t *info;
} scan_t;

static midi_port_t** scan_port_del(alsa_rawmidi_t *midi, midi_port_t **list);

static
void scan_cleanup(alsa_rawmidi_t *midi)
{
	midi_port_t **list = &midi->scan.ports;
	while (*list)
		list = scan_port_del(midi, list);
}

static void scan_card(scan_t *scan);
static midi_port_t** scan_port_open(alsa_rawmidi_t *midi, midi_port_t **list);

void scan_cycle(alsa_rawmidi_t *midi)
{
	int card = -1, err;
	scan_t scan;
	midi_port_t **ports;

	//debug_log("scan: cleanup");
	scan_cleanup(midi);

	scan.midi = midi;
	scan.iterator = &midi->scan.ports;
	snd_rawmidi_info_alloca(&scan.info);

	//debug_log("scan: rescan");
	while ((err = snd_card_next(&card))>=0 && card>=0) {
		char name[32];
		snprintf(name, sizeof(name), "hw:%d", card);
		if ((err = snd_ctl_open(&scan.ctl, name, SND_CTL_NONBLOCK))>=0) {
			scan_card(&scan);
			snd_ctl_close(scan.ctl);
		} else
			alsa_error("scan: snd_ctl_open", err);
	}

	// delayed open to workaround alsa<1.0.14 bug (can't open more than 1 subdevice if ctl is opened).
	ports = &midi->scan.ports;
	while (*ports) {
		midi_port_t *port = *ports;
		if (port->state == PORT_CREATED)
			ports = scan_port_open(midi, ports);
		else
			ports = &port->next;
	}
}

static void scan_device(scan_t *scan);

static
void scan_card(scan_t *scan)
{
	int device = -1;
	int err;

	while ((err = snd_ctl_rawmidi_next_device(scan->ctl, &device))>=0 && device >=0) {
		snd_rawmidi_info_set_device(scan->info, device);

		snd_rawmidi_info_set_stream(scan->info, SND_RAWMIDI_STREAM_INPUT);
		snd_rawmidi_info_set_subdevice(scan->info, 0);
		if ((err = snd_ctl_rawmidi_info(scan->ctl, scan->info))>=0)
			scan_device(scan);
		else if (err != -ENOENT)
			alsa_error("scan: snd_ctl_rawmidi_info on device", err);

		snd_rawmidi_info_set_stream(scan->info, SND_RAWMIDI_STREAM_OUTPUT);
		snd_rawmidi_info_set_subdevice(scan->info, 0);
		if ((err = snd_ctl_rawmidi_info(scan->ctl, scan->info))>=0)
			scan_device(scan);
		else if (err != -ENOENT)
			alsa_error("scan: snd_ctl_rawmidi_info on device", err);
	}
}

static void scan_port_update(scan_t *scan);

static
void scan_device(scan_t *scan)
{
	int err;
	int sub, nsubs = 0;
	nsubs = snd_rawmidi_info_get_subdevices_count(scan->info);

	for (sub=0; sub<nsubs; ++sub) {
		snd_rawmidi_info_set_subdevice(scan->info, sub);
		if ((err = snd_ctl_rawmidi_info(scan->ctl, scan->info)) < 0) {
			alsa_error("scan: snd_ctl_rawmidi_info on subdevice", err);
			continue;
		}

		scan_port_update(scan);
	}
}

static midi_port_t** scan_port_add(scan_t *scan, const alsa_id_t *id, midi_port_t **list);

static
void scan_port_update(scan_t *scan)
{
	midi_port_t **list = scan->iterator;
	alsa_id_t id;
	alsa_get_id(&id, scan->info);

	while (*list && alsa_id_before(&(*list)->id, &id))
		list = scan_port_del(scan->midi, list);

	if (!*list || alsa_id_before(&id, &(*list)->id))
		list = scan_port_add(scan, &id, list);
	else if (*list)
		list = &(*list)->next;

	scan->iterator = list;
}

static
midi_port_t** scan_port_add(scan_t *scan, const alsa_id_t *id, midi_port_t **list)
{
	midi_port_t *port;
	midi_stream_t *str = id->id[2] ? &scan->midi->out : &scan->midi->in;

	port = calloc(1, str->port_size);
	if (!port)
		return list;
	midi_port_init(scan->midi, port, scan->info, id);

	port->next = *list;
	*list = port;
	info_log("scan: added port %s %s", port->dev, port->name);
	return &port->next;
}

static
midi_port_t** scan_port_open(alsa_rawmidi_t *midi, midi_port_t **list)
{
	int ret;
	midi_stream_t *str;
	midi_port_t *port;

	port = *list;
	str = port->id.id[2] ? &midi->out : &midi->in;

	if (jack_ringbuffer_write_space(str->jack.new_ports) < sizeof(port))
		goto fail_0;

	ret = midi_port_open(midi, port);
	if (ret)
		goto fail_1;
	if ((str->port_init)(midi, port))
		goto fail_2;

	port->state = PORT_ADDED_TO_JACK;
	jack_ringbuffer_write(str->jack.new_ports, (char*) &port, sizeof(port));

	info_log("scan: opened port %s %s", port->dev, port->name);
	return &port->next;

 fail_2:
 	(str->port_close)(midi, port);
 fail_1:
	midi_port_close(midi, port);
	port->state = PORT_ZOMBIFIED;
	error_log("scan: can't open port %s %s, error code %d, zombified", port->dev, port->name, ret);
	return &port->next;
 fail_0:
	error_log("scan: can't open port %s %s", port->dev, port->name);
	return &port->next;
}

static
midi_port_t** scan_port_del(alsa_rawmidi_t *midi, midi_port_t **list)
{
	midi_port_t *port = *list;
	if (port->state == PORT_REMOVED_FROM_JACK) {
		info_log("scan: deleted port %s %s", port->dev, port->name);
		*list = port->next;
		if (port->id.id[2] )
			(midi->out.port_close)(midi, port);
		else
			(midi->in.port_close)(midi, port);
		midi_port_close(midi, port);
		free(port);
		return list;
	} else {
		//debug_log("can't delete port %s, wrong state: %d", port->name, (int)port->state);
		return &port->next;
	}
}

void* scan_thread(void *arg)
{
	alsa_rawmidi_t *midi = arg;
	struct pollfd wakeup;

	wakeup.fd = midi->scan.wake_pipe[0];
	wakeup.events = POLLIN|POLLERR|POLLNVAL;
	while (midi->keep_walking) {
		int res;
		//error_log("scanning....");
		scan_cycle(midi);
		res = poll(&wakeup, 1, 2000);
		if (res>0) {
			char c;
			read(wakeup.fd, &c, 1);
		} else if (res<0 && errno != EINTR)
			break;
	}
	return NULL;
}

/*
 * ------------------------------- Input/Output  ------------------------------
 */

static
void jack_add_ports(midi_stream_t *str)
{
	midi_port_t *port;
	while (can_pass(sizeof(port), str->jack.new_ports, str->midi.new_ports) && str->jack.nports < MAX_PORTS) {
		jack_ringbuffer_read(str->jack.new_ports, (char*)&port, sizeof(port));
		str->jack.ports[str->jack.nports++] = port;
		port->state = PORT_ADDED_TO_MIDI;
		jack_ringbuffer_write(str->midi.new_ports, (char*)&port, sizeof(port));
	}
}

static
void jack_process(midi_stream_t *str, jack_nframes_t nframes)
{
	int r, w;
	process_jack_t proc;
	jack_nframes_t cur_frames;

	if (!str->owner->keep_walking)
		return;

	proc.midi = str->owner;
	proc.nframes = nframes;
	proc.frame_time = jack_last_frame_time(proc.midi->client);
	cur_frames = jack_frame_time(proc.midi->client);
	int periods_diff = cur_frames - proc.frame_time;
	if (periods_diff < proc.nframes) {
		int periods_lost = periods_diff / proc.nframes;
		proc.frame_time += periods_lost * proc.nframes;
		debug_log("xrun detected: %d periods lost", periods_lost);
	}

	// process existing ports
	for (r=0, w=0; r<str->jack.nports; ++r) {
		midi_port_t *port = str->jack.ports[r];
		proc.port = port;

		assert (port->state > PORT_ADDED_TO_JACK && port->state < PORT_REMOVED_FROM_JACK);

		proc.buffer = jack_port_get_buffer(port->jack, nframes);
		if (str->mode == POLLIN)
			jack_midi_clear_buffer(proc.buffer);

		if (port->state == PORT_REMOVED_FROM_MIDI) {
			port->state = PORT_REMOVED_FROM_JACK; // this signals to scan thread
			continue; // this effectively removes port from the midi->in.jack.ports[]
		}

		(str->process_jack)(&proc);

		if (r != w)
			str->jack.ports[w] = port;
		++w;
	}
	if (str->jack.nports != w) {
		debug_log("jack_%s: nports %d -> %d", str->name, str->jack.nports, w);
	}
	str->jack.nports = w;

	jack_add_ports(str); // it makes no sense to add them earlier since they have no data yet

	// wake midi thread
	write(str->wake_pipe[1], &r, 1);
}

static
void *midi_thread(void *arg)
{
	midi_stream_t *str = arg;
	alsa_rawmidi_t *midi = str->owner;
	struct pollfd pfds[MAX_PFDS];
	int npfds;
	jack_time_t wait_nsec = 1000*1000*1000; // 1 sec
	process_midi_t proc;

	proc.midi = midi;
	proc.mode = str->mode;

	pfds[0].fd = str->wake_pipe[0];
	pfds[0].events = POLLIN|POLLERR|POLLNVAL;
	npfds = 1;

 	if (jack_is_realtime(midi->client))
	    set_threaded_log_function();

	//debug_log("midi_thread(%s): enter", str->name);

	while (midi->keep_walking) {
		int poll_timeout;
		int wait_nanosleep;
		int r=1, w=1; // read,write pos in pfds
		int rp=0, wp=0; // read, write pos in ports

		// sleep
		//if (wait_nsec != 1000*1000*1000) {
		//	debug_log("midi_thread(%s): ", str->name);
		//	assert (wait_nsec == 1000*1000*1000);
		//}
		poll_timeout = wait_nsec / (1000*1000);
		wait_nanosleep = wait_nsec % (1000*1000);
		if (wait_nanosleep > NANOSLEEP_RESOLUTION) {
			struct timespec ts;
			ts.tv_sec = 0;
			ts.tv_nsec = wait_nanosleep;
			clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
		}
		int res = poll((struct pollfd*)&pfds, npfds, poll_timeout);
		//debug_log("midi_thread(%s): poll exit: %d", str->name, res);
		if (!midi->keep_walking)
			break;
		if (res < 0) {
			if (errno == EINTR)
				continue;
			error_log("midi_thread(%s) poll failed: %s", str->name, strerror(errno));
			break;
		}

		// check wakeup pipe
		if (pfds[0].revents & ~POLLIN)
			break;
		if (pfds[0].revents & POLLIN) {
			char c;
			read(pfds[0].fd, &c, 1);
		}

		// add new ports
		while (jack_ringbuffer_read_space(str->midi.new_ports) >= sizeof(midi_port_t*) && str->midi.nports < MAX_PORTS) {
			midi_port_t *port;
			jack_ringbuffer_read(str->midi.new_ports, (char*)&port, sizeof(port));
			str->midi.ports[str->midi.nports++] = port;
			debug_log("midi_thread(%s): added port %s", str->name, port->name);
		}

//		if (res == 0)
//			continue;

		// process ports
		proc.cur_time = 0; //jack_frame_time(midi->client);
		proc.next_time = NFRAMES_INF;

		for (rp = 0; rp < str->midi.nports; ++rp) {
			midi_port_t *port = str->midi.ports[rp];
			proc.cur_time = jack_frame_time(midi->client);
			proc.port = port;
			proc.rpfds = &pfds[r];
			proc.wpfds = &pfds[w];
			proc.max_pfds = MAX_PFDS - w;
			r += port->npfds;
			if (!(str->process_midi)(&proc)) {
				port->state = PORT_REMOVED_FROM_MIDI; // this signals to jack thread
				continue; // this effectively removes port from array
			}
			w += port->npfds;
			if (rp != wp)
				str->midi.ports[wp] = port;
			++wp;
		}
		if (str->midi.nports != wp) {
			debug_log("midi_%s: nports %d -> %d", str->name, str->midi.nports, wp);
		}
		str->midi.nports = wp;
		if (npfds != w) {
			debug_log("midi_%s: npfds %d -> %d", str->name, npfds, w);
		}
		npfds = w;

		/*
		 * Input : ports do not set proc.next_time.
		 * Output: port sets proc.next_time ONLY if it does not have queued data.
		 * So, zero timeout will not cause busy-looping.
		 */
		if (proc.next_time < proc.cur_time) {
			debug_log("%s: late: next_time = %d, cur_time = %d", str->name, (int)proc.next_time, (int)proc.cur_time);
			wait_nsec = 0; // we are late
		} else if (proc.next_time != NFRAMES_INF) {
			jack_time_t wait_frames = proc.next_time - proc.cur_time;
			jack_nframes_t rate = jack_get_sample_rate(midi->client);
			wait_nsec = (wait_frames * (1000*1000*1000)) / rate;
			debug_log("midi_%s: timeout = %d", str->name, (int)wait_frames);
		} else
			wait_nsec = 1000*1000*1000;
		//debug_log("midi_thread(%s): wait_nsec = %lld", str->name, wait_nsec);
	}
	return NULL;
}

static
int midi_is_ready(process_midi_t *proc)
{
	midi_port_t *port = proc->port;
	if (port->npfds) {
		unsigned short revents = 0;
		int res = snd_rawmidi_poll_descriptors_revents(port->rawmidi, proc->rpfds, port->npfds, &revents);
		if (res) {
			error_log("snd_rawmidi_poll_descriptors_revents failed on port %s with: %s", port->name, snd_strerror(res));
			return 0;
		}

		if (revents & ~proc->mode) {
			debug_log("midi: port %s failed", port->name);
			return 0;
		}
		if (revents & proc->mode) {
			port->is_ready = 1;
			debug_log("midi: is_ready %s", port->name);
		}
	}
	return 1;
}

static
int midi_update_pfds(process_midi_t *proc)
{
	midi_port_t *port = proc->port;
	if (port->npfds == 0) {
		port->npfds = snd_rawmidi_poll_descriptors_count(port->rawmidi);
		if (port->npfds > proc->max_pfds) {
			debug_log("midi: not enough pfds for port %s", port->name);
			return 0;
		}
		snd_rawmidi_poll_descriptors(port->rawmidi, proc->wpfds, port->npfds);
	} else if (proc->rpfds != proc->wpfds) {
		memmove(proc->wpfds, proc->rpfds, sizeof(struct pollfd) * port->npfds);
	}
	return 1;
}

/*
 * ------------------------------------ Input ------------------------------
 */

static
int input_port_init(alsa_rawmidi_t *midi, midi_port_t *port)
{
	input_port_t *in = (input_port_t*)port;
	midi_unpack_init(&in->unpack);
	return 0;
}

static
void input_port_close(alsa_rawmidi_t *midi, midi_port_t *port)
{}

/*
 * Jack-level input.
 */

static
void do_jack_input(process_jack_t *p)
{
	input_port_t *port = (input_port_t*) p->port;
	event_head_t event;
	while (jack_ringbuffer_read_space(port->base.event_ring) >= sizeof(event)) {
		jack_ringbuffer_data_t vec[2];
		jack_nframes_t time;
		int i, todo;

		jack_ringbuffer_read(port->base.event_ring, (char*)&event, sizeof(event));
		// TODO: take into account possible warping
		if ((event.time + p->nframes) < p->frame_time)
			time = 0;
		else if (event.time >= p->frame_time)
			time = p->nframes -1;
		else
			time = event.time + p->nframes - p->frame_time;

		jack_ringbuffer_get_read_vector(port->base.data_ring, vec);
		assert ((vec[0].len + vec[1].len) >= event.size);

		if (event.overruns)
			midi_unpack_reset(&port->unpack);

		todo = event.size;
		for (i=0; i<2 && todo>0; ++i) {
			int avail = todo < vec[i].len ? todo : vec[i].len;
			int done = midi_unpack_buf(&port->unpack, (unsigned char*)vec[i].buf, avail, p->buffer, time);
			if (done != avail) {
				debug_log("jack_in: buffer overflow in port %s", port->base.name);
				break;
			}
			todo -= done;
		}
		jack_ringbuffer_read_advance(port->base.data_ring, event.size);
	}
}

/*
 * Low level input.
 */
static
int do_midi_input(process_midi_t *proc)
{
	input_port_t *port = (input_port_t*) proc->port;
	if (!midi_is_ready(proc))
		return 0;

	if (port->base.is_ready) {
		jack_ringbuffer_data_t vec[2];
		int res;

		jack_ringbuffer_get_write_vector(port->base.data_ring, vec);
		if (jack_ringbuffer_write_space(port->base.event_ring) < sizeof(event_head_t) || vec[0].len < 1) {
			port->overruns++;
			if (port->base.npfds) {
				debug_log("midi_in: internal overflow on %s", port->base.name);
			}
			// remove from poll to prevent busy-looping
			port->base.npfds = 0;
			return 1;
		}
		res = snd_rawmidi_read(port->base.rawmidi, vec[0].buf, vec[0].len);
		if (res < 0 && res != -EWOULDBLOCK) {
			error_log("midi_in: reading from port %s failed: %s", port->base.name, snd_strerror(res));
			return 0;
		} else if (res > 0) {
			event_head_t event;
			event.time = proc->cur_time;
			event.size = res;
			event.overruns = port->overruns;
			port->overruns = 0;
			debug_log("midi_in: read %d bytes at %d", (int)event.size, (int)event.time);
			jack_ringbuffer_write_advance(port->base.data_ring, event.size);
			jack_ringbuffer_write(port->base.event_ring, (char*)&event, sizeof(event));
		}
		port->base.is_ready = 0;
	}

	if (!midi_update_pfds(proc))
		return 0;

	return 1;
}

/*
 * ------------------------------------ Output ------------------------------
 */

static int output_port_init(alsa_rawmidi_t *midi, midi_port_t *port)
{
	output_port_t *out = (output_port_t*)port;
	midi_pack_reset(&out->packer);
	out->next_event.time = 0;
	out->next_event.size = 0;
	out->todo = 0;
	return 0;
}

static void output_port_close(alsa_rawmidi_t *midi, midi_port_t *port)
{}

static
void do_jack_output(process_jack_t *proc)
{
	output_port_t *port = (output_port_t*) proc->port;
	int nevents = jack_midi_get_event_count(proc->buffer);
	int i;
	if (nevents) {
		debug_log("jack_out: %d events in %s", nevents, port->base.name);
	}
	for (i=0; i<nevents; ++i) {
		jack_midi_event_t event;
		event_head_t hdr;

		jack_midi_event_get(&event, proc->buffer, i);

		if (jack_ringbuffer_write_space(port->base.data_ring) < event.size || jack_ringbuffer_write_space(port->base.event_ring) < sizeof(hdr)) {
			debug_log("jack_out: output buffer overflow on %s", port->base.name);
			break;
		}

		midi_pack_event(&port->packer, &event);

		jack_ringbuffer_write(port->base.data_ring, (char*)event.buffer, event.size);

		hdr.time = proc->frame_time + event.time + proc->nframes;
		hdr.size = event.size;
		jack_ringbuffer_write(port->base.event_ring, (char*)&hdr, sizeof(hdr));
		debug_log("jack_out: sent %d-byte event at %ld", (int)event.size, (long)event.time);
	}
}

static
int do_midi_output(process_midi_t *proc)
{
	int worked = 0;
	output_port_t *port = (output_port_t*) proc->port;

	if (!midi_is_ready(proc))
		return 0;

	// eat events
	while (port->next_event.time <= proc->cur_time) {
		port->todo += port->next_event.size;
		if (jack_ringbuffer_read(port->base.event_ring, (char*)&port->next_event, sizeof(port->next_event))!=sizeof(port->next_event)) {
			port->next_event.time = 0;
			port->next_event.size = 0;
			break;
		} else {
			debug_log("midi_out: at %ld got %d bytes for %ld", (long)proc->cur_time, (int)port->next_event.size, (long)port->next_event.time);
		}
	}

	if (port->todo) {
		debug_log("midi_out: todo = %d at %ld", (int)port->todo, (long)proc->cur_time);
	}

	// calc next wakeup time
	if (!port->todo && port->next_event.time && port->next_event.time < proc->next_time) {
		proc->next_time = port->next_event.time;
		debug_log("midi_out: next_time = %ld", (long)proc->next_time);
	}

	if (port->todo && port->base.is_ready) {
		// write data
		int size = port->todo;
		int res;
		jack_ringbuffer_data_t vec[2];

		jack_ringbuffer_get_read_vector(port->base.data_ring, vec);
		if (size > vec[0].len) {
			size = vec[0].len;
			assert (size > 0);
		}
		res = snd_rawmidi_write(port->base.rawmidi, vec[0].buf, size);
		if (res > 0) {
			jack_ringbuffer_read_advance(port->base.data_ring, res);
			debug_log("midi_out: written %d bytes to %s", res, port->base.name);
			port->todo -= res;
			worked = 1;
		} else if (res == -EWOULDBLOCK) {
			port->base.is_ready = 0;
			debug_log("midi_out: -EWOULDBLOCK on %s", port->base.name);
			return 1;
		} else {
			error_log("midi_out: writing to port %s failed: %s", port->base.name, snd_strerror(res));
			return 0;
		}
		snd_rawmidi_drain(port->base.rawmidi);
	}

	// update pfds for this port
	if (!midi_update_pfds(proc))
		return 0;

	if (!port->todo) {
		int i;
		if (worked) {
			debug_log("midi_out: relaxing on %s", port->base.name);
		}
		for (i=0; i<port->base.npfds; ++i)
			proc->wpfds[i].events &= ~POLLOUT;
	} else {
		int i;
		for (i=0; i<port->base.npfds; ++i)
			proc->wpfds[i].events |= POLLOUT;
	}
	return 1;
}
