/*
Copyright (C) 2010 Devin Anderson

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

/*
 * This program is used to measure MIDI latency and jitter.  It writes MIDI
 * messages to one port and calculates how long it takes before it reads the
 * same MIDI message over another port.  It was written to calculate the
 * latency and jitter of hardware and JACK hardware drivers, but might have
 * other practical applications.
 *
 * The latency results of the program include the latency introduced by the
 * JACK system.  Because JACK has sample accurate MIDI, the same latency
 * imposed on audio is also imposed on MIDI going through the system.  Make
 * sure you take this into account before complaining to me or (*especially*)
 * other JACK developers about reported MIDI latency.
 *
 * The jitter results are a little more interesting.  The program attempts to
 * calculate 'average jitter' and 'peak jitter', as defined here:
 *
 *     http://openmuse.org/transport/fidelity.html
 *
 * It also outputs a jitter plot, which gives you a more specific idea about
 * the MIDI jitter for the ports you're testing.  This is useful for catching
 * extreme jitter values, and for analyzing the amount of truth in the
 * technical specifications for your MIDI interface(s). :)
 *
 * This program is loosely based on 'alsa-midi-latency-test' in the ALSA test
 * suite.
 *
 * To port this program to non-POSIX platforms, you'll have to include
 * implementations for semaphores and command-line argument handling.
 */

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>

#include <jack/jack.h>
#include <jack/midiport.h>

#ifdef WIN32
#include <windows.h>
#include <unistd.h>
#else
#include <semaphore.h>
#endif

#include <signal.h>

#define ABS(x) (((x) >= 0) ? (x) : (-(x)))

#ifdef WIN32
typedef HANDLE semaphore_t;
#else
typedef sem_t *semaphore_t;
#endif

const char *ERROR_RESERVE = "could not reserve MIDI event on port buffer";
const char *ERROR_SHUTDOWN = "the JACK server has been shutdown";
const char *ERROR_TIMEOUT1 = "timed out while waiting for MIDI message";

const char *SOURCE_EVENT_RESERVE = "jack_midi_event_reserve";
const char *SOURCE_PROCESS = "handle_process";
const char *SOURCE_SHUTDOWN = "handle_shutdown";
const char *SOURCE_SIGNAL_SEMAPHORE = "signal_semaphore";
const char *SOURCE_WAIT_SEMAPHORE = "wait_semaphore";

jack_client_t *client;
const char *error_message;
const char *error_source;
jack_nframes_t highest_latency;
jack_time_t highest_latency_time;
jack_latency_range_t in_latency_range;
jack_port_t *in_port;
semaphore_t init_semaphore;
jack_nframes_t last_activity;
jack_time_t last_activity_time;
jack_time_t *latency_time_values;
jack_nframes_t *latency_values;
jack_nframes_t lowest_latency;
jack_time_t lowest_latency_time;
jack_midi_data_t *message_1;
jack_midi_data_t *message_2;
size_t messages_received;
size_t messages_sent;
size_t message_size;
jack_latency_range_t out_latency_range;
jack_port_t *out_port;
semaphore_t process_semaphore;
int process_state;
char *program_name;
jack_port_t *remote_in_port;
jack_port_t *remote_out_port;
size_t samples;
int timeout;
jack_nframes_t total_latency;
jack_time_t total_latency_time;
size_t unexpected_messages;
size_t xrun_count;

static void signal_handler(int sig)
{
	jack_client_close(client);
	fprintf(stderr, "signal received, exiting ...\n");
	exit(0);
}

#ifdef WIN32
char semaphore_error_msg[1024];
#endif

static void
output_error(const char *source, const char *message);

static void
output_usage();

static void
set_process_error(const char *source, const char *message);

static int
signal_semaphore(semaphore_t semaphore);

static int
wait_semaphore(semaphore_t semaphore, int block);

static semaphore_t
create_semaphore(int id)
{
    semaphore_t semaphore;

#ifdef WIN32
    semaphore = CreateSemaphore(NULL, 0, 1, NULL);
#elif defined (__APPLE__)
    char name[128];
    sprintf(name, "midi_sem_%d", id);
    semaphore = sem_open(name, O_CREAT, 0777, 0);
    if (semaphore == (sem_t *) SEM_FAILED) {
        semaphore = NULL;
    }
#else
    semaphore = malloc(sizeof(semaphore_t));
    if (semaphore != NULL) {
        if (sem_init(semaphore, 0, 0)) {
            free(semaphore);
            semaphore = NULL;
        }
    }
#endif

    return semaphore;
}

