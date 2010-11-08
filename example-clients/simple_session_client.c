/** @file simple_session_client.c
 *
 * @brief This simple client demonstrates the most basic features of JACK
 * as they would be used by many applications.
 * this version also adds session manager functionality.
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <jack/jack.h>
#include <jack/types.h>
#include <jack/session.h>

jack_port_t *input_port;
jack_port_t *output_port;
jack_client_t *client;

int simple_quit = 0;

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client does nothing more than copy data from its input
 * port to its output port. It will exit when stopped by
 * the user (e.g. using Ctrl-C on a unix-ish operating system)
 */
int
process (jack_nframes_t nframes, void *arg)
{
        jack_default_audio_sample_t *in, *out;

        in = jack_port_get_buffer (input_port, nframes);
        out = jack_port_get_buffer (output_port, nframes);
        memcpy (out, in,
                sizeof (jack_default_audio_sample_t) * nframes);

        return 0;
}

void
session_callback (jack_session_event_t *event, void *arg)
{
        char retval[100];
        printf ("session notification\n");
        printf ("path %s, uuid %s, type: %s\n", event->session_dir, event->client_uuid, event->type == JackSessionSave ? "save" : "quit");


        snprintf (retval, 100, "jack_simple_session_client %s", event->client_uuid);
        event->command_line = strdup (retval);

        jack_session_reply( client, event );

        if (event->type == JackSessionSaveAndQuit) {
                simple_quit = 1;
        }

        jack_session_event_free (event);
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void
jack_shutdown (void *arg)
{
        exit (1);
}

int
main (int argc, char *argv[])
{
        const char **ports;
        const char *client_name = "simple";
        jack_status_t status;

        /* open a client connection to the JACK server */

        if( argc == 1 )
                client = jack_client_open (client_name, JackNullOption, &status );
        else if( argc == 2 )
                client = jack_client_open (client_name, JackSessionID, &status, argv[1] );

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

        jack_set_process_callback (client, process, 0);

        /* tell the JACK server to call `jack_shutdown()' if
           it ever shuts down, either entirely, or if it
           just decides to stop calling us.
        */

        jack_on_shutdown (client, jack_shutdown, 0);

        /* tell the JACK server to call `session_callback()' if
           the session is saved.
        */

        jack_set_session_callback (client, session_callback, NULL);

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


        /* only do the autoconnect when not reloading from a session.
         * in case of a session reload, the SM will restore our connections
         */

        if (argc==1) {

                ports = jack_get_ports (client, NULL, NULL,
                                JackPortIsPhysical|JackPortIsOutput);
                if (ports == NULL) {
                        fprintf(stderr, "no physical capture ports\n");
                        exit (1);
                }

                if (jack_connect (client, ports[0], jack_port_name (input_port))) {
                        fprintf (stderr, "cannot connect input ports\n");
                }

                free (ports);

                ports = jack_get_ports (client, NULL, NULL,
                                JackPortIsPhysical|JackPortIsInput);
                if (ports == NULL) {
                        fprintf(stderr, "no physical playback ports\n");
                        exit (1);
                }

                if (jack_connect (client, jack_port_name (output_port), ports[0])) {
                        fprintf (stderr, "cannot connect output ports\n");
                }

                free (ports);
        }

        /* keep running until until we get a quit event */

        while (!simple_quit)
                sleep(1);
        

        jack_client_close (client);
        exit (0);
}
