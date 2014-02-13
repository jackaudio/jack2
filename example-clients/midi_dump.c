// gcc -o jack_midi_dump -Wall midi_dump.c -ljack -pthread

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>

#ifdef __MINGW32__
#include <pthread.h>
#endif

#ifndef WIN32
#include <signal.h>
#include <pthread.h>
#include <sys/mman.h>
#endif

#ifndef MAX
#define MAX(a,b) ( (a) < (b) ? (b) : (a) )
#endif

static jack_port_t* port;
static jack_ringbuffer_t *rb = NULL;
static pthread_mutex_t msg_thread_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t data_ready = PTHREAD_COND_INITIALIZER;

static int keeprunning = 1;
static uint64_t monotonic_cnt = 0;

#define RBSIZE 512

typedef struct {
	uint8_t  buffer[128];
	uint32_t size;
	uint32_t tme_rel;
	uint64_t tme_mon;
} midimsg;

static void
describe (midimsg* event)
{
	if (event->size == 0) {
		return;
	}

	uint8_t type = event->buffer[0] & 0xf0;
	uint8_t channel = event->buffer[0] & 0xf;

	switch (type) {
		case 0x90:
			assert (event->size == 3);
			printf (" note on  (channel %2d): pitch %3d, velocity %3d", channel, event->buffer[1], event->buffer[2]);
			break;
		case 0x80:
			assert (event->size == 3);
			printf (" note off (channel %2d): pitch %3d, velocity %3d", channel, event->buffer[1], event->buffer[2]);
			break;
		case 0xb0:
			assert (event->size == 3);
			printf (" control change (channel %2d): controller %3d, value %3d", channel, event->buffer[1], event->buffer[2]);
			break;
		default:
			break;
	}
}

int
process (jack_nframes_t frames, void* arg)
{
	void* buffer;
	jack_nframes_t N;
	jack_nframes_t i;

	buffer = jack_port_get_buffer (port, frames);
	assert (buffer);

	N = jack_midi_get_event_count (buffer);
	for (i = 0; i < N; ++i) {
		jack_midi_event_t event;
		int r;

		r = jack_midi_event_get (&event, buffer, i);

		if (r == 0 && jack_ringbuffer_write_space (rb) >= sizeof(midimsg)) {
			midimsg m;
			m.tme_mon = monotonic_cnt;
			m.tme_rel = event.time;
			m.size    = event.size;
			memcpy (m.buffer, event.buffer, MAX(sizeof(m.buffer), event.size));
			jack_ringbuffer_write (rb, (void *) &m, sizeof(midimsg));

		}
	}

	monotonic_cnt += frames;

	if (pthread_mutex_trylock (&msg_thread_lock) == 0) {
		pthread_cond_signal (&data_ready);
		pthread_mutex_unlock (&msg_thread_lock);
	}

	return 0;
}

static void wearedone(int sig) {
	keeprunning = 0;
}

static void usage (int status) {
	printf ("jack_midi_dump - JACK MIDI Monitor.\n\n");
	printf ("Usage: jack_midi_dump [ OPTIONS ] [CLIENT-NAME]\n\n");
	printf ("Options:\n\
  -a        use absoute timestamps relative to application start\n\
  -h        display this help and exit\n\
  -r        use relative timestamps to previous MIDI event\n\
\n");
	printf ("\n\
This tool listens for MIDI events on a JACK MIDI port and prints\n\
the message to stdout.\n\
\n\
If no client name is given it defaults to 'midi-monitor'.\n\
\n\
See also: jackd(1)\n\
\n");
	exit (status);
}

int
main (int argc, char* argv[])
{
	jack_client_t* client;
	char const default_name[] = "midi-monitor";
	char const * client_name;
	int time_format = 0;
	int r;

	int cn = 1;

	if (argc > 1) {
		if      (!strcmp (argv[1], "-a")) { time_format = 1; cn = 2; }
		else if (!strcmp (argv[1], "-r")) { time_format = 2; cn = 2; }
		else if (!strcmp (argv[1], "-h")) { usage (EXIT_SUCCESS); }
		else if (argv[1][0] == '-')       { usage (EXIT_FAILURE); }
	}

	if (argc > cn) {
		client_name = argv[cn];
	} else {
		client_name = default_name;
	}

	client = jack_client_open (client_name, JackNullOption, NULL);
	if (client == NULL) {
		fprintf (stderr, "Could not create JACK client.\n");
		exit (EXIT_FAILURE);
	}

	rb = jack_ringbuffer_create (RBSIZE * sizeof(midimsg));

	jack_set_process_callback (client, process, 0);

	port = jack_port_register (client, "input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	if (port == NULL) {
		fprintf (stderr, "Could not register port.\n");
		exit (EXIT_FAILURE);
	}

#ifndef WIN32
	if (mlockall (MCL_CURRENT | MCL_FUTURE)) {
		fprintf (stderr, "Warning: Can not lock memory.\n");
	}
#endif

	r = jack_activate (client);
	if (r != 0) {
		fprintf (stderr, "Could not activate client.\n");
		exit (EXIT_FAILURE);
	}

#ifndef WIN32
	signal(SIGHUP, wearedone);
	signal(SIGINT, wearedone);
#endif

	pthread_mutex_lock (&msg_thread_lock);

	uint64_t prev_event = 0;
	while (keeprunning) {
		const int mqlen = jack_ringbuffer_read_space (rb) / sizeof(midimsg);
		int i;
		for (i=0; i < mqlen; ++i) {
			size_t j;
			midimsg m;
			jack_ringbuffer_read(rb, (char*) &m, sizeof(midimsg));

			switch(time_format) {
				case 1:
					printf ("%7"PRId64":", m.tme_rel + m.tme_mon);
					break;
				case 2:
					printf ("%+6"PRId64":", m.tme_rel + m.tme_mon - prev_event);
					break;
				default:
					printf ("%4d:", m.tme_rel);
					break;
			}
			for (j = 0; j < m.size && j < sizeof(m.buffer); ++j) {
				printf (" %02x", m.buffer[j]);
			}

			describe (&m);
			printf("\n");
			prev_event = m.tme_rel + m.tme_mon;
		}
		fflush (stdout);
		pthread_cond_wait (&data_ready, &msg_thread_lock);
	}
	pthread_mutex_unlock (&msg_thread_lock);

	jack_deactivate (client);
	jack_client_close (client);
	jack_ringbuffer_free (rb);

	return 0;
}
