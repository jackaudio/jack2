/*
 * ioaudio_driver.c
 *
 * Copyright 2015 Garmin
 */

#define HW_CONTEXT_T struct jack_card
#define PCM_SUBCHN_CONTEXT_T struct subchn

#include <stdlib.h>

#include <audio_driver.h>

#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <pthread.h>

#include <sys/asound.h>

static const char* PORTNAME_FMT = "capture_%d";
typedef jack_default_audio_sample_t sample_t;

struct subchn
{
    ado_pcm_subchn_t *pcm_subchn;
    ado_pcm_config_t *pcm_config;
    int32_t pcm_offset; /* holds offset of data populated in PCM subchannel buffer */
    uint8_t go; /* indicates if trigger GO has been issue by client for data transfer */
    void *strm; /* pointer back to parent stream structure */
};

typedef struct jack_card
{
    /**
     * PCM device instance
     */
    ado_pcm_t* pcm;

    /**
     * Audio Card Instance
     */
    ado_card_t* card;

    /**
     * Audio Hardware Mutex
     */
    ado_mutex_t hw_lock;

    /**
     * Audio Mixer Instance
     */
    ado_mixer_t* mixer;

    /**
     * PCM Device Capabilities
     */
    ado_pcm_cap_t caps;

    /**
     * PCM Device Callbacks
     */
    ado_pcm_hw_t funcs;

    /**
     * PCM Subchannel used by client
     */
    struct subchn subchn;

    /**
     * Name of JACK server to communicate with
     */
    char* server;

    /**
     * Name of this client
     */
    char* name;

    /**
     * Handle to this client
     */
    jack_client_t* client;

    /**
     * SND_PCM_CHANNEL_CAPTURE or SND_PCM_CHANNEL_PLAYBACK
     */
    int channel_type;

    /**
     * Number of voices / ports
     */
    int voices;

    /**
     * DMA buffer between io-audio and JACK.
     */
    void * dmabuf;

    /**
     * Current DMA buffer fragment index between io-audio and JACK.
     */
    int current_frag;

    /**
     * Number of DMA buffer fragments between io-audio and JACK.
     */
    int num_frags;

    /**
     * Number of audio frames per fragment
     */
    size_t fragsize;

    /**
     * Array of ports between io-audio and JACK
     *
     * size of array is 'voices'
     */
    jack_port_t** ports;

    /**
     * Mutex controlling JACK processing
     */
    pthread_mutex_t process_lock;
} jack_card_t;

static int parse_commandline(
    jack_card_t* jack_card,
    char* theArgs )
{
    int opt = 0;
    char* value;
    char* argdup = ado_strdup( theArgs );
    char* args = argdup;
    char* opts[] = { "server", "name", "type", "voices", NULL };

    while( ( NULL != args ) && ( '\0' != args[0] ) )
        {
        opt = getsubopt( &args,
                         opts,
                         &value );

        if( opt >= 0 )
            {
            switch( opt )
                {
                case 0: /* server: JACK server name */
                    if( NULL == value )
                        {
                        ado_error( "server option given without value; ignoring" );
                        jack_card->server = NULL;
                        }
                    else
                        {
                        jack_card->server = ado_strdup( value );
                        }
                    break;
                case 1: /* name: JACK client name */
                    if( NULL == value )
                        {
                        ado_error( "name option given without value; ignoring" );
                        jack_card->name = ado_strdup( "jack_ioaudio" );
                        }
                    else
                        {
                        jack_card->name = ado_strdup( value );
                        }
                    break;
                case 2: /* type: io-audio channel type */
                    if( NULL != value )
                        {
                        if( value[0] == 'P' || value[0] == 'p' )
                            {
                            jack_card->channel_type = SND_PCM_CHANNEL_PLAYBACK;
                            }
                        else if( value[0] == 'C' || value[0] == 'c' )
                            {
                            jack_card->channel_type = SND_PCM_CHANNEL_CAPTURE;
                            }
                        }
                    else
                        {
                        ado_error(
                                   "type option value not 'c' or 'p'; defaulting to 'p'" );
                        jack_card->channel_type = SND_PCM_CHANNEL_PLAYBACK;
                        }
                    break;
                case 3: /* voices: number of voices in io-audio channel and JACK ports */
                    if( NULL != value )
                        {
                        char* endptr;
                        int val;
                        errno = 0; /* To distinguish success/failure after call */
                        val = strtol( value,
                                      &endptr,
                                      0 );

                        /* Check for various possible errors */

                        if( ( errno == ERANGE
                            && ( val == LONG_MAX || val == LONG_MIN ) )
                            || ( errno != 0 && val == 0 ) )
                            {
                            ado_error( "voices option value out of range" );
                            return errno;
                            }

                        if( endptr == value )
                            {
                            ado_error( "voices option value contains no digits" );
                            return EINVAL;
                            }

                        jack_card->voices = val;
                        }
                    else
                        {
                        ado_error( "voices option given without value; failing" );
                        return EINVAL;
                        }
                    break;
                default:
                    break;
                }
            }
        else
            {
            ado_debug( DB_LVL_DRIVER,
                       "deva-ctrl-jack: found unknown argument %s",
                       value );
            }
        }
    ado_free( argdup );
    return EOK;
}

