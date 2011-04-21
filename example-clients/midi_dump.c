#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <jack/jack.h>
#include <jack/midiport.h>

static jack_port_t* port;

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

			printf ("%d:", event.time);
			for (j = 0; j < event.size; ++j) {
				printf (" %x", event.buffer[j]);
			}

			describe (&event, description, sizeof (description));
			printf (" %s", description);

			printf ("\n");
		}
	}

	return 0;
}


int
main (int argc, char* argv[])
{
	jack_client_t* client;
	char const default_name[] = "midi-monitor";
	char const * client_name;
	int r;

	if (argc == 2) {
		client_name = argv[1];
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

	/* run until interrupted */
	while (1) {
	#ifdef WIN32
		Sleep(1000);
	#else
		sleep(1);
	#endif
	};

	return 0;
}
