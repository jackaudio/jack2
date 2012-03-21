/*
  Copyright (C) 2005 Samuel TRACOL for GRAME

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

/** @file jack_test.c
 *
 * @brief This client test the jack API.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <assert.h>
#include <stdarg.h>
#include <jack/jack.h>
#include <jack/intclient.h>
#include <jack/transport.h>


#if defined(WIN32) && !defined(M_PI)
#define M_PI 3.151592653
#endif

#ifdef WIN32
#define jack_sleep(val) Sleep((val))
#else
#define jack_sleep(val) usleep((val) * 1000)
#endif

typedef struct
{
    jack_nframes_t ft;		// running counter frame time
    jack_nframes_t fcs;		// from sycle start...
    jack_nframes_t lft;		// last frame time...
}
FrameTimeCollector;

FILE *file;
FrameTimeCollector* framecollect;
FrameTimeCollector perpetualcollect;
FrameTimeCollector lastperpetualcollect;
int frames_collected = 0;

// ports
jack_port_t *output_port1;
jack_port_t *output_port1b;
jack_port_t *input_port2;
jack_port_t *output_port2;
jack_port_t *input_port1;

// clients
jack_client_t *client1;
jack_client_t *client2;
const char *client_name1;
const char *client_name2;

unsigned long sr;	// sample rate
// for time -t option
int time_to_run = 0;
int time_before_exit = 1;
// standard error count
int t_error = 0;
int reorder = 0;	// graph reorder callback
int RT = 0;			// is real time or not...
int FW = 0;			// freewheel mode
int init_clbk = 0;	// init callback
int port_rename_clbk = 0;	// portrename callback
int i, j, k = 0;
int port_callback_reg = 0;
jack_nframes_t cur_buffer_size, old_buffer_size, cur_pos;
int activated = 0;
int count1, count2 = 0;			// for freewheel
int xrun = 0;
int have_xrun = 0;				// msg to tell the process1 function to write a special thing in the frametime file.
int process1_activated = -1;	// to control processing...
int process2_activated = -1;	// to control processing...
unsigned long int index1 = 0;
unsigned long int index2 = 0;
jack_default_audio_sample_t *signal1;	// signal source d'emission
jack_default_audio_sample_t *signal2;	// tableau de reception
jack_transport_state_t ts;
jack_position_t pos;
jack_position_t request_pos;
int silent_error = 0;	// jack silent mode
int verbose_mode = 0;
int transport_mode = 1;
jack_nframes_t input_ext_latency = 0;	// test latency for PHY devices
jack_nframes_t output_ext_latency = 0;	// test latency for PHY devices

int sync_called = 0;
int starting_state = 1;

int linecount = 0;		// line counter for log file of sampleframe counter --> for graph function.
int linebuf = 0;		// reminders for graph analysis
int linetransport = 0;
int linefw = 0;
int lineports = 0;
int linecl2 = 0;

int client_register = 0;

/**
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

							Callbacks & basics functions

*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*/

void usage()
{
    fprintf (stderr, "\n\n"
             "usage: jack_test \n"
             "              [ --time OR -t time_to_run (in seconds) ]\n"
             "              [ --quiet OR -q (quiet mode : without jack server errors) ]\n"
             "              [ --verbose OR -v (verbose mode : no details on tests done. Only main results & errors) ]\n"
             "              [ --transport OR -k (Do not test transport functions.) ]\n"
             "                --realtime OR -R (jack is in rt mode)\n\n\n"
            );
    exit(1);
}

void Log(const char *fmt, ...)
{
    if (verbose_mode) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
}

void Collect(FrameTimeCollector* TheFrame)
{
    TheFrame->lft = jack_last_frame_time(client1);
    TheFrame->ft = jack_frame_time(client1);
    TheFrame->fcs = jack_frames_since_cycle_start(client1);
}

void Jack_Thread_Init_Callback(void *arg)
{
#ifdef WIN32
    Log("Init callback has been successfully called from thread = %x. (msg from callback)\n", GetCurrentThread());
#else
    Log("Init callback has been successfully called from thread = %x. (msg from callback)\n", pthread_self());
#endif
    init_clbk = 1;
}

void Jack_Freewheel_Callback(int starting, void *arg)
{
    Log("Freewheel callback has been successfully called with value %i. (msg from callback)\n", starting);
    FW = starting;
}

void Jack_Client_Registration_Callback(const char* name, int val, void *arg)
{
    Log("Client registration callback name = %s has been successfully called with value %i. (msg from callback)\n", name, val);
	if (val)
		client_register++;
	else
		client_register--;
}

int Jack_Port_Rename_Callback(jack_port_id_t port, const char* old_name, const char* new_name, void *arg)
{
     Log("Rename callback has been successfully called with old_name '%s' and new_name '%s'. (msg from callback)\n", old_name, new_name);
     port_rename_clbk = 1;
     return 0;
}

int Jack_Update_Buffer_Size(jack_nframes_t nframes, void *arg)
{
    cur_buffer_size = jack_get_buffer_size(client1);
    Log("Buffer size = %d (msg from callback)\n", cur_buffer_size);
    return 0;
}

int Jack_XRun_Callback(void *arg)
{
    xrun++;
    have_xrun = 1;
    Log("Xrun has been detected ! (msg from callback)\n");
    return 0;
}

int Jack_Graph_Order_Callback(void *arg)
{
    reorder++;
    return 0;
}

int Jack_Sample_Rate_Callback(jack_nframes_t nframes, void *arg)
{
    Log("Sample rate : %i.\n", nframes);
    return 0;
}

void Jack_Error_Callback(const char *msg)
{
    if (silent_error == 0) {
        fprintf(stderr, "error : %s (msg from callback)\n", msg);
    }
}

void jack_shutdown(void *arg)
{
    printf("Jack_test has been kicked out by jackd !\n");
    exit(1);
}

void jack_info_shutdown(jack_status_t code, const char* reason, void *arg)
{
    printf("JACK server failure : %s\n", reason);
    exit(1);
}

void Jack_Port_Register(jack_port_id_t port, int mode, void *arg)
{
    port_callback_reg++;
}

void Jack_Port_Connect(jack_port_id_t a, jack_port_id_t b, int connect, void* arg)
{
	Log("PortConnect src = %ld dst = %ld  onoff = %ld (msg from callback)\n", a, b, connect);
}

int Jack_Sync_Callback(jack_transport_state_t state, jack_position_t *pos, void *arg)
{
    int res = 0;

    switch (state) {

        case JackTransportStarting:
            sync_called++;
            if (starting_state == 0) {
                Log("sync callback : Releasing status : now ready...\n");
                res = 1;
            } else {
                if (sync_called == 1) {
                    Log("sync callback : Holding status...\n");
                }
                res = 0;
            }
            break;

        case JackTransportStopped:
            Log("sync callback : JackTransportStopped...\n");
            res = 0;
            break;

        default:
            res = 0;
            break;
    }

    return res;
}


/**
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

							processing functions

*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Proccess1 is for client1
 * 4 modes, activated with process1_activated
 *
 * -1 : idle mode
 *  0 : write zeros to output 1
 *  1 : write continuously signal1 (sinusoidal test signal) to output1
 *  3 : mode for summation test. While record (done by process2) is not running, write signal1 to both out1 & out1b.
 *      when record begin (index2 > 0), write signal1 in phase opposition to out1 & out2
 *  5 : Frames Time checking mode : write the array containing the three values of frame_time, frames cycles start and
 *      last frame time during 150 cycles.
 */

int process1(jack_nframes_t nframes, void *arg)
{
    if (FW == 0) {
        Collect(&perpetualcollect);
        if (have_xrun) {
            fprintf(file, "%i	%i\n", (perpetualcollect.ft - lastperpetualcollect.ft), (2*cur_buffer_size));
            have_xrun = 0;
        } else {
            fprintf(file, "%i	0\n", (perpetualcollect.ft - lastperpetualcollect.ft));
        }
        linecount++;
        lastperpetualcollect.ft = perpetualcollect.ft;
    }

    jack_default_audio_sample_t *out1;
    jack_default_audio_sample_t *out1b;
    activated++; // counter of callback activation
    if (process1_activated == 1) {
        out1 = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port1, nframes);
        for (jack_nframes_t p = 0; p < nframes; p++) {
            out1[p] = signal1[index1];
            index1++;
            if (index1 == 48000)
                index1 = 0;
        }
    }
    if (process1_activated == 3) {
        out1 = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port1, nframes);
        out1b = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port1b, nframes);
        for (jack_nframes_t p = 0; p < nframes; p++) {
            out1[p] = signal1[index1];
            if (index2 != 0) {
                out1b[p] = ( -1 * signal1[index1]);
            } else {
                out1b[p] = signal1[index1];
            }
            index1++;
            if (index1 == 48000)
                index1 = 0;
        }
    }
    if (process1_activated == 0) {
        out1 = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port1, nframes);
        memset (out1, 0, sizeof (jack_default_audio_sample_t) * nframes); //�crit des z�ros en sortie...
    }
    if (process1_activated == 5) {
        Collect(&framecollect[frames_collected]);
        frames_collected++;
        if (frames_collected > 798) {
            process1_activated = -1;
        }
    }
    return 0;
}

/**
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Proccess2 is for client2
 * 3 modes, activated with process1_activated
 *
 * -1 : idle mode
 *  0 : idle mode
 *  1 : record in2 into signal2.(for first transmit test)
 *  2 : record in2 into signal2 while send signal1 in out2. used dor Tie data test.
 *  3 : record in2 into sigal2 for summation data test.
 * In records modes, at the end of the record (signal2 is full), it stop the test, setting both activation states to -1.
 */

