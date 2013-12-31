#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <jack/jack.h>
#include <jack/midiport.h>

#ifndef WIN32
#include <signal.h>
#endif

static jack_port_t* port;

static int keeprunning = 1;
static int time_format = 0;

static uint64_t monotonic_cnt = 0;
static uint64_t prev_event = 0;

static void
describe (jack_midi_event_t* event, char* buffer, size_t buflen)
{
	assert (buflen > 0);

	buffer[0] = '\0';

	if (event->size == 0) {
		return;
	}

	int type = event->buffer[0] & 0xf0;
	int channel = event->buffer[0] & 0xf;

	switch (type) {
	case 0x90:
		assert (event->size == 3);
		snprintf (buffer, buflen, "note on (channel %d): pitch %d, velocity %d", channel, event->buffer[1], event->buffer[2]);
                break;
	case 0x80:
		assert (event->size == 3);
		snprintf (buffer, buflen, "note off (channel %d): pitch %d, velocity %d", channel, event->buffer[1], event->buffer[2]);
                break;
	case 0xb0:
		assert (event->size == 3);
		snprintf (buffer, buflen, "control change (channel %d): controller %d, value %d", channel, event->buffer[1], event->buffer[2]);
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
	char description[256];

	buffer = jack_port_get_buffer (port, frames);
	assert (buffer);

	N = jack_midi_get_event_count (buffer);
	for (i = 0; i < N; ++i) {
		jack_midi_event_t event;
		int r;

		r = jack_midi_event_get (&event, buffer, i);
		if (r == 0) {
			size_t j;

			switch(time_format) {
				case 1:
					printf ("%7"PRId64":", event.time + monotonic_cnt);
					break;
				case 2:
					printf ("%+6"PRId64":", event.time + monotonic_cnt - prev_event);
					break;
				default:
					printf ("%4d:", event.time);
					break;
			}
			for (j = 0; j < event.size; ++j) {
				printf (" %x", event.buffer[j]);
			}

			describe (&event, description, sizeof (description));
			printf (" %s", description);

			printf ("\n");
			prev_event = event.time + monotonic_cnt;
		}
	}

	monotonic_cnt += frames;

	return 0;
}

static void wearedone(int sig) {
	keeprunning = 0;
}

int
main (int argc, char* argv[])
{
	jack_client_t* client;
	char const default_name[] = "midi-monitor";
	char const * client_name;
	int r;

	int cn = 1;

	if (argc > 1) {
		if (!strcmp(argv[1], "-a")) { time_format = 1; cn = 2; }
		if (!strcmp(argv[1], "-r")) { time_format = 2; cn = 2; }
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

	jack_set_process_callback (client, process, 0);

	port = jack_port_register (client, "input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	if (port == NULL) {
		fprintf (stderr, "Could not register port.\n");
		exit (EXIT_FAILURE);
	}

	r = jack_activate (client);
	if (r != 0) {
		fprintf (stderr, "Could not activate client.\n");
		exit (EXIT_FAILURE);
	}

#ifndef WIN32
	signal(SIGHUP, wearedone);
	signal(SIGINT, wearedone);
#endif

	/* run until interrupted */
	while (keeprunning) {
	#ifdef WIN32
		Sleep(1000);
	#else
		sleep(1);
	#endif
	};

	jack_deactivate (client);
	jack_client_close (client);

	return 0;
}