static void
destroy_semaphore(semaphore_t semaphore, int id)
{

#ifdef WIN32
    CloseHandle(semaphore);
#else
    sem_destroy(semaphore);
#ifdef __APPLE__
    {
        char name[128];
        sprintf(name, "midi_sem_%d", id);
        sem_unlink(name);
    }
#else
    free(semaphore);
#endif
#endif

}

static void
die(const char *source, const char *error_message)
{
    output_error(source, error_message);
    output_usage();
    exit(EXIT_FAILURE);
}

static const char *
get_semaphore_error()
{

#ifdef WIN32
    DWORD error = GetLastError();
    if (! FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        semaphore_error_msg, 1024, NULL)) {
        snprintf(semaphore_error_msg, 1023, "Unknown OS error code '%d'",
                error);
    }
    return semaphore_error_msg;
#else
    return strerror(errno);
#endif

}

static void
handle_info(const char *message)
{
    /* Suppress info */
}

static int
handle_process(jack_nframes_t frames, void *arg)
{
    jack_midi_data_t *buffer;
    jack_midi_event_t event;
    jack_nframes_t event_count;
    jack_nframes_t event_time;
    jack_nframes_t frame;
    size_t i;
    jack_nframes_t last_frame_time;
    jack_midi_data_t *message;
    jack_time_t microseconds;
    void *port_buffer;
    jack_time_t time;
    jack_midi_clear_buffer(jack_port_get_buffer(out_port, frames));
    switch (process_state) {

    case 0:
        /* State: initializing */
        switch (wait_semaphore(init_semaphore, 0)) {
        case -1:
            set_process_error(SOURCE_WAIT_SEMAPHORE, get_semaphore_error());
            // Fallthrough on purpose
        case 0:
            return 0;
        }
        highest_latency = 0;
        lowest_latency = 0;
        messages_received = 0;
        messages_sent = 0;
        process_state = 1;
        total_latency = 0;
        total_latency_time = 0;
        unexpected_messages = 0;
        xrun_count = 0;
        jack_port_get_latency_range(remote_in_port, JackCaptureLatency,
                                    &in_latency_range);
        jack_port_get_latency_range(remote_out_port, JackPlaybackLatency,
                                    &out_latency_range);
        goto send_message;

    case 1:
        /* State: processing */
        port_buffer = jack_port_get_buffer(in_port, frames);
        event_count = jack_midi_get_event_count(port_buffer);
        last_frame_time = jack_last_frame_time(client);
        for (i = 0; i < event_count; i++) {
            jack_midi_event_get(&event, port_buffer, i);
            message = (messages_received % 2) ? message_2 : message_1;
            if ((event.size == message_size) &&
                (! memcmp(message, event.buffer,
                          message_size * sizeof(jack_midi_data_t)))) {
                goto found_message;
            }
            unexpected_messages++;
        }
        microseconds = jack_frames_to_time(client, last_frame_time) -
            last_activity_time;
        if ((microseconds / 1000000) >= timeout) {
            set_process_error(SOURCE_PROCESS, ERROR_TIMEOUT1);
        }
        break;
    found_message:
        event_time = last_frame_time + event.time;
        frame = event_time - last_activity;
        time = jack_frames_to_time(client, event_time) - last_activity_time;
        if ((! highest_latency) || (frame > highest_latency)) {
            highest_latency = frame;
            highest_latency_time = time;
        }
        if ((! lowest_latency) || (frame < lowest_latency)) {
            lowest_latency = frame;
            lowest_latency_time = time;
        }
        latency_time_values[messages_received] = time;
        latency_values[messages_received] = frame;
        total_latency += frame;
        total_latency_time += time;
        messages_received++;
        if (messages_received == samples) {
            process_state = 2;
            if (! signal_semaphore(process_semaphore)) {
                // Sigh ...
                die(SOURCE_SIGNAL_SEMAPHORE, get_semaphore_error());
            }
            break;
        }
    send_message:
        frame = (jack_nframes_t) ((((double) rand()) / RAND_MAX) * frames);
        if (frame >= frames) {
            frame = frames - 1;
        }
        port_buffer = jack_port_get_buffer(out_port, frames);
        buffer = jack_midi_event_reserve(port_buffer, frame, message_size);
        if (buffer == NULL) {
            set_process_error(SOURCE_EVENT_RESERVE, ERROR_RESERVE);
            break;
        }
        message = (messages_sent % 2) ? message_2 : message_1;
        memcpy(buffer, message, message_size * sizeof(jack_midi_data_t));
        last_activity = jack_last_frame_time(client) + frame;
        last_activity_time = jack_frames_to_time(client, last_activity);
        messages_sent++;

    case 2:
        /* State: finished - do nothing */

    case -1:
        /* State: error - do nothing */
        ;

    }
    return 0;
}

