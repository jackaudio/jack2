/** @file simple_client.c
 *
 * @brief This simple client demonstrates the basic features of JACK
 * as they would be used by many applications.
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <math.h>

#include <jack/jack.h>
#include <jack/jslist.h>
#include "memops.h"

#include "alsa/asoundlib.h"

#include <samplerate.h>

// Here are the lists of the jack ports...

JSList	   *capture_ports = NULL;
JSList	   *capture_srcs = NULL;
JSList	   *playback_ports = NULL;
JSList	   *playback_srcs = NULL;
jack_client_t *client;

snd_pcm_t *alsa_handle;

int jack_sample_rate;
int jack_buffer_size;

int quit = 0;
double resample_mean = 1.0;
double static_resample_factor = 1.0;
double resample_lower_limit = 0.25;
double resample_upper_limit = 4.0;

double *offset_array;
double *window_array;
int offset_differential_index = 0;

double offset_integral = 0;

// ------------------------------------------------------ commandline parameters

int sample_rate = 0;				 /* stream rate */
int num_channels = 2;				 /* count of channels */
int period_size = 1024;
int num_periods = 2;

int target_delay = 0;	    /* the delay which the program should try to approach. */
int max_diff = 0;	    /* the diff value, when a hard readpointer skip should occur */
int catch_factor = 100000;
int catch_factor2 = 10000;
double pclamp = 15.0;
double controlquant = 10000.0;
int smooth_size = 256;
int good_window=0;
int verbose = 0;
int instrument = 0;
int samplerate_quality = 2;

// Debug stuff:

volatile float output_resampling_factor = 1.0;
volatile int output_new_delay = 0;
volatile float output_offset = 0.0;
volatile float output_integral = 0.0;
volatile float output_diff = 0.0;
volatile int running_freewheel = 0;

snd_pcm_uframes_t real_buffer_size;
snd_pcm_uframes_t real_period_size;

// buffers

char *tmpbuf;
char *outbuf;
float *resampbuf;

// format selection, and corresponding functions from memops in a nice set of structs.

typedef struct alsa_format {
	snd_pcm_format_t format_id;
	size_t sample_size;
	void (*jack_to_soundcard) (char *dst, jack_default_audio_sample_t *src, unsigned long nsamples, unsigned long dst_skip, dither_state_t *state);
	void (*soundcard_to_jack) (jack_default_audio_sample_t *dst, char *src, unsigned long nsamples, unsigned long src_skip);
	const char *name;
} alsa_format_t;

alsa_format_t formats[] = {
	{ SND_PCM_FORMAT_FLOAT_LE, 4, sample_move_dS_floatLE, sample_move_floatLE_sSs, "float" },
	{ SND_PCM_FORMAT_S32, 4, sample_move_d32u24_sS, sample_move_dS_s32u24, "32bit" },
	{ SND_PCM_FORMAT_S24_3LE, 3, sample_move_d24_sS, sample_move_dS_s24, "24bit - real" },
	{ SND_PCM_FORMAT_S24, 4, sample_move_d24_sS, sample_move_dS_s24, "24bit" },
	{ SND_PCM_FORMAT_S16, 2, sample_move_d16_sS, sample_move_dS_s16, "16bit" }
#ifdef __ANDROID__
	,{ SND_PCM_FORMAT_S16_LE, 2, sample_move_d16_sS, sample_move_dS_s16, "16bit little-endian" }
#endif
};
#define NUMFORMATS (sizeof(formats)/sizeof(formats[0]))
int format=0;

// Alsa stuff... i dont want to touch this bullshit in the next years.... please...

static int xrun_recovery(snd_pcm_t *handle, int err) {
//    printf( "xrun !!!.... %d\n", err );
	if (err == -EPIPE) {	/* under-run */
		err = snd_pcm_prepare(handle);
		if (err < 0)
			printf("Can't recover from underrun, prepare failed: %s\n", snd_strerror(err));
		return 0;
	} else if (err == -ESTRPIPE) {
		while ((err = snd_pcm_resume(handle)) == -EAGAIN)
			usleep(100);	/* wait until the suspend flag is released */
		if (err < 0) {
			err = snd_pcm_prepare(handle);
			if (err < 0)
				printf("Can't recover from suspend, prepare failed: %s\n", snd_strerror(err));
		}
		return 0;
	}
	return err;
}