int32_t cb_capabilities(
    HW_CONTEXT_T * mcasp_card,
    ado_pcm_t *pcm,
    snd_pcm_channel_info_t * info )
{
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: capabilities()" );
    return EOK;
}

int32_t cb_aquire(
    HW_CONTEXT_T *jack_card,
    PCM_SUBCHN_CONTEXT_T **pc,
    ado_pcm_config_t *config,
    ado_pcm_subchn_t *subchn,
    uint32_t *why_failed )
{
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: aquire()" );
    if( jack_card->subchn.pcm_subchn )
        {
        *why_failed = SND_PCM_PARAMS_NO_CHANNEL;
        return ( EAGAIN );
        }

    /*
     * Create shared memory region for DMA transfer
     */
    config->dmabuf.addr = ado_shm_alloc( config->dmabuf.size,
                                         config->dmabuf.name,
                                         ADO_SHM_DMA_SAFE,
                                         &config->dmabuf.phys_addr );
    jack_card->dmabuf = config->dmabuf.addr;
    jack_card->current_frag = 0;
    jack_card->num_frags = config->mode.block.frags_total;

    /*
     * Store parameters for future use
     */
    jack_card->subchn.pcm_config = config;
    jack_card->subchn.pcm_subchn = subchn;

    /*
     * Set output parameters
     */
    *pc = &jack_card->subchn;

    return EOK;
}

int32_t cb_release(
    HW_CONTEXT_T *jack_card,
    PCM_SUBCHN_CONTEXT_T *pc,
    ado_pcm_config_t *config )
{
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: release()" );

    pthread_mutex_lock( &jack_card->process_lock );

    jack_card->subchn.go = 0;

    ado_shm_free( pc->pcm_config->dmabuf.addr,
                  pc->pcm_config->dmabuf.size,
                  pc->pcm_config->dmabuf.name );
    pc->pcm_subchn = NULL;

    pthread_mutex_unlock( &jack_card->process_lock );
    return EOK;
}

int32_t cb_prepare(
    HW_CONTEXT_T *jack_card,
    PCM_SUBCHN_CONTEXT_T *pc,
    ado_pcm_config_t *config )
{
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: prepare()" );

    /* Reset the DMA buffer fragment index. */
    pthread_mutex_lock( &jack_card->process_lock );
    jack_card->current_frag = 0;
    pthread_mutex_unlock( &jack_card->process_lock );
    return EOK;
}