int process2(jack_nframes_t nframes, void *arg)
{
    jack_default_audio_sample_t *out2;
    jack_default_audio_sample_t *in2;

    if (process2_activated == 1) { // Reception du process1 pour comparer les donnees
        in2 = (jack_default_audio_sample_t *) jack_port_get_buffer (input_port2, nframes);
        for (unsigned int p = 0; p < nframes; p++) {
            signal2[index2] = in2[p];
            if (index2 == 95999) {
                process2_activated = 0;
                process1_activated = 0;
                //index2 = 0;
            } else {
                index2++;
            }
        }
    }

    if (process2_activated == 2) { // envoie de signal1 pour test tie mode et le r�cup�re direct + latence de la boucle jack...
        out2 = (jack_default_audio_sample_t *) jack_port_get_buffer (output_port2, nframes);
        in2 = (jack_default_audio_sample_t *) jack_port_get_buffer (input_port2, nframes);

        for (unsigned int p = 0; p < nframes; p++) {
            out2[p] = signal1[index1];
            index1++;
            if (index1 == 48000)
                index1 = 0;
            signal2[index2] = in2[p];
            if (index2 == 95999) {
                process2_activated = -1;
                //index2 = 0;
            } else {
                index2++;
            }
        }
    }

    if (process2_activated == 3) { // envoie de -signal1 pour sommation en oppo de phase par jack
        in2 = (jack_default_audio_sample_t *) jack_port_get_buffer (input_port2, nframes);

        for (unsigned int p = 0; p < nframes;p++) {
            signal2[index2] = in2[p];
            if (index2 == 95999) {
                process2_activated = 0;
                process1_activated = 0;
                //index2 = 0;
            } else {
                index2++;
            }
        }
    }

    return 0;
}

// Alternate thread model
static int _process (jack_nframes_t nframes)
{
	jack_default_audio_sample_t *in, *out;
	in = (jack_default_audio_sample_t *)jack_port_get_buffer (input_port1, nframes);
	out = (jack_default_audio_sample_t *)jack_port_get_buffer (output_port1, nframes);
	memcpy (out, in,
		sizeof (jack_default_audio_sample_t) * nframes);
	return 0;
}

static void* jack_thread(void *arg)
{
	jack_client_t* client = (jack_client_t*) arg;
	jack_nframes_t last_thread_time = jack_frame_time(client);

	while (1) {
		jack_nframes_t frames = jack_cycle_wait(client);
		jack_nframes_t current_thread_time = jack_frame_time(client);
		jack_nframes_t delta_time = current_thread_time - last_thread_time;
		Log("jack_thread : delta_time = %ld\n", delta_time);
		int status = _process(frames);
		last_thread_time = current_thread_time;
		jack_cycle_signal (client, status);
	}

	return 0;
}

// To test callback exiting
int process3(jack_nframes_t nframes, void *arg)
{
	static int process3_call = 0;

	if (process3_call++ > 10) {
		Log("process3 callback : exiting...\n");
		return -1;
	} else {
		Log("calling process3 callback : process3_call = %ld\n", process3_call);
		return 0;
	}
}

int process4(jack_nframes_t nframes, void *arg)
{
	jack_client_t* client = (jack_client_t*) arg;

	static jack_nframes_t last_time = jack_frame_time(client);
	static jack_nframes_t tolerance = (jack_nframes_t)(cur_buffer_size * 0.1f);

	jack_nframes_t cur_time = jack_frame_time(client);
	jack_nframes_t delta_time = cur_time - last_time;

	Log("calling process4 callback : jack_frame_time = %ld delta_time = %ld\n", cur_time, delta_time);
	if (delta_time > 0  && (jack_nframes_t)abs(delta_time - cur_buffer_size) > tolerance) {
		printf("!!! ERROR !!! jack_frame_time seems to return incorrect values cur_buffer_size = %d, delta_time = %d tolerance %d\n", cur_buffer_size, delta_time, tolerance);
	}

	last_time = cur_time;
	return 0;
}

int process5(jack_nframes_t nframes, void *arg)
{
	jack_client_t* client = (jack_client_t*) arg;
    
    static jack_nframes_t first_current_frames;
    static jack_time_t first_current_usecs;
    static jack_time_t first_next_usecs;
    static float first_period_usecs;
	static int res1 = jack_get_cycle_times(client, &first_current_frames, &first_current_usecs, &first_next_usecs, &first_period_usecs);
	   
    jack_nframes_t current_frames;
    jack_time_t current_usecs;
    jack_time_t next_usecs;
    float period_usecs;

    int res = jack_get_cycle_times(client, &current_frames, &current_usecs, &next_usecs, &period_usecs);
    if (res != 0) {
        printf("!!! ERROR !!! jack_get_cycle_times fails...\n");
        return 0;
    }
    
	Log("calling process5 callback : jack_get_cycle_times delta current_frames = %ld delta current_usecs = %ld delta next_usecs = %ld period_usecs = %f\n", 
        current_frames - first_current_frames, current_usecs - first_current_usecs, next_usecs - first_next_usecs, period_usecs);
 
    first_current_frames = current_frames;
    first_current_usecs = current_usecs;
    first_next_usecs = next_usecs;
	return 0;
}

static void display_transport_state()
{
    jack_transport_state_t ts;
    jack_position_t pos;

    ts = jack_transport_query(client2, &pos);
    switch (ts) {
        case JackTransportStopped:
            Log("Transport is stopped...\n");
            break;
        case JackTransportRolling:
            Log("Transport is rolling...\n");
            break;
        case JackTransportLooping:
            Log("Transport is looping...\n");
            break;
        case JackTransportStarting:
            Log("Transport is starting...\n");
            break;
        case JackTransportNetStarting:
            Log("Transport is starting with network sync...\n");
            break;
    }
}