static int set_hwformat( snd_pcm_t *handle, snd_pcm_hw_params_t *params )
{
#ifdef __ANDROID__
	format = 5;
	snd_pcm_hw_params_set_format(handle, params, formats[format].format_id);
	return 0;
#else
	int i;
	int err;

	for( i=0; i<NUMFORMATS; i++ ) {
		/* set the sample format */
		err = snd_pcm_hw_params_set_format(handle, params, formats[i].format_id);
		if (err == 0) {
			format = i;
			return 0;
		}
	}

	return err;
#endif
}

static int set_hwparams(snd_pcm_t *handle, snd_pcm_hw_params_t *params, snd_pcm_access_t access, int rate, int channels, int period, int nperiods ) {
	int err, dir=0;
	unsigned int buffer_time;
	unsigned int period_time;
	unsigned int rrate;
	unsigned int rchannels;

	/* choose all parameters */
	err = snd_pcm_hw_params_any(handle, params);
	if (err < 0) {
		printf("Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
		return err;
	}
	/* set the interleaved read/write format */
	err = snd_pcm_hw_params_set_access(handle, params, access);
	if (err < 0) {
		printf("Access type not available for playback: %s\n", snd_strerror(err));
		return err;
	}

	/* set the sample format */
	err = set_hwformat(handle, params);
	if (err < 0) {
		printf("Sample format not available for playback: %s\n", snd_strerror(err));
		return err;
	}
	/* set the count of channels */
	rchannels = channels;
	err = snd_pcm_hw_params_set_channels_near(handle, params, &rchannels);
	if (err < 0) {
		printf("Channels count (%i) not available for record: %s\n", channels, snd_strerror(err));
		return err;
	}
	if (rchannels != channels) {
		printf("WARNING: chennel count does not match (requested %d got %d)\n", channels, rchannels);
		num_channels = rchannels;
	}
	/* set the stream rate */
	rrate = rate;
	err = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);
	if (err < 0) {
		printf("Rate %iHz not available for playback: %s\n", rate, snd_strerror(err));
		return err;
	}
	if (rrate != rate) {
		printf("WARNING: Rate doesn't match (requested %iHz, get %iHz)\n", rate, rrate);
		sample_rate = rrate;
	}
	/* set the buffer time */

	buffer_time = 1000000*(uint64_t)period*nperiods/rate;
	err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, &dir);
	if (err < 0) {
		printf("Unable to set buffer time %i for playback: %s\n",  1000000*period*nperiods/rate, snd_strerror(err));
		return err;
	}
	err = snd_pcm_hw_params_get_buffer_size( params, &real_buffer_size );
	if (err < 0) {
		printf("Unable to get buffer size back: %s\n", snd_strerror(err));
		return err;
	}
	if( real_buffer_size != nperiods * period ) {
	    printf( "WARNING: buffer size does not match: (requested %d, got %d)\n", nperiods * period, (int) real_buffer_size );
	}
	/* set the period time */
	period_time = 1000000*(uint64_t)period/rate;
	err = snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, &dir);
	if (err < 0) {
		printf("Unable to set period time %i for playback: %s\n", 1000000*period/rate, snd_strerror(err));
		return err;
	}
	err = snd_pcm_hw_params_get_period_size(params, &real_period_size, NULL );
	if (err < 0) {
		printf("Unable to get period size back: %s\n", snd_strerror(err));
		return err;
	}
	if( real_period_size != period ) {
	    printf( "WARNING: period size does not match: (requested %i, got %i)\n", period, (int)real_period_size );
	}
	/* write the parameters to device */
	err = snd_pcm_hw_params(handle, params);
	if (err < 0) {
		printf("Unable to set hw params for playback: %s\n", snd_strerror(err));
		return err;
	}
	return 0;
}

static int set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams, int period) {
	int err;

	/* get the current swparams */
	err = snd_pcm_sw_params_current(handle, swparams);
	if (err < 0) {
		printf("Unable to determine current swparams for capture: %s\n", snd_strerror(err));
		return err;
	}
	/* start the transfer when the buffer is full */
	err = snd_pcm_sw_params_set_start_threshold(handle, swparams, period );
	if (err < 0) {
		printf("Unable to set start threshold mode for capture: %s\n", snd_strerror(err));
		return err;
	}
	err = snd_pcm_sw_params_set_stop_threshold(handle, swparams, -1 );
	if (err < 0) {
		printf("Unable to set start threshold mode for capture: %s\n", snd_strerror(err));
		return err;
	}
	/* allow the transfer when at least period_size samples can be processed */
	err = snd_pcm_sw_params_set_avail_min(handle, swparams, 2*period );
	if (err < 0) {
		printf("Unable to set avail min for capture: %s\n", snd_strerror(err));
		return err;
	}
	/* align all transfers to 1 sample */
	err = snd_pcm_sw_params_set_xfer_align(handle, swparams, 1);
	if (err < 0) {
		printf("Unable to set transfer align for capture: %s\n", snd_strerror(err));
		return err;
	}
	/* write the parameters to the playback device */
	err = snd_pcm_sw_params(handle, swparams);
	if (err < 0) {
		printf("Unable to set sw params for capture: %s\n", snd_strerror(err));
		return err;
	}
	return 0;
}

