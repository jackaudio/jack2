/** @file inprocess.c
 *
 * @brief This demonstrates the basic concepts for writing a client
 * that runs within the JACK server process.
 *
 * For the sake of example, a port_pair_t is allocated in
 * jack_initialize(), passed to inprocess() as an argument, then freed
 * in jack_finish().
 */

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <jack/jack.h>

/**
 * For the sake of example, an instance of this struct is allocated in
 * jack_initialize(), passed to inprocess() as an argument, then freed
 * in jack_finish().
 */
typedef struct {
	jack_port_t *input_port;
	jack_port_t *output_port;
} port_pair_t;

/**
 * Called in the realtime thread on every process cycle.  The entry
 * point name was passed to jack_set_process_callback() from
 * jack_initialize().  Although this is an internal client, its
 * process() interface is identical to @ref simple_client.c.
 *
 * @return 0 if successful; otherwise jack_finish() will be called and
 * the client terminated immediately.
 */
int
inprocess (jack_nframes_t nframes, void *arg)
{
	port_pair_t *pp = arg;
	jack_default_audio_sample_t *out =
		jack_port_get_buffer (pp->output_port, nframes);
	jack_default_audio_sample_t *in =
		jack_port_get_buffer (pp->input_port, nframes);

	memcpy (out, in, sizeof (jack_default_audio_sample_t) * nframes);

	return 0;			/* continue */
}

/**
 * This required entry point is called after the client is loaded by
 * jack_internal_client_load().
 *
 * @param client pointer to JACK client structure.
 * @param load_init character string passed to the load operation.
 *
 * @return 0 if successful; otherwise jack_finish() will be called and
 * the client terminated immediately.
 */
int
jack_initialize (jack_client_t *client, const char *load_init)
{
	port_pair_t *pp = malloc (sizeof (port_pair_t));
	const char **ports;

	if (pp == NULL)
		return 1;		/* heap exhausted */

	jack_set_process_callback (client, inprocess, pp);

	/* create a pair of ports */
	pp->input_port = jack_port_register (client, "input",
					     JACK_DEFAULT_AUDIO_TYPE,
					     JackPortIsInput, 0);
	pp->output_port = jack_port_register (client, "output",
					      JACK_DEFAULT_AUDIO_TYPE,
					      JackPortIsOutput, 0);

	/* join the process() cycle */
	jack_activate (client);
	
	ports = jack_get_ports (client, NULL, NULL,
				JackPortIsPhysical|JackPortIsOutput);
	if (ports == NULL) {
		fprintf(stderr, "no physical capture ports\n");
		return 1;		/* terminate client */
	}

	if (jack_connect (client, ports[0], jack_port_name (pp->input_port))) {
		fprintf (stderr, "cannot connect input ports\n");
	}
	
	free (ports);
	
	ports = jack_get_ports (client, NULL, NULL,
				JackPortIsPhysical|JackPortIsInput);
	if (ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
		return 1;		/* terminate client */
	}
	
	if (jack_connect (client, jack_port_name (pp->output_port), ports[0])) {
		fprintf (stderr, "cannot connect output ports\n");
	}
	
	free (ports);

	return 0;			/* success */
}

/**
 * This required entry point is called immediately before the client
 * is unloaded, which could happen due to a call to
 * jack_internal_client_unload(), or a nonzero return from either
 * jack_initialize() or inprocess().
 *
 * @param arg the same parameter provided to inprocess().
 */
void
jack_finish (void *arg)
{
	if (arg)
		free ((port_pair_t *) arg);
}
