/** @file control.c
 *
 * @brief This simple client demonstrates the basic features of JACK
 * as they would be used by many applications.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <jack/jack.h>

jack_client_t *client;
static int reorder = 0;

static int Jack_Graph_Order_Callback(void *arg)
{
    const char **ports;
    int i;

    printf("Jack_Graph_Order_Callback count = %d\n", reorder++);

    ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput);
    if (ports) {
        for (i = 0;  ports[i]; ++i) {
            printf("name: %s\n", ports[i]);
        }
        jack_free(ports);
    }

    ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsInput);
    if (ports) {
        for (i = 0;  ports[i]; ++i) {
            printf("name: %s\n", ports[i]);
        }
        jack_free(ports);
    }

    return 0;
}

int
main (int argc, char *argv[])
{
	jack_options_t options = JackNullOption;
	jack_status_t status;

	/* open a client connection to the JACK server */

	client = jack_client_open("control_client", options, &status);
	if (client == NULL) {
		printf("jack_client_open() failed \n");
		exit(1);
	}

	if (jack_set_graph_order_callback(client, Jack_Graph_Order_Callback, 0) != 0) {
        printf("Error when calling jack_set_graph_order_callback() !\n");
    }

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	if (jack_activate(client)) {
		printf("cannot activate client");
		exit(1);
	}

    printf("Type 'q' to quit\n");
    while ((getchar() != 'q')) {}

	jack_client_close(client);
	exit (0);
}