int32_t cb_trigger(
    HW_CONTEXT_T *jack_card,
    PCM_SUBCHN_CONTEXT_T *pc,
    uint32_t cmd )
{
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: trigger( %d )",
               cmd );

    if( ADO_PCM_TRIGGER_STOP == cmd )
        {
        jack_card->subchn.go = 0;
        ado_debug( DB_LVL_DRIVER,
                   "deva-ctrl-jack: ADO_PCM_TRIGGER_STOP" );
        }
    else if( ADO_PCM_TRIGGER_GO == cmd )
        {
        /*
         * Signal readiness to JACK and ADO
         */
        jack_card->subchn.go = 1;
        ado_debug( DB_LVL_DRIVER,
                   "deva-ctrl-jack: ADO_PCM_TRIGGER_GO" );
        }
    else
        {
        jack_card->subchn.go = 0;
        ado_debug( DB_LVL_DRIVER,
                   "deva-ctrl-jack: ADO_PCM_TRIGGER_SYNC_AND_GO" );
        }
    return EOK;
}

uint32_t cb_position(
    HW_CONTEXT_T *jack_card,
    PCM_SUBCHN_CONTEXT_T *pc,
    ado_pcm_config_t *config )
{
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: position()" );
    return EOK;
}

void irq_handler(
    HW_CONTEXT_T *jack_card,
    int32_t event )
{
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: irq_handler()" );
}

/**
 * Prototype for the client supplied function that is called
 * by the engine anytime there is work to be done.
 *
 * @pre nframes == jack_get_buffer_size()
 * @pre nframes == pow(2,x)
 *
 * @param nframes number of frames to process
 * @param arg pointer to a client supplied structure
 *
 * @return zero on success, non-zero on error
 */
int jack_process(
    jack_nframes_t nframes,
    void *arg )
{
    int v;
    jack_card_t* jack_card = (jack_card_t*)arg;

    size_t frame_size = nframes * sizeof(sample_t);
    size_t frag_size = frame_size * jack_card->voices;

    if( jack_card->fragsize != frag_size )
        {
        ado_error(
                   "deva_ctrl-jack: mismatch of fragment size with number of frames requested" );
        }
    /*
     * Try to lock the process mutex. If the lock can't be acquired, assume something is happening with the subchannel
     * and write zeroes to JACK instead.
     */
    else if( jack_card->subchn.go
        && ( 0 == pthread_mutex_trylock( &jack_card->process_lock ) ) )
        {
        /* Get the current frame ( voice ) buffer */
        char * frame_buf = (char * )jack_card->dmabuf;
        frame_buf += jack_card->current_frag * frag_size;

        for( v = 0; v < jack_card->voices; ++v )
            {
            sample_t * jack_buf;
            jack_buf = jack_port_get_buffer( jack_card->ports[v], nframes );
            if( SND_PCM_SFMT_FLOAT_LE
                == jack_card->subchn.pcm_config->format.format )
                {
                float * read_buf = ( float * )frame_buf;
                memcpy( jack_buf, read_buf, frame_size );
                }
            else if( SND_PCM_SFMT_S32_LE
                == jack_card->subchn.pcm_config->format.format )
                {
                int32_t * read_buf = ( int32_t * )frame_buf;
                int s;
                for( s = 0; s < nframes; ++s )
                    {
                    jack_buf[s] = ( (sample_t)read_buf[s] ) * ( (sample_t)1.0 )
                        / ( (sample_t)INT_MAX );

                    }
                }
            frame_buf += frame_size;
            }
        ++jack_card->current_frag;
        if( jack_card->current_frag >= jack_card->num_frags )
            {
            jack_card->current_frag = 0;
            }
        dma_interrupt( jack_card->subchn.pcm_subchn );
        pthread_mutex_unlock( &jack_card->process_lock );
        }
    else
        {
        /*
         * In the case where there is no subchannel or we can't aquire the ringbuffer lock,
         * write zeroes to the JACK buffers rather than let stale data sit in them.
         */
        for( v = 0; v < jack_card->voices; ++v )
            {
            void* jack_buf = jack_port_get_buffer( jack_card->ports[v],
                                                   nframes );
            memset( jack_buf,
                    0,
                    frame_size );
            }
        }

    return 0;
}