// ok... i only need this function to communicate with the alsa bloat api...

static snd_pcm_t *open_audiofd( char *device_name, int capture, int rate, int channels, int period, int nperiods ) {
  int err;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *hwparams;
  snd_pcm_sw_params_t *swparams;

  snd_pcm_hw_params_alloca(&hwparams);
  snd_pcm_sw_params_alloca(&swparams);

  if ((err = snd_pcm_open(&(handle), device_name, capture ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK )) < 0) {
      printf("Capture open error: %s\n", snd_strerror(err));
      return NULL;
  }

  if ((err = set_hwparams(handle, hwparams,SND_PCM_ACCESS_RW_INTERLEAVED, rate, channels, period, nperiods )) < 0) {
      printf("Setting of hwparams failed: %s\n", snd_strerror(err));
      return NULL;
  }
  if ((err = set_swparams(handle, swparams, period)) < 0) {
      printf("Setting of swparams failed: %s\n", snd_strerror(err));
      return NULL;
  }

  snd_pcm_start( handle );
  snd_pcm_wait( handle, 200 );

  return handle;
}

double hann( double x )
{
	return 0.5 * (1.0 - cos( 2*M_PI * x ) );
}

/**
 * The freewheel callback.
 */
void freewheel (int starting, void* arg) {
    running_freewheel = starting;
}

/**
 * The process callback for this JACK application.
 * It is called by JACK at the appropriate times.
 */