static void
handle_shutdown(void *arg)
{
    set_process_error(SOURCE_SHUTDOWN, ERROR_SHUTDOWN);
}

static int
handle_xrun(void *arg)
{
    xrun_count++;
    return 0;
}

static void
output_error(const char *source, const char *message)
{
    fprintf(stderr, "%s: %s: %s\n", program_name, source, message);
}

static void
output_usage()
{
    fprintf(stderr, "Usage: %s [options] out-port-name in-port-name\n\n"
            "\t-h, --help              print program usage\n"
            "\t-m, --message-size=size set size of MIDI messages to send\n"
            "\t-s, --samples=n         number of MIDI messages to send\n"
            "\t-t, --timeout=seconds   wait time before giving up on message\n"
            "\n", program_name);
}

static unsigned long
parse_positive_number_arg(char *s, char *name)
{
    char *end_ptr;
    unsigned long result;
    errno = 0;
    result = strtoul(s, &end_ptr, 10);
    if (errno) {
        die(name, strerror(errno));
    }
    if (*s == '\0') {
        die(name, "argument value cannot be empty");
    }
    if (*end_ptr != '\0') {
        die(name, "invalid value");
    }
    if (! result) {
        die(name, "must be a positive number");
    }
    return result;
}

static void
set_process_error(const char *source, const char *message)
{
    error_source = source;
    error_message = message;
    process_state = -1;
    if (! signal_semaphore(process_semaphore)) {
        // Sigh
        output_error(source, message);
        die(SOURCE_SIGNAL_SEMAPHORE, get_semaphore_error());
    }
}

static int
signal_semaphore(semaphore_t semaphore)
{

#ifdef WIN32
    return ReleaseSemaphore(semaphore, 1, NULL);
#else
    return ! sem_post(semaphore);
#endif

}

static int
wait_semaphore(semaphore_t semaphore, int block)
{

#ifdef WIN32
    DWORD result = WaitForSingleObject(semaphore, block ? INFINITE : 0);
    switch (result) {
    case WAIT_OBJECT_0:
        return 1;
    case WAIT_TIMEOUT:
        return 0;
    }
    return -1;
#else
    if (block) {
        while (sem_wait(semaphore)) {
            if (errno != EINTR) {
                return -1;
            }
        }
    } else {
        while (sem_trywait(semaphore)) {
            switch (errno) {
            case EAGAIN:
                return 0;
            case EINTR:
                continue;
            default:
                return -1;
            }
        }
    }
    return 1;
#endif

}

