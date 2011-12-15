/** @file cpu_load.c
 *
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <jack/jack.h>


jack_client_t *client;

static void signal_handler(int sig)
{
    jack_client_close(client);
    fprintf(stderr, "signal received, exiting ...\n");
    exit(0);
}


/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void
jack_shutdown(void *arg)
{
     exit(1);
}

int
main(int argc, char *argv[])
{
    jack_options_t options = JackNullOption;
    jack_status_t status;

    /* open a client connection to the JACK server */

    client = jack_client_open ("jack_cpu_load", options, &status);
    if (client == NULL) {
        fprintf(stderr, "jack_client_open() failed, "
                  "status = 0x%2.0x\n", status);
        if (status & JackServerFailed) {
            fprintf(stderr, "Unable to connect to JACK server\n");
        }
        exit(1);
    }

    jack_on_shutdown(client, jack_shutdown, 0);

    /* Tell the JACK server that we are ready to roll.  Our
     * process() callback will start running now. */

    if (jack_activate(client)) {
        fprintf(stderr, "cannot activate client");
        exit(1);
    }

    /* install a signal handler to properly quits jack client */
#ifdef WIN32
    signal(SIGINT, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGTERM, signal_handler);
#else
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);
#endif

    while (1) {
        printf("jack DSP load %f\n", jack_cpu_load(client));
#ifdef WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
    }

    jack_client_close(client);
    exit(0 );
}