int process (jack_nframes_t nframes, void *arg) {

    if (running_freewheel) {
	JSList *node = capture_ports;

	while ( node != NULL)
	{
	    jack_port_t *port = (jack_port_t *) node->data;
	    float *buf = jack_port_get_buffer (port, nframes);

	    memset(buf, 0, sizeof(float)*nframes);

	    node = jack_slist_next (node);
	}

	return 0;
    }

    int rlen;
    int err;
    snd_pcm_sframes_t delay = target_delay;
    int put_back_samples=0;
    int i;

    delay = snd_pcm_avail( alsa_handle );

    delay -= round( jack_frames_since_cycle_start( client ) / static_resample_factor );
    // Do it the hard way.
    // this is for compensating xruns etc...

    if( delay > (target_delay+max_diff) ) {

	output_new_delay = (int) delay;

	while ((delay-target_delay) > 0) {
	    snd_pcm_uframes_t to_read = ((delay-target_delay) > 512) ? 512 : (delay-target_delay);
	    snd_pcm_readi( alsa_handle, tmpbuf, to_read );
	    delay -= to_read;
	}

	delay = target_delay;

	// Set the resample_rate... we need to adjust the offset integral, to do this.
	// first look at the PI controller, this code is just a special case, which should never execute once
	// everything is swung in. 
	offset_integral = - (resample_mean - static_resample_factor) * catch_factor * catch_factor2;
	// Also clear the array. we are beginning a new control cycle.
	for( i=0; i<smooth_size; i++ )
		offset_array[i] = 0.0;
    }
    if( delay < (target_delay-max_diff) ) {
	snd_pcm_rewind( alsa_handle, target_delay - delay );
	output_new_delay = (int) delay;
	delay = target_delay;

	// Set the resample_rate... we need to adjust the offset integral, to do this.
	offset_integral = - (resample_mean - static_resample_factor) * catch_factor * catch_factor2;
	// Also clear the array. we are beginning a new control cycle.
	for( i=0; i<smooth_size; i++ )
		offset_array[i] = 0.0;
    }
    /* ok... now we should have target_delay +- max_diff on the alsa side.
     *
     * calculate the number of frames, we want to get.
     */

    double offset = delay - target_delay;

    // Save offset.
    offset_array[(offset_differential_index++)% smooth_size ] = offset;

    // Build the mean of the windowed offset array
    // basically fir lowpassing.
    double smooth_offset = 0.0;
    for( i=0; i<smooth_size; i++ )
	    smooth_offset +=
		    offset_array[ (i + offset_differential_index-1) % smooth_size] * window_array[i];
    smooth_offset /= (double) smooth_size;

    // this is the integral of the smoothed_offset
    offset_integral += smooth_offset;

    // Clamp offset.
    // the smooth offset still contains unwanted noise
    // which would go straigth onto the resample coeff.
    // it only used in the P component and the I component is used for the fine tuning anyways.
    if( fabs( smooth_offset ) < pclamp )
	    smooth_offset = 0.0;

    // ok. now this is the PI controller. 
    // u(t) = K * ( e(t) + 1/T \int e(t') dt' )
    // K = 1/catch_factor and T = catch_factor2
    double current_resample_factor = static_resample_factor - smooth_offset / (double) catch_factor - offset_integral / (double) catch_factor / (double)catch_factor2;

    // now quantize this value around resample_mean, so that the noise which is in the integral component doesnt hurt.
    current_resample_factor = floor( (current_resample_factor - resample_mean) * controlquant + 0.5 ) / controlquant + resample_mean;

    // Output "instrumentatio" gonna change that to real instrumentation in a few.
    output_resampling_factor = (float) current_resample_factor;
    output_diff = (float) smooth_offset;
    output_integral = (float) offset_integral;
    output_offset = (float) offset;

    // Clamp a bit.
    if( current_resample_factor < resample_lower_limit ) current_resample_factor = resample_lower_limit;
    if( current_resample_factor > resample_upper_limit ) current_resample_factor = resample_upper_limit;

    // Now Calculate how many samples we need.
    rlen = ceil( ((double)nframes) / current_resample_factor )+2;
    assert( rlen > 2 );

    // Calculate resample_mean so we can init ourselves to saner values.
    resample_mean = 0.9999 * resample_mean + 0.0001 * current_resample_factor;

    // get the data...
again:
    err = snd_pcm_readi(alsa_handle, outbuf, rlen);
    if( err < 0 ) {
	printf( "err = %d\n", err );
	if (xrun_recovery(alsa_handle, err) < 0) {
	    //printf("Write error: %s\n", snd_strerror(err));
	    //exit(EXIT_FAILURE);
	}
	goto again;
    }
    if( err != rlen ) {
	//printf( "read = %d\n", rlen );
    }

    /*
     * render jack ports to the outbuf...
     */

    int chn = 0;
    JSList *node = capture_ports;
    JSList *src_node = capture_srcs;
    SRC_DATA src;

    while ( node != NULL)
    {
	jack_port_t *port = (jack_port_t *) node->data;
	float *buf = jack_port_get_buffer (port, nframes);

	SRC_STATE *src_state = src_node->data;

	formats[format].soundcard_to_jack( resampbuf, outbuf + format[formats].sample_size * chn, rlen, num_channels*format[formats].sample_size );

	src.data_in = resampbuf;
	src.input_frames = rlen;

	src.data_out = buf;
	src.output_frames = nframes;
	src.end_of_input = 0;

	src.src_ratio = current_resample_factor;

	src_process( src_state, &src );

	put_back_samples = rlen-src.input_frames_used;

	src_node = jack_slist_next (src_node);
	node = jack_slist_next (node);
	chn++;
    }

    // Put back the samples libsamplerate did not consume.
    //printf( "putback = %d\n", put_back_samples );
    snd_pcm_rewind( alsa_handle, put_back_samples );

    return 0;      
}

/**
 * the latency callback.
 * sets up the latencies on the ports.
 */

void
latency_cb (jack_latency_callback_mode_t mode, void *arg)
{
	jack_latency_range_t range;
	JSList *node;

	range.min = range.max = round(target_delay * static_resample_factor);

	if (mode == JackCaptureLatency) {
		for (node = capture_ports; node; node = jack_slist_next (node)) {
			jack_port_t *port = node->data;
			jack_port_set_latency_range (port, mode, &range);
		}
	} else {
		for (node = playback_ports; node; node = jack_slist_next (node)) {
			jack_port_t *port = node->data;
			jack_port_set_latency_range (port, mode, &range);
		}
	}
}


/**
 * Allocate the necessary jack ports...
 */