int
main(int argc, char **argv)
{
    size_t jitter_plot[101];
    size_t latency_plot[101];
    int long_index = 0;
    struct option long_options[] = {
        {"help", 0, NULL, 'h'},
        {"message-size", 1, NULL, 'm'},
        {"samples", 1, NULL, 's'},
        {"timeout", 1, NULL, 't'}
    };
    char *option_string = "hm:s:t:";
    int show_usage = 0;
    error_message = NULL;
    message_size = 3;
    program_name = argv[0];
    samples = 1024;
    timeout = 5;

    for (;;) {
        char c = getopt_long(argc, argv, option_string, long_options,
                             &long_index);
        switch (c) {
        case 'h':
            show_usage = 1;
            break;
        case 'm':
            message_size = parse_positive_number_arg(optarg, "message-size");
            break;
        case 's':
            samples = parse_positive_number_arg(optarg, "samples");
            break;
        case 't':
            timeout = parse_positive_number_arg(optarg, "timeout");
            break;
        default:
            {
                char *s = "'- '";
                s[2] = c;
                die(s, "invalid switch");
            }
        case -1:
            if (show_usage) {
                output_usage();
                exit(EXIT_SUCCESS);
            }
            goto parse_port_names;
        case 1:
            /* end of switch :) */
            ;
        }
    }
 parse_port_names:
    if ((argc - optind) != 2) {
        output_usage();
        return EXIT_FAILURE;
    }
    latency_values = malloc(sizeof(jack_nframes_t) * samples);
    if (latency_values == NULL) {
        error_message = strerror(errno);
        error_source = "malloc";
        goto show_error;
    }
    latency_time_values = malloc(sizeof(jack_time_t) * samples);
    if (latency_time_values == NULL) {
        error_message = strerror(errno);
        error_source = "malloc";
        goto free_latency_values;
    }
    message_1 = malloc(message_size * sizeof(jack_midi_data_t));
    if (message_1 == NULL) {
        error_message = strerror(errno);
        error_source = "malloc";
        goto free_latency_time_values;
    }
    message_2 = malloc(message_size * sizeof(jack_midi_data_t));
    if (message_2 == NULL) {
        error_message = strerror(errno);
        error_source = "malloc";
        goto free_message_1;
    }
    switch (message_size) {
    case 1:
        message_1[0] = 0xf6;
        message_2[0] = 0xfe;
        break;
    case 2:
        message_1[0] = 0xc0;
        message_1[1] = 0x00;
        message_2[0] = 0xd0;
        message_2[1] = 0x7f;
        break;
    case 3:
        message_1[0] = 0x80;
        message_1[1] = 0x00;
        message_1[2] = 0x00;
        message_2[0] = 0x90;
        message_2[1] = 0x7f;
        message_2[2] = 0x7f;
        break;
    default:
        message_1[0] = 0xf0;
        memset(message_1 + 1, 0,
               (message_size - 2) * sizeof(jack_midi_data_t));
        message_1[message_size - 1] = 0xf7;
        message_2[0] = 0xf0;
        memset(message_2 + 1, 0x7f,
               (message_size - 2) * sizeof(jack_midi_data_t));
        message_2[message_size - 1] = 0xf7;
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

    client = jack_client_open(program_name, JackNullOption, NULL);
    if (client == NULL) {
        error_message = "failed to open JACK client";
        error_source = "jack_client_open";
        goto free_message_2;
    }
    remote_in_port = jack_port_by_name(client, argv[optind + 1]);
    if (remote_in_port == NULL) {
        error_message = "invalid port name";
        error_source = argv[optind + 1];
        goto close_client;
    }
    remote_out_port = jack_port_by_name(client, argv[optind]);
    if (remote_out_port == NULL) {
        error_message = "invalid port name";
        error_source = argv[optind];
        goto close_client;
    }
    in_port = jack_port_register(client, "in", JACK_DEFAULT_MIDI_TYPE,
                                 JackPortIsInput, 0);
    if (in_port == NULL) {
        error_message = "failed to register MIDI-in port";
        error_source = "jack_port_register";
        goto close_client;
    }
    out_port = jack_port_register(client, "out", JACK_DEFAULT_MIDI_TYPE,
                                  JackPortIsOutput, 0);
    if (out_port == NULL) {
        error_message = "failed to register MIDI-out port";
        error_source = "jack_port_register";
        goto unregister_in_port;
    }
    if (jack_set_process_callback(client, handle_process, NULL)) {
        error_message = "failed to set process callback";
        error_source = "jack_set_process_callback";
        goto unregister_out_port;
    }
    if (jack_set_xrun_callback(client, handle_xrun, NULL)) {
        error_message = "failed to set xrun callback";
        error_source = "jack_set_xrun_callback";
        goto unregister_out_port;
    }
    jack_on_shutdown(client, handle_shutdown, NULL);
    jack_set_info_function(handle_info);
    process_state = 0;
    init_semaphore = create_semaphore(0);
    if (init_semaphore == NULL) {
        error_message = get_semaphore_error();
        error_source = "create_semaphore";
        goto unregister_out_port;
    }
    process_semaphore = create_semaphore(1);
    if (process_semaphore == NULL) {
        error_message = get_semaphore_error();
        error_source = "create_semaphore";
        goto destroy_init_semaphore;
    }
    if (jack_activate(client)) {
        error_message = "could not activate client";
        error_source = "jack_activate";
        goto destroy_process_semaphore;
    }
    if (jack_connect(client, jack_port_name(out_port),
                     jack_port_name(remote_out_port))) {
        error_message = "could not connect MIDI out port";
        error_source = "jack_connect";
        goto deactivate_client;
    }
    if (jack_connect(client, jack_port_name(remote_in_port),
                     jack_port_name(in_port))) {
        error_message = "could not connect MIDI in port";
        error_source = "jack_connect";
        goto deactivate_client;
    }
    if (! signal_semaphore(init_semaphore)) {
        error_message = get_semaphore_error();
        error_source = "post_semaphore";
        goto deactivate_client;
    }
    if (wait_semaphore(process_semaphore, 1) == -1) {
        error_message = get_semaphore_error();
        error_source = "wait_semaphore";
        goto deactivate_client;
    }
    if (process_state == 2) {
        double average_latency = ((double) total_latency) / samples;
        double average_latency_time = total_latency_time / samples;
        size_t i;
        double latency_plot_offset =
            floor(((double) lowest_latency_time) / 100.0) / 10.0;
        double sample_rate = (double) jack_get_sample_rate(client);
        jack_nframes_t total_jitter = 0;
        jack_time_t total_jitter_time = 0;
        for (i = 0; i <= 100; i++) {
            jitter_plot[i] = 0;
            latency_plot[i] = 0;
        }
        for (i = 0; i < samples; i++) {
            double latency_time_value = (double) latency_time_values[i];
            double latency_plot_time =
                (latency_time_value / 1000.0) - latency_plot_offset;
            double jitter_time = ABS(average_latency_time -
                                     latency_time_value);
            if (latency_plot_time >= 10.0) {
                (latency_plot[100])++;
            } else {
                (latency_plot[(int) (latency_plot_time * 10.0)])++;
            }
            if (jitter_time >= 10000.0) {
                (jitter_plot[100])++;
            } else {
                (jitter_plot[(int) (jitter_time / 100.0)])++;
            }
            total_jitter += ABS(average_latency -
                                ((double) latency_values[i]));
            total_jitter_time += jitter_time;
        }
        printf("Reported out-port latency: %.2f-%.2f ms (%u-%u frames)\n"
               "Reported in-port latency: %.2f-%.2f ms (%u-%u frames)\n"
               "Average latency: %.2f ms (%.2f frames)\n"
               "Lowest latency: %.2f ms (%u frames)\n"
               "Highest latency: %.2f ms (%u frames)\n"
               "Peak MIDI jitter: %.2f ms (%u frames)\n"
               "Average MIDI jitter: %.2f ms (%.2f frames)\n",
               (out_latency_range.min / sample_rate) * 1000.0,
               (out_latency_range.max / sample_rate) * 1000.0,
               out_latency_range.min, out_latency_range.max,
               (in_latency_range.min / sample_rate) * 1000.0,
               (in_latency_range.max / sample_rate) * 1000.0,
               in_latency_range.min, in_latency_range.max,
               average_latency_time / 1000.0, average_latency,
               lowest_latency_time / 1000.0, lowest_latency,
               highest_latency_time / 1000.0, highest_latency,
               (highest_latency_time - lowest_latency_time) / 1000.0,
               highest_latency - lowest_latency,
               (total_jitter_time / 1000.0) / samples,
               ((double) total_jitter) / samples);
        printf("\nJitter Plot:\n");
        for (i = 0; i < 100; i++) {
            if (jitter_plot[i]) {
                printf("%.1f - %.1f ms: %u\n", ((float) i) / 10.0,
                       ((float) (i + 1)) / 10.0, jitter_plot[i]);
            }
        }
        if (jitter_plot[100]) {
            printf("     > 10 ms: %u\n", jitter_plot[100]);
        }
        printf("\nLatency Plot:\n");
        for (i = 0; i < 100; i++) {
            if (latency_plot[i]) {
                printf("%.1f - %.1f ms: %u\n",
                       latency_plot_offset + (((float) i) / 10.0),
                       latency_plot_offset + (((float) (i + 1)) / 10.0),
                       latency_plot[i]);
            }
        }
        if (latency_plot[100]) {
            printf("     > %.1f ms: %u\n", latency_plot_offset + 10.0,
                   latency_plot[100]);
        }
    }
    printf("\nMessages sent: %d\n"
           "Messages received: %d\n",
           messages_sent, messages_received);
    if (unexpected_messages) {
        printf("Unexpected messages received: %d\n", unexpected_messages);
    }
    if (xrun_count) {
        printf("Xruns: %d (messages may have been lost)\n", xrun_count);
    }
 deactivate_client:
    jack_deactivate(client);
 destroy_process_semaphore:
    destroy_semaphore(process_semaphore, 1);
 destroy_init_semaphore:
    destroy_semaphore(init_semaphore, 0);
 unregister_out_port:
    jack_port_unregister(client, out_port);
 unregister_in_port:
    jack_port_unregister(client, in_port);
 close_client:
    jack_client_close(client);
 free_message_2:
    free(message_2);
 free_message_1:
    free(message_1);
 free_latency_time_values:
    free(latency_time_values);
 free_latency_values:
    free(latency_values);
    if (error_message != NULL) {
    show_error:
        output_error(error_source, error_message);
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}