/**
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
							MAIN FUNCTION
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
*/
int main (int argc, char *argv[])
{
    const char **inports; // array of PHY input/output
    const char **outports; // array of PHY input/outputs
    const char *server_name = NULL;
	const char **connexions1;
    const char **connexions2;
    jack_status_t status;
    char portname[128] = "port";
    char filename[128] = "framefile.ext";
    const char *nullportname = "";
    int option_index;
    int opt;
    int a = 0;			// working number for in/out port (PHY)...
    int test_link = 0;	// for testing the "overconnect" function
    int flag;			// flag for ports...
    int is_mine = 0;	// to test jack_port_is_mine function...
    const char *options = "kRnqvt:";
    float ratio;		// for speed calculation in freewheel mode
    jack_options_t jack_options = JackNullOption;
	struct option long_options[] = {
                                       {"realtime", 0, 0, 'R'},
                                       {"non-realtime", 0, 0, 'n'},
                                       {"time", 0, 0, 't'},
                                       {"quiet", 0, 0, 'q'},
                                       {"verbose", 0, 0, 'v'},
                                       {"transport", 0, 0, 'k'},
                                       {0, 0, 0, 0}
                                   };

    client_name1 = "jack_test";
    time_to_run = 1;
    //verbose_mode = 1;
    //RT = 1;
    while ((opt = getopt_long (argc, argv, options, long_options, &option_index)) != EOF) {
        switch (opt) {
            case 'k':
                transport_mode = 0;
                break;
            case 'q':
                silent_error = 1;
                break;
            case 'v':
                verbose_mode = 1;
                printf("Verbose mode is activated...\n");
                break;
            case 't':
                time_to_run = atoi(optarg);
                break;
            case 'R':
                RT = 1;
                break;
            default:
                fprintf (stderr, "unknown option %c\n", opt);
                usage ();
        }
    }

    if (RT) {
        printf("Jack server is said being in realtime mode...\n");
    } else {
        printf("Jack server is said being in non-realtime mode...\n");
    }
    /**
    *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
    *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
    						init signal data for test
    *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
    *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
    */
    framecollect = (FrameTimeCollector *) malloc(800 * sizeof(FrameTimeCollector));

    signal1 = (jack_default_audio_sample_t *) malloc(48000 * sizeof(jack_default_audio_sample_t));
    signal2 = (jack_default_audio_sample_t *) malloc(96000 * sizeof(jack_default_audio_sample_t));
    signal1[0] = 0;
    int p;
    for (p = 1; p < 48000;p++) {
        signal1[p] = (float)(sin((p * 2 * M_PI * 1000 ) / 48000));
    }
    for (p = 0; p < 95999;p++) {
        signal2[p] = 0.0 ;
    }
    index1 = 0;
    index2 = 0;
    /**
    *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
    *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
    						begin test
    *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
    *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
    */
    printf("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*--*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
    printf("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*--*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
    printf("*-*-*-*-*-*-*-*-*-*-*-*-*-* Start jack server stress test  *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
    printf("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*--*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
    printf("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*--*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");

    /**
     * Register a client...
     *
     */
	Log("Register a client using jack_client_open()...\n");
    client1 = jack_client_open(client_name1, jack_options, &status, server_name);
    if (client1 == NULL) {
        fprintf (stderr, "jack_client_open() failed, "
                 "status = 0x%2.0x\n", status);
        if (status & JackServerFailed) {
            fprintf(stderr, "Unable to connect to JACK server\n");
        }
        exit (1);
    }
    if (status & JackServerStarted) {
        fprintf(stderr, "JACK server started\n");
    }

    /**
     * Internal client tests...
     *
     */
    jack_intclient_t intclient;

    Log("trying to load the \"inprocess\" server internal client \n");

    intclient = jack_internal_client_load (client1, "inprocess",
                                           (jack_options_t)(JackLoadName|JackLoadInit),
                                           &status, "inprocess", "");

    if (intclient == 0 || status & JackFailure) {
        printf("!!! ERROR !!! cannot load internal client \"inprocess\" intclient 0x%llX status 0x%2.0x !\n", (unsigned long long)intclient, status);
	} else {

        Log("\"inprocess\" server internal client loaded\n");

        char* internal_name = jack_get_internal_client_name(client1, intclient);
        if (strcmp(internal_name, "inprocess") == 0) {
            Log("jack_get_internal_client_name returns %s\n", internal_name);
        } else {
            printf("!!! ERROR !!! jack_get_internal_client_name returns incorrect name %s\n", internal_name);
        }

        jack_intclient_t intclient1 = jack_internal_client_handle(client1, "inprocess", &status);
        if (intclient1 == intclient) {
            Log("jack_internal_client_handle returns correct handle\n");
        } else {
            printf("!!! ERROR !!! jack_internal_client_handle returns incorrect handle 0x%llX\n", (unsigned long long)intclient1);
        }

        // Unload internal client
        status = jack_internal_client_unload (client1, intclient);
        if (status  == 0) {
            Log("jack_internal_client_unload done first time returns correct value\n");
        } else {
            printf("!!! ERROR !!! jack_internal_client_unload returns incorrect value 0x%2.0x\n", status);
        }

        // Unload internal client second time
        status = jack_internal_client_unload (client1, intclient);
        if (status & JackFailure &&  status & JackNoSuchClient) {
            Log("jack_internal_client_unload done second time returns correct value\n");
        } else {
            printf("!!! ERROR !!! jack_internal_client_unload returns incorrect value 0x%2.0x\n", status);
        }
    }


    /**
     * try to register another one with the same name...
     *
     */
    Log("trying to register a new jackd client with name %s using jack_client_new()...\n", client_name1);
    client2 = jack_client_new(client_name1);
    if (client2 == NULL) {
        Log ("valid : a second client with the same name cannot be registered\n");
    } else {
        printf("!!! ERROR !!! Jackd server has accepted multiples client with the same name !\n");
        jack_client_close(client2);
    }

	/**
     * try to register another one with the same name using jack_client_open ==> since JackUseExactName is not used, an new client should be opened...
     *
     */
    Log("trying to register a new jackd client with name %s using jack_client_open()...\n", client_name1);
    client2 = jack_client_open(client_name1, jack_options, &status, server_name);
    if (client2 != NULL) {
        Log ("valid : a second client with the same name can be registered (client automatic renaming)\n");
		jack_client_close(client2);
    } else {
        printf("!!! ERROR !!! Jackd server automatic renaming feature does not work!\n");
    }

    /**
     * testing client name...
     * Verify that the name sended at registration and the one returned by jack server is the same...
     *
     */
    Log("Testing name...");
    client_name2 = jack_get_client_name(client1);
    if (strcmp(client_name1, client_name2) == 0)
        Log(" ok\n");
    else
        printf("\n!!! ERROR !!! name returned different from the one given : %s\n", client_name2);

    /**
     * Test RT mode...
     * verify if the real time mode returned by jack match the optional argument defined when launching jack_test*/
    if (jack_is_realtime(client1) == RT)
        Log("Jackd is in realtime mode (RT = %i).\n", RT);
    else
        printf("!!! ERROR !!! Jackd is in a non-expected realtime mode (RT = %i).\n", RT);

    /**
     * Register all callbacks...
     *
     */
    if (jack_set_thread_init_callback(client1, Jack_Thread_Init_Callback, 0) != 0)
        printf("!!! ERROR !!! while calling jack_set_thread_init_callback()...\n");
    if (jack_set_freewheel_callback(client1, Jack_Freewheel_Callback, 0) != 0 )
        printf("\n!!! ERROR !!! while calling jack_set_freewheel_callback()...\n");


    if (jack_set_process_callback(client1, process1, 0) != 0) {
        printf("Error when calling jack_set_process_callback() !\n");
    }

    jack_on_shutdown(client1, jack_shutdown, 0);

    if (jack_on_info_shutdown)
        jack_on_info_shutdown(client1, jack_info_shutdown, 0);

    if (jack_set_buffer_size_callback(client1, Jack_Update_Buffer_Size, 0) != 0) {
        printf("Error when calling buffer_size_callback !\n");
    }

    if (jack_set_graph_order_callback(client1, Jack_Graph_Order_Callback, 0) != 0) {
        printf("Error when calling Jack_Graph_Order_Callback() !\n");
    }

    if (jack_set_port_rename_callback(client1, Jack_Port_Rename_Callback, 0) != 0 )
        printf("\n!!! ERROR !!! while calling jack_set_rename_callback()...\n");

    if (jack_set_xrun_callback(client1, Jack_XRun_Callback, 0 ) != 0) {
        printf("Error when calling jack_set_xrun_callback() !\n");
    }

    if (jack_set_sample_rate_callback(client1, Jack_Sample_Rate_Callback, 0 ) != 0) {
        printf("Error when calling Jack_Sample_Rate_Callback() !\n");
    }

    if (jack_set_port_registration_callback(client1, Jack_Port_Register, 0) != 0) {
        printf("Error when calling jack_set_port_registration_callback() !\n");
    }

    if (jack_set_port_connect_callback(client1, Jack_Port_Connect, 0) != 0) {
        printf("Error when calling jack_set_port_connect_callback() !\n");
    }

	if (jack_set_client_registration_callback(client1, Jack_Client_Registration_Callback, 0) != 0) {
		printf("Error when calling jack_set_client_registration_callback() !\n");
	}

    jack_set_error_function(Jack_Error_Callback);

    /**
     * Create file for clock "frame time" analysis
     *
     */
    cur_buffer_size = jack_get_buffer_size(client1);
    sprintf (filename, "framefile-%i.dat", cur_buffer_size);
    file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Erreur dans l'ouverture du fichier log framefile.dat");
        exit(-1);
    }

    /**
     * Try to register a client with a NULL name/zero length name...
     *
     */
    output_port1 = jack_port_register(client1, nullportname,
                                      JACK_DEFAULT_AUDIO_TYPE,
                                      JackPortIsOutput, 0);
    if (output_port1 == NULL) {
        Log("Can't register a port with a NULL portname... ok.\n");
    } else {
        printf("!!! ERROR !!! Can register a port with a NULL portname !\n");
        jack_port_unregister(client1, output_port1);
    }

    /**
     * Register 1 port in order to stress other functions.
     *
     */
    output_port1 = jack_port_register(client1, portname,
                                      JACK_DEFAULT_AUDIO_TYPE,
                                      JackPortIsOutput, 0);
    if (output_port1 == NULL) {
        printf("!!! ERROR !!! Can't register any port for the client !\n");
        exit(1);
    }

   /**
     * Test port type of the just registered port.
     *
     */
    if (strcmp(jack_port_type(output_port1), JACK_DEFAULT_AUDIO_TYPE) != 0) {
        printf("!!! ERROR !!! jack_port_type returns an incorrect value!\n");
	} else {
		Log("Checking jack_port_type()... ok.\n");
	}

    /**
     * Try to register another port with the same name...
     *
     */
    output_port2 = jack_port_register(client1, portname,
                                      JACK_DEFAULT_AUDIO_TYPE,
                                      JackPortIsOutput, 0);
    if (output_port2 == NULL) {
        Log("Can't register two ports with the same name... ok\n");
    } else {
        if (strcmp (jack_port_name(output_port1), jack_port_name(output_port2)) == 0) {
            printf("!!! ERROR !!! Can register two ports with the same name ! (%px : %s & %px : %s).\n", output_port1, jack_port_name(output_port1), output_port2, jack_port_name(output_port2));
            jack_port_unregister(client1, output_port2);
        } else {
            Log("Can't register two ports with the same name... ok (auto-rename %s into %s).\n", jack_port_name(output_port1), jack_port_name(output_port2));
            jack_port_unregister(client1, output_port2);
        }
    }

    /**
     * Verify that both port_name and port_short_name return correct results...
     *
     */
    sprintf (portname, "%s:%s", jack_get_client_name(client1), jack_port_short_name(output_port1));
    if (strcmp(jack_port_name(output_port1), portname) != 0) {
        printf("!!! ERROR !!! functions jack_port_name and/or jack_short_port_name seems to be invalid !\n");
        printf("client_name = %s\n short_port_name = %s\n port_name = %s\n", jack_get_client_name(client1), jack_port_short_name(output_port1), jack_port_name(output_port1));
    }

    /**
     * Verify the function port_set_name
     *
     */
    if (jack_port_set_name (output_port1, "renamed-port#") == 0 ) {
        if (strcmp(jack_port_name(output_port1), "renamed-port#") == 0) {
            printf("!!! ERROR !!! functions jack_port_set_name seems to be invalid !\n");
            printf("jack_port_name return '%s' whereas 'renamed-port#' was expected...\n", jack_port_name(output_port1));
        } else {
            Log("Checking jack_port_set_name()... ok\n");
            jack_port_set_name (output_port1, "port");
        }
    } else {
        printf("error : port_set_name function can't be tested...\n");
    }

    port_callback_reg = 0;	// number of port registration received by the callback

    /**
     * Activate the client
     *
     */
    if (jack_activate(client1) < 0) {
        printf ("Fatal error : cannot activate client1\n");
        exit(1);
    }

    /**
     * Test if portrename callback have been called.
     *
     */
    jack_port_set_name (output_port1, "renamed-port#");
    jack_sleep(1 * 1000);

    if (port_rename_clbk == 0)
        printf("!!! ERROR !!! Jack_Port_Rename_Callback was not called !!.\n");


    /**
     * Test if portregistration callback have been called.
     *
     */

    jack_sleep(1 * 1000);

    if (1 == port_callback_reg) {
        Log("%i ports have been successfully created, and %i callback reg ports have been received... ok\n", 1, port_callback_reg);
    } else {
        printf("!!! ERROR !!! %i ports have been created, and %i callback reg ports have been received !\n", 1, port_callback_reg);
    }

    /**
     * Test if init callback initThread have been called.
     *
     */
    if (init_clbk == 0)
        printf("!!! ERROR !!! Jack_Thread_Init_Callback was not called !!.\n");

    jack_sleep(10 * 1000); // test see the clock in the graph at the begining...

    /**
     * Stress Freewheel mode...
     * Try to enter freewheel mode. Check the realtime mode de-activation.
     * Check that the number of call of the process callback is greater than in non-freewheel mode.
     * Give an approximated speed ratio (number of process call) between the two modes.
     * Then return in normal mode.
     */
    t_error = 0;
    activated = 0;
    jack_sleep(1 * 1000);
    count1 = activated;
    Log("Testing activation freewheel mode...\n");
    linefw = linecount; // count for better graph reading with gnuplot
    jack_set_freewheel(client1, 1);
    activated = 0;
    jack_sleep(1 * 1000);
    count2 = activated;
    if (jack_is_realtime(client1) == 0) {
        t_error = 0;
    } else {
        printf("\n!!! ERROR !!! RT mode is always activated while freewheel mode is applied !\n");
        t_error = 1;
    }
    if (activated == 0)
        printf("!!! ERROR !!! Freewheel mode doesn't activate audio callback !!\n");

    jack_set_freewheel(client1, 0);
    jack_sleep(7 * 1000);

	if (jack_is_realtime(client1) == 1) {}
    else {
        printf("\n!!! ERROR !!! freewheel mode fail to reactivate RT mode when exiting !\n");
        t_error = 1;
    }
    if (t_error == 0) {
        Log("Freewheel mode appears to work well...\n");
    }
    if (count1 == 0) {
        Log("Audio Callback in 'standard' (non-freewheel) mode seems not to be called...\n");
        Log("Ratio speed would be unavailable...\n");
    } else {
        ratio = (float) ((count2 - count1) / count1);
        Log("Approximative speed ratio of freewheel mode = %f : 1.00\n", ratio);
    }

    /**
     * Stress buffer function...
     * get current buffer size.
     * Try to apply a new buffer size value ( 2 x the precedent buffer size value)
     * Then return in previous buffer size mode.
     *
     */

    float factor = 0.5f;
    old_buffer_size = jack_get_buffer_size(client1);
    Log("Testing BufferSize change & Callback...\n--> Current buffer size : %d.\n", old_buffer_size);
    linebuf = linecount;
    if (jack_set_buffer_size(client1, (jack_nframes_t)(old_buffer_size * factor)) < 0) {
        printf("!!! ERROR !!! jack_set_buffer_size fails !\n");
    }
    jack_sleep(1 * 1000);
    cur_buffer_size = jack_get_buffer_size(client1);
    if (abs((old_buffer_size * factor) - cur_buffer_size) > 5) {  // Tolerance needed for dummy driver...
        printf("!!! ERROR !!! Buffer size has not been changed !\n");
        printf("!!! Maybe jack was compiled without the '--enable-resize' flag...\n");
    } else {
        Log("jack_set_buffer_size() command successfully applied...\n");
    }
    jack_sleep(3 * 1000);
    jack_set_buffer_size(client1, old_buffer_size);
    cur_buffer_size = jack_get_buffer_size(client1);

    /**
     * Test the last regestered port to see if port_is_mine function the right value.
     * A second test will be performed later.
     * The result will be printed at the end.
     *
     */
    if (jack_port_is_mine(client1, output_port1)) {
        is_mine = 1;
    } else {
        is_mine = 0;
    }

    /**
     * Check that the ID returned by the port_by_name is right.
     * (it seems there is a problem here in some jack versions).
     *
    */
    if (output_port1 != jack_port_by_name(client1, jack_port_name(output_port1))) {
        printf("!!! ERROR !!! function jack_port_by_name() return bad value !\n");
        printf("!!! jack_port_by_name(jack_port_name(_ID_) ) != _ID_returned_at_port_registering ! (%px != %px)\n", jack_port_by_name(client1, jack_port_name(output_port1)), output_port1);
    } else {
        Log("Checking jack_port_by_name() return value... ok\n");
    }
    if (NULL != jack_port_by_name(client1, jack_port_short_name(output_port1))) {
        printf("!!! ERROR !!! function jack_port_by_name() return a value (%px) while name is incomplete !\n", jack_port_by_name(client1, jack_port_short_name(output_port1)));
    } else {
        Log("Checking jack_port_by_name() with bad argument... ok (returned id 0)\n");
    }

    /**
    * remove the output port previously created
    * no more ports should subsist here for our client.
    *
    */
    if (jack_port_unregister(client1, output_port1) != 0) {
        printf("!!! ERROR !!! while unregistering port %s.\n", jack_port_name(output_port1));
    }

    /**
     * list all in ports
     *
     */
    inports = jack_get_ports(client1, NULL, NULL, 0);

    /**
     * Test the first PHY (physical) connection to see if it's "mine".
     * and report the result in the test that began before.
     * The result is printed later.
     *
     */
    if (jack_port_is_mine(client1, jack_port_by_name(client1, inports[0]))) {
        is_mine = 0;
    }

    /**
     * List all devices' flags and print them...
     *
     */
    Log("\nTry functions jack_get_ports, jack_port_flag & jack_port_by_name to list PHY devices...\n");
    Log("-----------------------------------------------------------\n");
    Log("---------------------------DEVICES-------------------------\n");
    Log("-----------------------------------------------------------\n");
    a = 0;
    while (inports[a] != NULL) {
        flag = jack_port_flags(jack_port_by_name(client1, inports[a]) );
        Log("   * %s (id : %i)\n", inports[a], jack_port_by_name(client1, inports[a]));
        Log("    (");
        if (flag & JackPortIsInput)
            Log("JackPortIsInput ");
        if (flag & JackPortIsOutput)
            Log("JackPortIsOutput ");
        if (flag & JackPortIsPhysical)
            Log("JackPortIsPhysical ");
        if (flag & JackPortCanMonitor)
            Log("JackPortCanMonitor ");
        if (flag & JackPortIsTerminal)
            Log("JackPortIsTerminal ");
        Log(")\n\n");
        a++;
    }
    Log("-----------------------------------------------------------\n\n");

    /**
     * list all PHY in/out ports...
     * This list will be used later many times.
     *
     */
    outports = jack_get_ports(client1, NULL, NULL, JackPortIsPhysical | JackPortIsOutput);
    inports = jack_get_ports(client1, NULL, NULL, JackPortIsPhysical | JackPortIsInput);

    if (outports == NULL) {
        printf("!!! WARNING !!! no physical capture ports founded !\n");
    }
    if (inports == NULL) {
        printf("!!! WARNING !!! no physical output ports founded !\n");
    }

    /**
     * Brute test : try to create as many ports as possible.
     * It stops when jack returns an error.
     * Then try to connect each port to physical entry...
     * Check also that graph reorder callback is called.
     *
     */
    Log("Registering as many ports as possible and connect them to physical entries...\n");
    lineports = linecount;
    t_error = 0;

    i = 0;	// number of couple 'input-ouput'
    j = 0;	// number of ports created
    port_callback_reg = 0;	// number of port registration received by the callback
    reorder = 0;			// number of graph reorder callback activation
    test_link = 0 ;			// Test the "overconnect" function only one time
    while (t_error == 0) {
        sprintf (portname, "input_%d", i);
        input_port1 = jack_port_register(client1, portname,
                                         JACK_DEFAULT_AUDIO_TYPE,
                                         JackPortIsInput, 0);
        j++;
        if (input_port1 == NULL) {
            j--;
            t_error = 1;
        } else {
            // Connect created input to PHY output
            a = 0;
            while (outports[a] != NULL) {
                if (jack_connect(client1, outports[a], jack_port_name(input_port1))) {
                    printf ("error : cannot connect input PHY port to client port %s\n", jack_port_name(input_port1));
                } else {
                    // printf ("input PHY port %s connected to client port %s\n", outports[a], jack_port_name(input_port1));
                }
                a++;
            }
            // Try one time to "overconnect" 2 ports (the latest created)...
            if (test_link == 0) {
                if (jack_connect(client1, outports[a - 1], jack_port_name(input_port1)) == EEXIST) {
                    // cannot over-connect input PHY port to client port. ok.
                    test_link = 1;
                }
            }
        }

        sprintf(portname, "output_%d", i);
        output_port1 = jack_port_register(client1, portname,
                                          JACK_DEFAULT_AUDIO_TYPE,
                                          JackPortIsOutput, 0);
        j++;
        if (output_port1 == NULL) {
            t_error = 1;
            j--;
        } else {
            // Connect created input to PHY output
            a = 0;
            while (inports[a] != NULL) {
                if (jack_connect(client1, jack_port_name(output_port1), inports[a])) {
                    printf ("error : cannot connect input PHY port %s to client port %s\n", inports[a], jack_port_name(output_port1));
                } else {
                    // output PHY port %s connected to client port. ok.
                }
                a++;
            }
            // Try one time to "overconnect" 2 ports (the latest created)...
            if (test_link == 0) {
                if (jack_connect(client1, jack_port_name(output_port1), inports[a - 1]) == EEXIST) {
                    // cannot over-connect output PHY port to client port. ok.
                    test_link = 1;
                }
            }
        }
        i++;
    }

    jack_sleep(1 * 1000); // To hope all port registration and reorder callback have been received...

    // Check port registration callback
    if (j == port_callback_reg) {
        Log("%i ports have been successfully created, and %i callback reg ports have been received... ok\n", j, port_callback_reg);
    } else {
        printf("!!! ERROR !!! %i ports have been created, and %i callback reg ports have been received !\n", j, port_callback_reg);
    }

    if (reorder == (2 * j)) {
        Log("%i graph reorder callback have been received... ok\n", reorder);
    } else {
        printf("!!! ERROR !!! %i graph reorder callback have been received (maybe non-valid value)...\n", reorder);
    }
    /**
     * print basic test connection functions result ...
     * over-connected means here that we try to connect 2 ports that are already connected.
     *
     */
    if (test_link) {
        Log("Jack links can't be 'over-connected'... ok\n");
    } else {
        printf("!!! ERROR !!! Jack links can be 'over-connected'...\n");
    }
    /**
     * Print the result of the two jack_is_mine test.
     *
     */
    if (is_mine == 1) {
        Log("Checking jack_port_is_mine()... ok\n");
    } else {
        printf("!!! ERROR !!! jack_port_is_mine() function seems to send non-valid datas !\n");
    }
    /**
     * Free the array of the physical input and ouput ports.
     * (as mentionned in the doc of jack_get_ports)
     *
     */
    jack_free(inports);
    jack_free(outports);

    /**
     * Try to "reactivate" the client whereas it's already activated...
     *
     */
    if (jack_activate(client1) < 0) {
        printf("!!! ERROR !!! Cannot activate client1 a second time...\n");
        exit(1);
    } else {
        Log("jackd server accept client.jack_activate() re-activation (while client was already activated).\n");
    }

    /**
     * Deregister all ports previously created.
     *
     */
    port_callback_reg = 0; // to check registration callback
    Log("Deregistering all ports of the client...\n");
    inports = jack_get_ports(client1, NULL, NULL, 0);
    a = 0;
    while (inports[a] != NULL) {
        flag = jack_port_flags(jack_port_by_name(client1, inports[a]));
        input_port1 = jack_port_by_name(client1, inports[a]);
        if (jack_port_is_mine(client1, input_port1)) {
            if (jack_port_unregister(client1, input_port1) != 0) {
                printf("!!! ERROR !!! while unregistering port %s.\n", jack_port_name(output_port1));
            }
        }
        a++;
    }

    // Check port registration callback again
    if (j == port_callback_reg) {
        Log("%i ports have been successfully created, and %i callback reg ports have been received... ok\n", j, port_callback_reg);
    } else {
        printf("!!! ERROR !!! %i ports have been created, and %i callback reg ports have been received !\n", j, port_callback_reg);
    }

    jack_free(inports); // free array of ports (as mentionned in the doc of jack_get_ports)

    /**
     *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
     *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
     				Open a new client (second one) to test some other things...
     *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
     *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
     */

    Log("\n\n----------------------------------------------------------------------\n");
    Log("Starting second new client 'jack_test_#2'...\n");
    /* open a client connection to the JACK server */
    client_name2 = "jack_test_#2";
    linecl2 = linecount; // reminders for graph analysis
    client2 = jack_client_new(client_name2);

    if (client2 == NULL) {
        fprintf(stderr, "jack_client_new() failed for %s.\n"
                "status = 0x%2.0x\n", client_name2, status);
        if (status & JackServerFailed) {
            fprintf(stderr, "Unable to connect client2 to JACK server\n");
        }
        exit(1);
    }

	// Check client registration callback
	jack_sleep(1000);
	if (client_register == 0)
		printf("!!! ERROR !!! Client registration callback not called!\n");

    /**
     * Register callback for this client.
     * Callbacks are the same as the first client for most of them, excepted for process audio callback.
     *
     */
    jack_set_port_registration_callback(client2, Jack_Port_Register, 0);

    jack_set_process_callback(client2, process2, 0);

    jack_on_shutdown(client2, jack_shutdown, 0);

    /**
     * Register one input and one output for each client.
     *
     */
    Log("registering 1 input/output ports for each client...\n");

    output_port1 = jack_port_register(client1, "out1",
                                      JACK_DEFAULT_AUDIO_TYPE,
                                      JackPortIsOutput, 0);
    output_port2 = jack_port_register(client2, "out2",
                                      JACK_DEFAULT_AUDIO_TYPE,
                                      JackPortIsOutput, 0);
    input_port1 = jack_port_register(client1, "in1",
                                     JACK_DEFAULT_AUDIO_TYPE,
                                     JackPortIsInput, 0);
    input_port2 = jack_port_register(client2, "in2",
                                     JACK_DEFAULT_AUDIO_TYPE,
                                     JackPortIsInput, 0);
    if ((output_port1 == NULL) || (output_port2 == NULL) || (input_port1 == NULL) || (input_port2 == NULL)) {
        printf("!!! ERROR !!! Unable to register ports...\n");
    }

    /**
     * Set each process mode to idle and activate client2
     *
     */
    process2_activated = -1;
    process1_activated = -1;
    if (jack_activate(client2) < 0) {
        printf ("Fatal error : cannot activate client2\n");
        exit (1);
    }

    /**
     * Connect the two clients and check that all connections are well-done.
     *
     */
    Log("Testing connections functions between clients...\n");
    if (jack_connect(client1, jack_port_name(output_port1), jack_port_name(input_port2)) != 0) {
        printf("!!! ERROR !!! while client1 intenting to connect ports...\n");
    }
    if (jack_connect(client2, jack_port_name(output_port2), jack_port_name(input_port1)) != 0) {
        printf("!!! ERROR !!! while client2 intenting to connect ports...\n");
    }
    if (jack_connect(client1, jack_port_name(output_port1), jack_port_name(input_port1)) != 0) {
        printf("!!! ERROR !!! while client1 intenting to connect ports...\n");
    }

    /**
     * Test the port_connected function...
     *
     */
    if ((jack_port_connected(output_port1) == jack_port_connected(input_port1)) &&
            (jack_port_connected(output_port2) == jack_port_connected(input_port2)) &&
            (jack_port_connected(output_port2) == 1) &&
            (jack_port_connected(output_port1) == 2)
       ) {
        Log("Checking jack_port_connected()... ok.\n");
    } else {
        printf("!!! ERROR !!! function jack_port_connected() return a bad value !\n");
        printf("jack_port_connected(output_port1) %d\n", jack_port_connected(output_port1));
        printf("jack_port_connected(output_port2) %d\n", jack_port_connected(output_port2));
        printf("jack_port_connected(input_port1) %d\n", jack_port_connected(input_port1));
        printf("jack_port_connected(input_port2) %d\n", jack_port_connected(input_port2));
    }

    /**
     * Test a new time the port_by_name function...(now we are in multi-client mode)
     *
     */
    Log("Testing again jack_port_by_name...\n");
    if (output_port1 != jack_port_by_name(client1, jack_port_name(output_port1))) {
        printf("!!! ERROR !!! function jack_port_by_name() return bad value in a multi-client application!\n");
        printf("!!! jack_port_by_name(jack_port_name(_ID_) ) != _ID_ in multiclient application.\n");
    } else {
        Log("Checking jack_port_by_name() function with a multi-client application... ok\n");
    }

    /**
     * Test the port_connected_to function...
     *
     */
    if ((jack_port_connected_to (output_port1, jack_port_name(input_port2))) &&
            (!(jack_port_connected_to (output_port2, jack_port_name(input_port2))))) {
        Log("checking jack_port_connected_to()... ok\n");
    } else {
        printf("!!! ERROR !!! jack_port_connected_to() return bad value !\n");
    }

    /**
     * Test the port_get_connections & port_get_all_connections functions...
     *
     */
    Log("Testing jack_port_get_connections and jack_port_get_all_connections...\n");
    a = 0;
    t_error = 0;
    connexions1 = jack_port_get_connections (output_port1);
    connexions2 = jack_port_get_all_connections(client1, output_port1);
    if ((connexions1 == NULL) || (connexions2 == NULL)) {
        printf("!!! ERROR !!! port_get_connexions or port_get_all_connexions return a NULL pointer !\n");
    } else {
        while ((connexions1[a] != NULL) && (connexions2[a] != NULL) && (t_error == 0)) {
            t_error = strcmp(connexions1[a], connexions2[a]);
            a++;
        }

        if (t_error == 0) {
            Log("Checking jack_port_get_connections Vs jack_port_get_all_connections... ok\n");
        } else {
            printf("!!! ERROR !!! while checking jack_port_get_connections Vs jack_port_get_all_connections...\n");
        }
    }
    a = 0;
    t_error = 0;
    inports = jack_get_ports(client1, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
    connexions1 = NULL;
    assert(inports != NULL);
    if (inports[0] != NULL) {
        connexions1 = jack_port_get_connections (jack_port_by_name(client1, inports[0]));
        connexions2 = jack_port_get_all_connections(client1, jack_port_by_name(client1, inports[0]));
    }

    jack_free (inports);
    if (connexions1 == NULL) {
        Log("checking jack_port_get_connections() for external client... ok\n");
    } else {
        while ((connexions1[a] != NULL) && (connexions2[a] != NULL) && (t_error == 0)) {
            t_error = strcmp(connexions1[a], connexions2[a]);
            a++;
        }
    }
    if (t_error == 0) {
        Log("Checking jack_port_get_connections() Vs jack_port_get_all_connections() on PHY port... ok\n");
    } else {
        printf("!!! ERROR !!! while checking jack_port_get_connections() Vs jack_port_get_all_connections() on PHY port...\n");
    }

	if (jack_disconnect(client1, jack_port_name(output_port1), jack_port_name(input_port1)) != 0) {
        printf("!!! ERROR !!! while client1 intenting to disconnect ports...\n");
    }
    if (jack_disconnect(client1, jack_port_name(output_port2), jack_port_name(input_port1)) != 0) {
        printf("!!! ERROR !!! while client1 intenting to disconnect ports...\n");
    }
    // No links should subsist now...

    /**
     * Checking data connexion
     * establishing a link between client1.out1 --> client2.in2
     * Send the signal1 test on out1. Record the result into signal2. (see process functions).
    ---------------------------------------------------------------------------*/
    Log("Testing connections datas between clients...\n");
    jack_connect(client2, jack_port_name(output_port1), jack_port_name(input_port2) );
    process2_activated = -1;
    process1_activated = -1;
    Log("process 2 : idle mode...\n");
    Log("Sending datas...");
    index1 = 0;
    index2 = 0;
    process1_activated = 1; // We start emitting first.
    process2_activated = 1; // So record begin at least when we just begin to emitt the signal, else at next call of process with
    // nframe = jack buffersize shifting.

    while (process2_activated == 1) {
        jack_sleep(1 * 1000);
        Log(".");
    }
    index2 = 0;
    Log("\nAnalysing datas...\n"); // search the first occurence of the first element of the reference signal in the recorded signal
    while (signal2[index2] != signal1[1] ) {
        index2++;
        if (index2 == 95999) {
            printf("!!! ERROR !!! Data not found in first connexion data check!\n");
            break;
        }
    }
    index1 = index2;
    Log("Data founded at offset %i.\n", index2);
    // And now we founded were the recorded data are, we can see if the two signals matches...
    while ( (signal2[index2] == signal1[index2 - index1 + 1]) || (index2 == 95999) || ((index2 - index1 + 1) == 47999) ) {
        index2++;
    }
    Log("Checking difference between datas... %i have the same value...\n", index2 - index1);
    if ((index2 - index1) == 48000) {
        Log("Data received are valid...\n");
    } else {
        printf("!!! ERROR !!! data transmission seems not to be valid in first connexion data check!\n");
    }
    if (jack_disconnect(client1, jack_port_name(output_port1), jack_port_name(input_port2) ) != 0)
        // no more connection between ports exist now...
    {
        printf("Error while establishing new connexion (disconnect).\n");
    }

    /**
     * Test TIE MODE
     * (This mode seems to be problematic in standard jack version 0.100. It seems that nobody
     * is used to apply this mode because the tie mode doesn't work at all. A patch seems difficult to produce
     * in this version of jack. Tie mode work well in MP version.)
     * Test some basic thinks (tie with 2 differents client, tie non-owned ports...)
     * Tie client1.in1 and client1.out1 ports, and make some data test to check the validity of the tie.
     *
     */
    Log("Testing tie mode...\n");
    if (jack_port_tie(input_port1, output_port2) != 0) {
        Log("not possible to tie two ports from two differents clients... ok\n");
    } else {
        printf("!!! ERROR !!! port_tie has allowed a connexion between two differents clients !\n");
        jack_port_untie(output_port2);
    }
    Log("Testing connections datas in tie mode...\n");
    int g;
    for (g = 0; g < 96000; g++)
        signal2[g] = 0.0;
    // Create a loop (emit test) client2.out2----client.in1--tie--client1.out1-----client2.in1 (receive test)
    if (jack_port_tie(input_port1, output_port1) != 0) {
        printf("Unable to tie... fatal error : data test will not be performed on tie mode !!\n");
    } else { // begin of tie
        if (jack_connect(client1, jack_port_name(output_port1), jack_port_name(input_port2)) != 0) {
            printf("!!! ERROR !!! while client1 intenting to connect ports...\n");
        }
        if (jack_connect(client1, jack_port_name(output_port2), jack_port_name(input_port1)) != 0) {
            printf("!!! ERROR !!! while client1 intenting to connect ports...\n");
        }

        process1_activated = -1;
        process2_activated = -1;

        //		We can manualy check here that the tie is effective.
        //		ie : playing a wav with a client, connecting ports manualy with qjackctl, and listen...
        // 		printf("manual test\n");
        // 		jack_sleep(50);
        // 		printf("end of manual test\n");

        index1 = 0;
        index2 = 0;
        process1_activated = -1;
        process2_activated = 2;

        Log("Sending datas...");

        while (process2_activated == 2) {
            jack_sleep(1 * 1000);
            Log(".");
        }
        process1_activated = -1;
        process2_activated = -1;
        index2 = 0;
        Log("\nAnalysing datas...\n");
        // We must find at least 2 identical values to ensure we are at the right place in the siusoidal array...
        while (!((signal2[index2] == signal1[1]) && (signal2[index2 + 1] == signal1[2]))) {
            index2++;
            if (index2 == 95999) {
                printf("!!! ERROR !!! Data not found in connexion check of tie mode!\n");
                break;
            }
        }
        index1 = index2;
        Log("Tie mode : Data founded at offset %i.\n", index2);
        while (signal2[index2] == signal1[index2 - index1 + 1]) {
            index2++;
            if ((index2 == 95999) || ((index2 - index1 + 1) == 47999)) {
                break;
            }
        }
        Log("Checking difference between datas... %i have the same value...\n", index2 - index1);
        if ((index2 - index1) > 47995) {
            Log("Data received in tie mode are valid...\n");
        } else {
            // in tie mode, the buffers adress should be the same for the two tied ports.
            printf("!!! ERROR !!! data transmission seems not to be valid !\n");
            printf("Links topology : (emitt) client2.out2 ----> client1.in1--(tie)--client1.out1----->client2.in2 (recive)\n");
            printf("  port_name    : Port_adress \n");
            printf("  output_port1 : %px\n", jack_port_get_buffer(output_port1, cur_buffer_size));
            printf("  input_port2  : %px\n", jack_port_get_buffer(input_port2, cur_buffer_size));
            printf("  output_port2 : %px\n", jack_port_get_buffer(output_port2, cur_buffer_size));
            printf("  input_port1  : %px\n", jack_port_get_buffer(input_port1, cur_buffer_size));
        }

        jack_port_untie(output_port1);
        jack_port_disconnect(client1, output_port2);
        jack_port_disconnect(client1, output_port1);

    } //end of tie


    /**
     * Testing SUMMATION CAPABILITIES OF JACK CONNECTIONS
     *
     * In a short test, we just check a simple summation in jack.
     * A first client(client1) send two signal in phase opposition
     * A second client(client2) record the summation at one of his port
     * So, the result must be zero...
     * See process1 for details about steps of this test
     *
     */
    // fprintf(file, "Sum test\n");
    Log("Checking summation capabilities of patching...\n");
    output_port1b = jack_port_register(client1, "out1b",
                                       JACK_DEFAULT_AUDIO_TYPE,
                                       JackPortIsOutput, 0);
    jack_connect(client2, jack_port_name(output_port1), jack_port_name(input_port2));
    jack_connect(client2, jack_port_name(output_port1b), jack_port_name(input_port2));

    process1_activated = 3;
    process2_activated = -1;
    for (g = 0; g < 96000; g++)
        signal2[g] = 0.0;
    index1 = 0;
    index2 = 0;

    Log("Sending datas...");
    process2_activated = 3;

    while (process2_activated == 3) {
        jack_sleep(1 * 1000);
        Log(".");
    }
    process1_activated = -1;
    process2_activated = -1;
    index2 = 0;
    Log("\nAnalysing datas...\n"); // same idea as above, with first data check...
    while (!((signal2[index2] == 0.0 ) && (signal2[(index2 + 1)] == 0.0 ))) {
        index2++;
        if (index2 == 95999) {
            printf("!!! ERROR !!! Data not found in summation check!\n");
            break;
        }
    }
    index1 = index2;
    Log("Data founded at offset %i.\n", index2);

    while ( signal2[index2] == 0.0 ) {
        index2++;
        if ((index2 > 95998) || ((index2 - index1 + 1) > 47998)) {
            break;
        }
    }
    Log("Checking difference between datas...\n");
    if ((index2 - index1) > 47996) {
        Log("Data mixed received are valid...\nSummation is well done.\n");
    } else {
        printf("!!! ERROR !!! data transmission / summation seems not to be valid !\n");
    }
    jack_port_disconnect(client1, output_port1);
    jack_port_disconnect(client1, output_port1b);
    jack_port_unregister(client1, output_port1b);

    if (jack_port_name(output_port1b) != NULL ) {
        printf("!!! WARNING !!! port_name return something while the port have been unregistered !\n");
        printf("!!! Name of unregistered port : %s !\n", jack_port_name(output_port1b));
    } else {
        Log("Checking jack_port_name() with a non valid port... ok\n");
    }

    if (jack_port_set_name(output_port1b, "new_name") == 0 ) {
        printf("!!! WARNING !!! An unregistered port can be renamed successfully !\n");
    } else {
        Log("Checking renaming of an unregistered port... ok\n");
    }
    inports = jack_get_ports(client1, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
    if (jack_port_set_name(jack_port_by_name(client1, inports[0]), "new_name") == 0 ) {
        printf("!!! WARNING !!! A PHYSICAL port can be renamed successfully !\n");
    } else {
        Log("Checking renaming of an unregistered port... ok\n");
    }
    jack_free (inports);


    /**
     * Checking latency issues
     * here are simple latency check
     * We simply check that the value returned by jack seems ok
     * Latency compensation is a difficult point.
     * Actually, jack is not able to see "thru" client to build a full latency chain.
     * Ardour use theses informations to do internally his compensations.
     *
     * 3 test are done : one with no connections between client, one with a serial connection, and one with parallel connection
     */
    Log("Checking about latency functions...\n");
    t_error = 0;
    jack_recompute_total_latencies(client1);
    Log("jack_recompute_total_latencies...\n");
    if ((jack_port_get_latency (output_port1) != 0) ||
            (jack_port_get_total_latency(client1, output_port1) != 0) ) {
        t_error = 1;
        printf("!!! ERROR !!! default latency of a non-PHY device is not set to zero !\n");
    }

    inports = jack_get_ports(client1, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
    outports = jack_get_ports(client1, NULL, NULL, JackPortIsPhysical | JackPortIsOutput);
    if (inports[0] != NULL) {
        output_ext_latency = jack_port_get_latency (jack_port_by_name(client1, inports[0]));  // from client to out driver (which has "inputs" ports..)
		input_ext_latency = jack_port_get_latency (jack_port_by_name(client1, outports[0]));  // from in driver (which has "output" ports..) to client
        if (output_ext_latency != jack_port_get_total_latency(client1, jack_port_by_name(client1, inports[0]))) {
            t_error = 1;
            printf("!!! ERROR !!! get_latency & get_all_latency for a PHY device (unconnected) didn't return the same value !\n");
        }
        Log("Checking a serial model with 2 clients...\n");

        jack_connect(client1, jack_port_name(output_port1), jack_port_name(input_port2));
        jack_connect(client1, outports[0], jack_port_name(input_port1));
        jack_connect(client2, jack_port_name(output_port2), inports[0]);
        jack_port_set_latency(output_port2, 256);
        jack_recompute_total_latencies(client1);

        if ((jack_port_get_latency (output_port1) != 0) ||
                (jack_port_get_total_latency(client1, output_port1) != 0) ||
                (jack_port_get_latency (jack_port_by_name(client1, inports[0])) != (output_ext_latency)) ||
                (jack_port_get_total_latency(client1, jack_port_by_name(client1, inports[0])) != (output_ext_latency + 256)) ||
                (jack_port_get_total_latency(client1, output_port2) != (output_ext_latency + 256)) ||
                (jack_port_get_total_latency(client1, input_port2) != 0) ||
                (jack_port_get_total_latency(client1, input_port1) != input_ext_latency) ||
                (jack_port_get_latency (jack_port_by_name(client1, outports[0])) != input_ext_latency) ||
                (jack_port_get_total_latency(client1, jack_port_by_name(client1, outports[0])) != input_ext_latency)
           ) {
            printf("!!! WARNING !!! get_latency functions may have a problem : bad value returned !\n");
            printf("!!! get_latency(output_port1) : %i (must be 0)\n", jack_port_get_latency(output_port1));
            printf("!!! get_total_latency(output_port1) : %i (must be 0)\n", jack_port_get_total_latency(client1, output_port1));
            printf("!!! get_latency(PHY[0]) : %i (must be external latency : %i)\n", jack_port_get_latency(jack_port_by_name(client1, inports[0])), output_ext_latency);
            printf("!!! get_total_latency(PHY[0]) : %i (must be %i)\n", jack_port_get_total_latency(client1, jack_port_by_name(client1, inports[0])) , (output_ext_latency + 256));
            printf("!!! get_total_latency(output_port2) : %i (must be %i)\n", jack_port_get_total_latency(client1, output_port2), (output_ext_latency + 256));
            printf("!!! get_total_latency(input_port2) : %i (must be 0)\n", jack_port_get_total_latency(client1, input_port2));
            printf("!!! get_total_latency(input_port1) : %i (must be %i)\n", jack_port_get_total_latency(client1, input_port1), input_ext_latency);
            printf("!!! get_latency(PHY[0]) : %i (must be %i)\n", jack_port_get_latency(jack_port_by_name(client1, outports[0])), input_ext_latency);
            printf("!!! get_total_latency(PHY[0]) : %i (must be %i)\n", jack_port_get_total_latency(client1, jack_port_by_name(client1, outports[0])), input_ext_latency);

        } else {
            Log("get_latency & get_total_latency seems quite ok...\n");
        }

        jack_port_disconnect(client1, output_port1);
	    jack_port_disconnect(client1, output_port2);
		jack_port_disconnect(client1, input_port1);
	    jack_port_disconnect(client1, input_port2);
	    Log("Checking a parallel model with 2 clients...\n");
        jack_connect(client2, outports[0], jack_port_name(input_port1));
        jack_connect(client2, outports[0], jack_port_name(input_port2));
        jack_connect(client2, jack_port_name(output_port1), inports[0]);
        jack_connect(client2, jack_port_name(output_port2), inports[0]);
        jack_port_set_latency(output_port1, 256);
        jack_port_set_latency(output_port2, 512);
        jack_recompute_total_latencies(client1);

        if ((jack_port_get_latency(output_port1) != 256 ) ||
			(jack_port_get_total_latency(client1, output_port1) != (256 + output_ext_latency)) ||
			(jack_port_get_latency(output_port2) != 512) ||
			(jack_port_get_total_latency(client1, output_port2) != (512 + output_ext_latency)) ||
			(jack_port_get_latency(jack_port_by_name(client1, inports[0])) != output_ext_latency) ||
			(jack_port_get_total_latency(client1, jack_port_by_name(client1, inports[0])) != (512 + output_ext_latency))
           ) {
            printf("!!! WARNING !!! get_latency functions may have a problem : bad value returned !\n");
			printf("!!! get_latency(output_port1) : %i (must be 256)\n", jack_port_get_latency(output_port1));
			printf("!!! get_total_latency(output_port1) : %i (must be 256 + output_ext_latency)\n", jack_port_get_total_latency(client1, output_port1));
			printf("!!! get_latency(output_port2) : %i (must 512)\n", jack_port_get_latency(output_port2));
			printf("!!! get_total_latency(output_port2) : %i (must 512 + output_ext_latency)\n", jack_port_get_total_latency(client1, output_port2));
			printf("!!! get_latency(inports[0])) : %i (must output_ext_latency)\n", jack_port_get_latency(jack_port_by_name(client1, inports[0])));
			printf("!!! get_total_latency(inports[0]) : %i (must 512 + output_ext_latency)\n", jack_port_get_total_latency(client1, jack_port_by_name(client1, inports[0])));
        } else {
            Log("get_latency & get_total_latency seems quite ok...\n");
        }
    } else {
        printf("No physical port founded : not able to test latency functions...");
    }

    jack_port_disconnect(client1, input_port1);
    jack_port_disconnect(client1, input_port2);
    jack_port_disconnect(client1, output_port1);
    jack_port_disconnect(client1, output_port2);

	jack_sleep(1000);

    jack_free(inports);
    jack_free(outports);

    /**
     * Checking transport API.
     * Simple transport test.
     * Check a transport start with a "slow" client, simulating a delay around 1 sec before becoming ready.
     *
     */
	Log("-----------------------------------------------------------\n");
    Log("---------------------------TRANSPORT-----------------------\n");
    Log("-----------------------------------------------------------\n");

    lineports = linecount;

    if (transport_mode) {
        int wait_count;
        ts = jack_transport_query(client1, &pos);
        if (ts == JackTransportStopped) {
            Log("Transport is stopped...\n");
        } else {
            jack_transport_stop(client1);
            Log("Transport state : %i\n", ts);
        }
        if (jack_set_sync_callback(client2, Jack_Sync_Callback, 0) != 0)
            printf("error while calling set_sync_callback...\n");

        Log("starting transport...\n");

        starting_state = 1; // Simulate starting state
        jack_transport_start(client1);

        // Wait until sync callback is called
        while (!(sync_called)) {
            jack_sleep(1 * 1000);
        }

        // Wait untill rolling : simulate sync time out
        Log("Simulate a slow-sync client exceeding the time-out\n");
        wait_count = 0;

        do {
            jack_sleep(100); // Wait 100 ms each cycle
            wait_count++;
            if (wait_count == 100) {
                Log("!!! ERROR !!! max time-out exceedeed : sync time-out does not work correctly\n");
                break;
            }
            ts = jack_transport_query(client2, &pos);
            Log("Waiting....pos = %ld\n", pos.frame);
            display_transport_state();

        } while (ts != JackTransportRolling);

        Log("Sync callback have been called %i times.\n", sync_called);
        jack_transport_stop(client1);

        // Wait until stopped
        ts = jack_transport_query(client2, &pos);
        while (ts != JackTransportStopped) {
            jack_sleep(1 * 1000);
            ts = jack_transport_query(client2, &pos);
        }

        // Simulate starting a slow-sync client that rolls after 0.5 sec
        Log("Simulate a slow-sync client that needs 0.5 sec to start\n");
        sync_called = 0;
        wait_count = 0;
        starting_state = 1; // Simulate starting state

        Log("Starting transport...\n");
        jack_transport_start(client1);
        display_transport_state();

        Log("Waiting 0.5 sec...\n");
        jack_sleep(500);
        starting_state = 0; // Simulate end of starting state after 0.5 sec

        // Wait untill rolling
        ts = jack_transport_query(client2, &pos);
        while (ts != JackTransportRolling) {
            jack_sleep(100); // Wait 100 ms each cycle
            wait_count++;
            if (wait_count == 10) {
                Log("!!! ERROR !!! starting a slow-sync client does not work correctly\n");
                break;
            }
            ts = jack_transport_query(client2, &pos);
        }

        if (sync_called == 0)
            Log("!!! ERROR !!! starting a slow-sync client does not work correctly\n");

        Log("Sync callback have been called %i times.\n", sync_called);
        display_transport_state();

        // Test jack_transport_locate while rolling
        Log("Test jack_transport_locate while rolling\n");
        ts = jack_transport_query(client2, &pos);
        Log("Transport current frame = %ld\n", pos.frame);
        jack_nframes_t cur_frame = pos.frame;

        wait_count = 0;
        do {
            display_transport_state();
            jack_sleep(10);  // 10 ms
            // locate at first...
            wait_count++;
            if (wait_count == 1) {
                Log("Do jack_transport_locate\n");
                jack_transport_locate(client1, cur_frame / 2);
            } else if (wait_count == 100) {
                break;
            }
            ts = jack_transport_query(client2, &pos);
            Log("Locating.... frame = %ld\n", pos.frame);
        } while (pos.frame > cur_frame);

        ts = jack_transport_query(client2, &pos);
        Log("Transport current frame = %ld\n", pos.frame);
        if (wait_count == 100) {
            printf("!!! ERROR !!! jack_transport_locate does not work correctly\n");
        }

        // Test jack_transport_reposition while rolling
        Log("Test jack_transport_reposition while rolling\n");
        ts = jack_transport_query(client2, &pos);
        Log("Transport current frame = %ld\n", pos.frame);
        cur_frame = pos.frame;

        wait_count = 0;
        do {
            display_transport_state();
            jack_sleep(10);  // 10 ms
            // locate at first...
            wait_count++;
            if (wait_count == 1) {
                Log("Do jack_transport_reposition\n");
                request_pos.frame = cur_frame / 2;
                jack_transport_reposition(client1, &request_pos);
            } else if (wait_count == 100) {
                break;
            }
            ts = jack_transport_query(client2, &pos);
            Log("Locating.... frame = %ld\n", pos.frame);
        } while (pos.frame > cur_frame);

        ts = jack_transport_query(client2, &pos);
        Log("Transport current frame = %ld\n", pos.frame);
        if (wait_count == 100) {
            printf("!!! ERROR !!! jack_transport_reposition does not work correctly\n");
        }

        // Test jack_transport_reposition while stopped
        jack_transport_stop(client1);
        ts = jack_transport_query(client2, &pos);
        Log("Transport current frame = %ld\n", pos.frame);

        Log("Test jack_transport_reposition while stopped\n");
        wait_count = 0;
        request_pos.frame = 10000;
        jack_transport_reposition(client1, &request_pos);

        do {
            display_transport_state();
            jack_sleep(100); // 100 ms
            if (wait_count++ == 10)
                break;
            ts = jack_transport_query(client2, &pos);
            Log("Locating.... frame = %ld\n", pos.frame);
        } while (pos.frame != 10000);

        ts = jack_transport_query(client2, &pos);
        Log("Transport current frame = %ld\n", pos.frame);
        if (pos.frame != 10000) {
            printf("!!! ERROR !!! jack_transport_reposition does not work correctly\n");
        }

        jack_transport_stop(client1);

        /* Tell the JACK server that we are ready to roll.  Our
         * process() callback will start running now. */

    } else {
        printf("Transport check is disabled...\n");
    }

    time_before_exit = time_to_run;
    while (time_before_exit != 0) {
        jack_sleep (1 * 1000);
        time_before_exit--;
    }

	if (jack_deactivate(client2) != 0) {
        printf("!!! ERROR !!! jack_deactivate does not return 0 for client2 !\n");
    }
    if (jack_deactivate(client1) != 0) {
        printf("!!! ERROR !!! jack_deactivate does not return 0 for client1 !\n");
    }

	/**
     * Checking jack_frame_time.
    */
	Log("Testing jack_frame_time...\n");
	jack_set_process_callback(client1, process4, client1);
	jack_activate(client1);
	jack_sleep(2 * 1000);
    
    /**
     * Checking jack_get_cycle_times.
    */
    Log("Testing jack_get_cycle_times...\n");
    jack_deactivate(client1);
	jack_set_process_callback(client1, process5, client1);
	jack_activate(client1);
	jack_sleep(3 * 1000);
    

	/**
     * Checking alternate thread model
    */
	Log("Testing alternate thread model...\n");
	jack_deactivate(client1);
	jack_set_process_callback(client1, NULL, NULL);  // remove callback
	jack_set_process_thread(client1, jack_thread, client1);
	jack_activate(client1);
	jack_sleep(2 * 1000);

	/**
     * Checking callback exiting : when the return code is != 0, the client is desactivated.
    */
	Log("Testing callback exiting...\n");
	jack_deactivate(client1);
	jack_set_process_thread(client1, NULL, NULL); // remove thread callback
	jack_set_process_callback(client1, process3, 0);
	jack_activate(client1);
	jack_sleep(3 * 1000);

    /**
     *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
     *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
     *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
     						Closing program
    *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
    *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
    *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	*/

    if (jack_deactivate(client2) != 0) {
        printf("!!! ERROR !!! jack_deactivate does not return 0 for client2 !\n");
    }
    if (jack_deactivate(client1) != 0) {
        printf("!!! ERROR !!! jack_deactivate does not return 0 for client1 !\n");
    }
    if (jack_client_close(client2) != 0) {
        printf("!!! ERROR !!! jack_client_close does not return 0 for client2 !\n");
    }
    if (jack_client_close(client1) != 0) {
        printf("!!! ERROR !!! jack_client_close does not return 0 for client1 !\n");
    }

    if (xrun == 0) {
        Log("No Xrun have been detected during this test... cool !\n");
    } else {
        printf("%i Xrun have been detected during this session (seen callback messages to see where are the problems).\n", xrun);
    }
    free(framecollect);
    free(signal1);
    free(signal2);
    Log("Exiting jack_test...\n");
    fclose(file);
    printf("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*--*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n");
    sprintf (filename, "framegraph-%i.gnu", cur_buffer_size);
    file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Erreur dans l'ouverture du fichier");
        exit( -1);
    }
    fprintf(file, "reset\n");
    fprintf(file, "set terminal png transparent nocrop enhanced\n");
    fprintf(file, "set output 'framegraph-%i-1.png'\n", cur_buffer_size);
    fprintf(file, "set title \"Frame time evolution during jack_test run\"\n");
    fprintf(file, "set yrange [ %i.00000 : %i.0000 ] noreverse nowriteback\n", cur_buffer_size - (cur_buffer_size / 8), cur_buffer_size + (cur_buffer_size / 8));
    fprintf(file, "set xrange [ 0.00000 : %i.0000 ] noreverse nowriteback\n" , linecount - 1);
    fprintf(file, "set ylabel \"Frametime evolution (d(ft)/dt)\"\n");
    fprintf(file, "set xlabel \"FrameTime\"\n");
    fprintf(file, "set label \"| buf.siz:%i | fr.wl:%i | rg.ports:%i | 2nd.client:%i | trsprt:%i |\" at graph 0.01, 0.04\n", linebuf, linefw, lineports, linecl2, linetransport);
    fprintf(file, "plot 'framefile-%i.dat' using 2 with impulses title \"Xruns\",'framefile-%i.dat' using 1 with line title \"Sampletime variation at %i\"\n", cur_buffer_size, cur_buffer_size, cur_buffer_size);

    fprintf(file, "set output 'framegraph-%i-2.png'\n", cur_buffer_size);
    fprintf(file, "set title \"Frame time evolution during jack_test run\"\n");
    fprintf(file, "set yrange [ %i.00000 : %i.0000 ] noreverse nowriteback\n", (int) (cur_buffer_size / 2), (int) (2*cur_buffer_size + (cur_buffer_size / 8)));
    fprintf(file, "set xrange [ 0.00000 : %i.0000 ] noreverse nowriteback\n" , linecount - 1);
    fprintf(file, "set ylabel \"Frametime evolution (d(ft)/dt)\"\n");
    fprintf(file, "set xlabel \"FrameTime\"\n");
    fprintf(file, "set label \"| buf.siz:%i | fr.wl:%i | rg.ports:%i | 2nd.client:%i | trsprt:%i |\" at graph 0.01, 0.04\n", linebuf, linefw, lineports, linecl2, linetransport);
    fprintf(file, "plot 'framefile-%i.dat' using 2 with impulses title \"Xruns\",'framefile-%i.dat' using 1 with line title \"Sampletime variation at %i\"\n", cur_buffer_size, cur_buffer_size, cur_buffer_size);
    fclose(file);
    return 0;
}