void alloc_ports( int n_capture, int n_playback ) {

    int port_flags = JackPortIsOutput;
    int chn;
    jack_port_t *port;
    char buf[32];

    capture_ports = NULL;
    for (chn = 0; chn < n_capture; chn++)
    {
	snprintf (buf, sizeof(buf) - 1, "capture_%u", chn+1);

	port = jack_port_register (client, buf,
		JACK_DEFAULT_AUDIO_TYPE,
		port_flags, 0);

	if (!port)
	{
	    printf( "jacknet_client: cannot register port for %s", buf);
	    break;
	}

	capture_srcs = jack_slist_append( capture_srcs, src_new( 4-samplerate_quality, 1, NULL ) );
	capture_ports = jack_slist_append (capture_ports, port);
    }

    port_flags = JackPortIsInput;

    playback_ports = NULL;
    for (chn = 0; chn < n_playback; chn++)
    {
	snprintf (buf, sizeof(buf) - 1, "playback_%u", chn+1);

	port = jack_port_register (client, buf,
		JACK_DEFAULT_AUDIO_TYPE,
		port_flags, 0);

	if (!port)
	{
	    printf( "jacknet_client: cannot register port for %s", buf);
	    break;
	}

	playback_srcs = jack_slist_append( playback_srcs, src_new( 4-samplerate_quality, 1, NULL ) );
	playback_ports = jack_slist_append (playback_ports, port);
    }
}

/**
 * This is the shutdown callback for this JACK application.
 * It is called by JACK if the server ever shuts down or
 * decides to disconnect the client.
 */

void jack_shutdown (void *arg) {

	exit (1);
}

/**
 * be user friendly.
 * be user friendly.
 * be user friendly.
 */

void printUsage() {
fprintf(stderr, "usage: alsa_out [options]\n"
		"\n"
		"  -j <jack name> - client name\n"
		"  -d <alsa_device> \n"
		"  -c <channels> \n"
		"  -p <period_size> \n"
		"  -n <num_period> \n"
		"  -r <sample_rate> \n"
		"  -q <sample_rate quality [0..4]\n"
		"  -m <max_diff> \n"
		"  -t <target_delay> \n"
		"  -i  turns on instrumentation\n"
		"  -v  turns on printouts\n"
		"\n");
}


/**
 * the main function....
 */

void
sigterm_handler( int signal )
{
	quit = 1;
}