/**
 * Prototype for the client supplied function that is called
 * whenever jackd is shutdown. Note that after server shutdown,
 * the client pointer is *not* deallocated by libjack,
 * the application is responsible to properly use jack_client_close()
 * to release client ressources. Warning: jack_client_close() cannot be
 * safely used inside the shutdown callback and has to be called outside of
 * the callback context.
 *
 * @param arg pointer to a client supplied structure
 */
void jack_shutdown(
    void *arg )
{
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: jack_shutdown" );
    jack_card_t* card = (jack_card_t*)arg;

    /*
     * TODO
     * Find a way to trigger io-audio to call ctrl_destroy() to clean up
     */
}

ado_ctrl_dll_init_t ctrl_init;
int ctrl_init(
    HW_CONTEXT_T ** hw_context,
    ado_card_t * card,
    char *args )
{
    int retval;
    int i;
    jack_card_t* jack_card;

    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: ctrl_init" );

    jack_card = ado_calloc( 1,
                            sizeof(jack_card_t) );
    if( NULL == jack_card )
        {
        ado_error( "Unable to allocate memory for jack_card (%s)",
                   strerror( errno ) );
        return -1;
        }

    *hw_context = jack_card;
    jack_card->card = card;

    pthread_mutex_init( &jack_card->process_lock,
                        NULL );

    ado_card_set_shortname( card,
                            "jack_card" );
    ado_card_set_longname( card,
                           "jack_card",
                           0x1000 );

    retval = parse_commandline( jack_card,
                                args );
    if( EOK != retval )
        {
        ado_error(
                   "Received error from parse_commandline(): %d; cannot continue",
                   retval );
        return -1;
        }

    /*
     * Create client per command line arguments
     */
    int client_name_size = jack_client_name_size();
    if( strlen( jack_card->name ) > client_name_size )
        {
        ado_error(
                   "Client name %s is too long (%d chars).\nPlease use a client name of %d characters or less.",
                   strlen( jack_card->name ),
                   client_name_size );
        return -1;
        }
    jack_status_t open_status;
    if( NULL != jack_card->server )
        {
        jack_card->client = jack_client_open( jack_card->name,
                                              JackServerName,
                                              &open_status,
                                              jack_card->server );
        }
    else
        {
        jack_card->client = jack_client_open( jack_card->name,
                                              JackNullOption,
                                              &open_status );
        }
    if( NULL == jack_card->client )
        {
        ado_error( "Error opening JACK client (%x)",
                   open_status );
        ado_free( jack_card );
        return -1;
        }

    /*
     * Set JACK and ADO processing callbacks
     */
    jack_set_process_callback( jack_card->client,
                               jack_process,
                               jack_card );
    jack_on_shutdown( jack_card->client,
                      jack_shutdown,
                      0 );

    /*
     * Allocate jack_port_t array
     */
    int voices = jack_card->voices;
    jack_card->ports = ado_calloc( voices,
                                   sizeof(jack_port_t*) );

    /*
     * Register JACK ports and create their ringbuffers
     */
    int port_name_size = jack_port_name_size();
    char* portname = calloc( jack_port_name_size(),
                             sizeof(char) );
    for( i = 0; i < voices; ++i )
        {
        snprintf( portname,
                  port_name_size,
                  PORTNAME_FMT,
                  ( i + 1 ) );
        jack_card->ports[i] = jack_port_register( jack_card->client,
                                                  portname,
                                                  JACK_DEFAULT_AUDIO_TYPE,
                                                  JackPortIsOutput,
                                                  0 );
        }
    free( portname );

    /*
     * Initialize Capabilities
     */
    jack_card->caps.chn_flags = SND_PCM_CHNINFO_BLOCK
        | SND_PCM_CHNINFO_NONINTERLEAVE;
    jack_card->caps.formats = 0xFFFF;
    jack_card->caps.formats = SND_PCM_FMT_FLOAT_LE | SND_PCM_FMT_S32_LE;

    jack_nframes_t rate = jack_get_sample_rate( jack_card->client );
    uint32_t rateflag = ado_pcm_rate2flag( rate );
    jack_card->caps.rates = rateflag;

    jack_card->caps.min_voices = jack_card->voices;
    jack_card->caps.max_voices = jack_card->voices;

    /*
     * Get size of ringbuffer from JACK server
     */
    jack_card->fragsize = jack_get_buffer_size( jack_card->client )
        * sizeof(sample_t) * jack_card->voices;
    jack_card->caps.min_fragsize = jack_card->fragsize;
    jack_card->caps.max_fragsize = jack_card->fragsize;
    jack_card->caps.max_dma_size = 0;
    jack_card->caps.max_frags = 0;

    jack_card->funcs.capabilities2 = cb_capabilities;
    jack_card->funcs.aquire = cb_aquire;
    jack_card->funcs.release = cb_release;
    jack_card->funcs.prepare = cb_prepare;
    jack_card->funcs.trigger = cb_trigger;
    jack_card->funcs.position = cb_position;

    /*
     * Create Audio PCM Device
     */
    retval = ado_pcm_create( card,
                             "JACK io-audio driver",
                             ( jack_card->channel_type
                                 == SND_PCM_CHANNEL_PLAYBACK ) ?
                                 SND_PCM_INFO_PLAYBACK : SND_PCM_INFO_CAPTURE,
                             "jack-audio",
                             ( jack_card->channel_type
                                 == SND_PCM_CHANNEL_PLAYBACK ) ? 1 : 0,
                             ( jack_card->channel_type
                                 == SND_PCM_CHANNEL_PLAYBACK ) ?
                                 &jack_card->caps : NULL,
                             ( jack_card->channel_type
                                 == SND_PCM_CHANNEL_PLAYBACK ) ?
                                 &jack_card->funcs : NULL,
                             ( jack_card->channel_type
                                 == SND_PCM_CHANNEL_CAPTURE ) ? 1 : 0,
                             ( jack_card->channel_type
                                 == SND_PCM_CHANNEL_CAPTURE ) ?
                                 &jack_card->caps : NULL,
                             ( jack_card->channel_type
                                 == SND_PCM_CHANNEL_CAPTURE ) ?
                                 &jack_card->funcs : NULL,
                             &jack_card->pcm );

    if( -1 == retval )
        {
        ado_error( "deva_ctrl_jack: ERROR: %s",
                   strerror( errno ) );
        ado_free( jack_card->ports );
        ado_free( jack_card );
        return -1;
        }

    /*
     * Activate JACK client
     */
    return jack_activate( jack_card->client );
}

ado_ctrl_dll_destroy_t ctrl_destroy;
int ctrl_destroy(
    HW_CONTEXT_T * context )
{
    jack_card_t* jack_card = (jack_card_t*)context;

    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: ctrl_destroy()" );

    jack_deactivate( jack_card->client );
    jack_client_close( jack_card->client );

    ado_free( jack_card->ports );
    ado_free( jack_card->server );
    ado_free( jack_card->name );

    pthread_mutex_destroy( &jack_card->process_lock );
    ado_free( jack_card );

    return EOK;
}

ado_dll_version_t ctrl_version;
void ctrl_version(
    int *major,
    int *minor,
    char *date )
{
    *major = ADO_MAJOR_VERSION;
    *minor = 1;
    date = __DATE__;
}

int ctrl_devctl(
    uint32_t cmd,
    uint8_t *msg,
    uint16_t *msg_size,
    HW_CONTEXT_T *context )
{

}

