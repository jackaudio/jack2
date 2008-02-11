#include "JackAlsaDriver.h"
#include "JackPort.h"
#include "alsa_midi_impl.h"

using Jack::JackAlsaDriver;

struct fake_port_t {
    JackAlsaDriver* driver;
    int port_id;
	fake_port_t(JackAlsaDriver *d, int i) : driver(d), port_id(i) {}
};

int JACK_is_realtime(jack_client_t* client)
{
	return ((JackAlsaDriver*)client)->is_realtime();
}

int JACK_client_create_thread(jack_client_t* client, pthread_t *thread, int priority, int realtime, void *(*start_routine)(void*), void *arg)
{
	return ((JackAlsaDriver*)client)->create_thread(thread, priority, realtime, start_routine, arg);
}

jack_port_t* JACK_port_register(jack_client_t *client, const char *port_name, const char *port_type, unsigned long flags, unsigned long buffer_size)
{
	JackAlsaDriver *driver = (JackAlsaDriver*)client;
	int port_id = driver->port_register(port_name, port_type, flags, buffer_size);
	if (port_id == NO_PORT)
	    return 0;
	else
	    return (jack_port_t*) new fake_port_t(driver, port_id);
}

int JACK_port_unregister(jack_client_t *client, jack_port_t *port)
{
	fake_port_t* real = (fake_port_t*)port;
	int res = 0; //real->driver->port_unregister(real->port_id);
	delete real;
	return res;
}

void* JACK_port_get_buffer(jack_port_t *port, jack_nframes_t nframes)
{
	fake_port_t* real = (fake_port_t*)port;
	return real->driver->port_get_buffer(real->port_id, nframes);
}

int JACK_port_set_alias(jack_port_t *port, const char* name)
{
	fake_port_t* real = (fake_port_t*)port;
	return real->driver->port_set_alias(real->port_id, name);
}

jack_nframes_t JACK_get_sample_rate(jack_client_t *client)
{
	return ((JackAlsaDriver*)client)->get_sample_rate();
}

jack_nframes_t JACK_frame_time(jack_client_t *client)
{
	return ((JackAlsaDriver*)client)->frame_time();
}

jack_nframes_t JACK_last_frame_time(jack_client_t *client)
{
	return ((JackAlsaDriver*)client)->last_frame_time();
}
