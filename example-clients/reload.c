#include <stdio.h>
#include <string.h>
#include <jack/jack.h>


int main(int argc, char* argv[])
{
	if (argc < 2) {
		printf("usage: %s [server_name [server_name ...]]", argv[0]);
		return 1;
	}

	const int client_count = argc - 1;
	char** server_names = &argv[1];

	jack_client_t* clients[client_count];

	for (int i = 0; i < client_count; ++i) {
		const jack_options_t options = (jack_options_t) (JackNoStartServer | JackServerName);
		jack_status_t status;

		printf("Connecting to JACK server %s\n", server_names[i]);
		clients[i] = jack_client_open("reload", options, &status, server_names[i]);
		jack_client_reload_master(clients[i]);
	}

	for (int i = 0; i < client_count; ++i) {
		jack_client_close(clients[i]);
	}

	return 0;
}
