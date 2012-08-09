/** @file tw.c
 *
 * @brief This simple client demonstrates the basic features of JACK
 * as they would be used by many applications.
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <jack/jack.h>

jack_port_t *input_port;
jack_port_t *output_port;
jack_client_t *client;

/* a simple state machine for this client */
volatile enum {
	Init,
	Run,
	Exit
} client_state = Init;

static void signal_handler(int sig)
{
	jack_client_close(client);
	fprintf(stderr, "signal received, exiting ...\n");
	exit(0);
}

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client follows a simple rule: when the JACK transport is
 * running, copy the input port to the output.  When it stops, exit.
 */
static int
_process (jack_nframes_t nframes)
{
	jack_default_audio_sample_t *in, *out;
	jack_transport_state_t ts = jack_transport_query(client, NULL);

	if (ts == JackTransportRolling) {

		if (client_state == Init)
			client_state = Run;

		in = jack_port_get_buffer (input_port, nframes);
		out = jack_port_get_buffer (output_port, nframes);
		memcpy (out, in,
			sizeof (jack_default_audio_sample_t) * nframes);

	} else if (ts == JackTransportStopped) {

		if (client_state == Run) {
			client_state = Exit;
			return -1;  // to stop the thread
		}
	}

	return 0;
}

static void* jack_thread(void *arg)
{
	jack_client_t* client = (jack_client_t*) arg;

	while (1) {

		jack_nframes_t frames = jack_cycle_wait (client);
		int status = _process(frames);
		jack_cycle_signal (client, status);

        /*
            Possibly do something else after signaling next clients in the graph
        */

        /* End condition */
        if (status != 0)
            return 0;
	}

    /* not reached*/
	return 0;
}

/*
static void* jack_thread(void *arg)
{
	jack_client_t* client = (jack_client_t*) arg;

	while (1) {
		jack_nframes_t frames;
		int status;
		// cycle 1
		frames = jack_cycle_wait (client);
		status = _process(frames);
		jack_cycle_signal (client, status);
		// cycle 2
		frames = jack_cycle_wait (client);
		status = _process(frames);
		jack_cycle_signal (client, status);
		// cycle 3
		frames = jack_cycle_wait (client);
		status = _process(frames);
		jack_cycle_signal (client, status);
		// cycle 4
		frames = jack_cycle_wait (client);
		status = _process(frames);
		jack_cycle_signal (client, status);
	}

	return 0;
}
*/

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
static void
jack_shutdown (void *arg)
{
    fprintf(stderr, "JACK shut down, exiting ...\n");
	exit (1);
}

int
main (int argc, char *argv[])
{
	const char **ports;
	const char *client_name;
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;

	if (argc >= 2) {		/* client name specified? */
		client_name = argv[1];
		if (argc >= 3) {	/* server name specified? */
			server_name = argv[2];
			options |= JackServerName;
		}
	} else {			/* use basename of argv[0] */
		client_name = strrchr(argv[0], '/');
		if (client_name == 0) {
			client_name = argv[0];
		} else {
			client_name++;
		}
	}

	/* open a client connection to the JACK server */

	client = jack_client_open (client_name, options, &status, server_name);
	if (client == NULL) {
		fprintf (stderr, "jack_client_open() failed, "
			 "status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
		exit (1);
	}
	if (status & JackServerStarted) {
		fprintf (stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name(client);
		fprintf (stderr, "unique name `%s' assigned\n", client_name);
	}

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/
    if (jack_set_process_thread(client, jack_thread, client) < 0)
		exit(1);

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/

	jack_on_shutdown (client, jack_shutdown, 0);

	/* display the current sample rate.
	 */

	printf ("engine sample rate: %" PRIu32 "\n",
		jack_get_sample_rate (client));

	/* create two ports */

	input_port = jack_port_register (client, "input",
					 JACK_DEFAULT_AUDIO_TYPE,
					 JackPortIsInput, 0);
	output_port = jack_port_register (client, "output",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput, 0);

	if ((input_port == NULL) || (output_port == NULL)) {
		fprintf(stderr, "no more JACK ports available\n");
		exit (1);
	}

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		exit (1);
	}

	/* Connect the ports.  You can't do this before the client is
	 * activated, because we can't make connections to clients
	 * that aren't running.  Note the confusing (but necessary)
	 * orientation of the driver backend ports: playback ports are
	 * "input" to the backend, and capture ports are "output" from
	 * it.
	 */

	ports = jack_get_ports (client, NULL, NULL,
				JackPortIsPhysical|JackPortIsOutput);
	if (ports == NULL) {
		fprintf(stderr, "no physical capture ports\n");
		exit (1);
	}

	if (jack_connect (client, ports[0], jack_port_name (input_port))) {
		fprintf (stderr, "cannot connect input ports\n");
	}

	jack_free (ports);

	ports = jack_get_ports (client, NULL, NULL,
				JackPortIsPhysical|JackPortIsInput);
	if (ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
		exit (1);
	}

	if (jack_connect (client, jack_port_name (output_port), ports[0])) {
		fprintf (stderr, "cannot connect output ports\n");
	}

	jack_free (ports);

    /* install a signal handler to properly quits jack client */
    signal(SIGQUIT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);

	/* keep running until the transport stops */

	while (client_state != Exit) {
		sleep (1);
	}

	jack_client_close (client);
	exit (0);
}