int main (int argc, char *argv[]) {
    char jack_name[30] = "alsa_in";
    char alsa_device[30] = "hw:0";

    extern char *optarg;
    extern int optind, optopt;
    int errflg=0;
    int c;

    while ((c = getopt(argc, argv, "ivj:r:c:p:n:d:q:m:t:f:F:C:Q:s:")) != -1) {
	switch(c) {
	    case 'j':
		strcpy(jack_name,optarg);
		break;
	    case 'r':
		sample_rate = atoi(optarg);
		break;
	    case 'c':
		num_channels = atoi(optarg);
		break;
	    case 'p':
		period_size = atoi(optarg);
		break;
	    case 'n':
		num_periods = atoi(optarg);
		break;
	    case 'd':
		strcpy(alsa_device,optarg);
		break;
	    case 't':
		target_delay = atoi(optarg);
		break;
	    case 'q':
		samplerate_quality = atoi(optarg);
		break;
	    case 'm':
		max_diff = atoi(optarg);
		break;
	    case 'f':
		catch_factor = atoi(optarg);
		break;
	    case 'F':
		catch_factor2 = atoi(optarg);
		break;
	    case 'C':
		pclamp = (double) atoi(optarg);
		break;
	    case 'Q':
		controlquant = (double) atoi(optarg);
		break;
	    case 'v':
		verbose = 1;
		break;
	    case 'i':
		instrument = 1;
		break;
	    case 's':
		smooth_size = atoi(optarg);
		break;
	    case ':':
		fprintf(stderr,
			"Option -%c requires an operand\n", optopt);
		errflg++;
		break;
	    case '?':
		fprintf(stderr,
			"Unrecognized option: -%c\n", optopt);
		errflg++;
	}
    }
    if (errflg) {
	printUsage();
	exit(2);
    }

    if( (samplerate_quality < 0) || (samplerate_quality > 4) ) {
	fprintf (stderr, "invalid samplerate quality\n");
	return 1;
    }
    if ((client = jack_client_open (jack_name, 0, NULL)) == 0) {
	fprintf (stderr, "jack server not running?\n");
	return 1;
    }

    /* tell the JACK server to call `process()' whenever
       there is work to be done.
       */

    jack_set_process_callback (client, process, 0);

    /* tell the JACK server to call `freewheel()' whenever
       freewheel mode changes.
       */

    jack_set_freewheel_callback (client, freewheel, 0);

    /* tell the JACK server to call `jack_shutdown()' if
       it ever shuts down, either entirely, or if it
       just decides to stop calling us.
       */

    jack_on_shutdown (client, jack_shutdown, 0);

    if (jack_set_latency_callback)
	    jack_set_latency_callback (client, latency_cb, 0);

    // get jack sample_rate
    
    jack_sample_rate = jack_get_sample_rate( client );

    if( !sample_rate )
	sample_rate = jack_sample_rate;

    // now open the alsa fd...
    alsa_handle = open_audiofd( alsa_device, 1, sample_rate, num_channels, period_size, num_periods);
    if( alsa_handle == 0 )
	exit(20);

    printf( "selected sample format: %s\n", formats[format].name );

    static_resample_factor = (double) jack_sample_rate / (double) sample_rate;
    resample_lower_limit = static_resample_factor * 0.25;
    resample_upper_limit = static_resample_factor * 4.0;
    resample_mean = static_resample_factor;

    offset_array = malloc( sizeof(double) * smooth_size );
    if( offset_array == NULL ) {
	    fprintf( stderr, "no memory for offset_array !!!\n" );
	    exit(20);
    }
    window_array = malloc( sizeof(double) * smooth_size );
    if( window_array == NULL ) {
	    fprintf( stderr, "no memory for window_array !!!\n" );
	    exit(20);
    }
    int i;
    for( i=0; i<smooth_size; i++ ) {
	    offset_array[i] = 0.0;
	    window_array[i] = hann( (double) i / ((double) smooth_size - 1.0) );
    }

    jack_buffer_size = jack_get_buffer_size( client );
    // Setup target delay and max_diff for the normal user, who does not play with them...
    if( !target_delay ) 
	target_delay = (num_periods*period_size / 2) + jack_buffer_size/2;

    if( !max_diff )
	max_diff = num_periods*period_size - target_delay ;	

    if( max_diff > target_delay ) {
	    fprintf( stderr, "target_delay (%d) cant be smaller than max_diff(%d)\n", target_delay, max_diff );
	    exit(20);
    }
    if( (target_delay+max_diff) > (num_periods*period_size) ) {
	    fprintf( stderr, "target_delay+max_diff (%d) cant be bigger than buffersize(%d)\n", target_delay+max_diff, num_periods*period_size );
	    exit(20);
    }
    // alloc input ports, which are blasted out to alsa...
    alloc_ports( num_channels, 0 );

    outbuf = malloc( num_periods * period_size * formats[format].sample_size * num_channels );
    resampbuf = malloc( num_periods * period_size * sizeof( float ) );
    tmpbuf = malloc( 512 * formats[format].sample_size * num_channels );

    if ((outbuf == NULL) || (resampbuf == NULL) || (tmpbuf == NULL))
    {
	    fprintf( stderr, "no memory for buffers.\n" );
	    exit(20);
    }

    memset( tmpbuf, 0, 512 * formats[format].sample_size * num_channels);

    /* tell the JACK server that we are ready to roll */

    if (jack_activate (client)) {
	fprintf (stderr, "cannot activate client");
	return 1;
    }

    signal( SIGTERM, sigterm_handler );
    signal( SIGINT, sigterm_handler );

    if( verbose ) {
	    while(!quit) {
		    usleep(500000);
		    if( output_new_delay ) {
			    printf( "delay = %d\n", output_new_delay );
			    output_new_delay = 0;
		    }
		    printf( "res: %f, \tdiff = %f, \toffset = %f \n", output_resampling_factor, output_diff, output_offset );
	    }
    } else if( instrument ) {
	    printf( "# n\tresamp\tdiff\toffseti\tintegral\n");
	    int n=0;
	    while(!quit) {
		    usleep(1000);
		    printf( "%d\t%f\t%f\t%f\t%f\n", n++, output_resampling_factor, output_diff, output_offset, output_integral );
	    }
    } else {
	    while(!quit)
	    {
		    usleep(500000);
		    if( output_new_delay ) {
			    printf( "delay = %d\n", output_new_delay );
			    output_new_delay = 0;
		    }
	    }
    }

    jack_deactivate( client );
    jack_client_close (client);
    exit (0);
}

