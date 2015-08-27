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

static const char* PORTNAME_FMT = "playback_%d";
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
     * Ringbuffer between io-audio and JACK
     */
    jack_ringbuffer_t ringbuffer;

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
            ado_debug( DB_LVL_DRIVER,
                       "deva-ctrl-jack: Parsed option %d (%s)",
                       opt,
                       opts[opt] );
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
                    ado_debug( DB_LVL_DRIVER,
                               "deva-ctrl-jack: argument 'server' parsed as %s",
                               jack_card->server );
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
                    ado_debug( DB_LVL_DRIVER,
                               "deva-ctrl-jack: argument 'name' parsed as %s",
                               jack_card->name );

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
                    ado_debug( DB_LVL_DRIVER,
                               "deva-ctrl-jack: argument 'type' parsed as %d",
                               jack_card->channel_type );
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
                    ado_debug( DB_LVL_DRIVER,
                               "deva-ctrl-jack: argument 'voices' with value '%s' parsed as %d",
                               value ? value : "NULL",
                               jack_card->voices );
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

    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: finished parsing arguments" );

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
    ado_debug( DB_LVL_DRIVER,
               "config:" );
    ado_debug( DB_LVL_DRIVER,
               "  format:" );
    ado_debug( DB_LVL_DRIVER,
               "    interleave:  %d",
               config->format.interleave );
    ado_debug( DB_LVL_DRIVER,
               "    format:      {id: %d, name: '%s'}",
               config->format.format,
               snd_pcm_get_format_name( config->format.format ) );
    ado_debug( DB_LVL_DRIVER,
               "    rate:        %d",
               config->format.rate );
    ado_debug( DB_LVL_DRIVER,
               "    voices:      %d",
               config->format.voices );
    ado_debug( DB_LVL_DRIVER,
               "  mode.block:" );
    ado_debug( DB_LVL_DRIVER,
               "    frag_size:   %d",
               config->mode.block.frag_size );
    ado_debug( DB_LVL_DRIVER,
               "    frags_min:   %d",
               config->mode.block.frags_min );
    ado_debug( DB_LVL_DRIVER,
               "    frags_max:   %d",
               config->mode.block.frags_max );
    ado_debug( DB_LVL_DRIVER,
               "    frags_total: %d",
               config->mode.block.frags_total );
    ado_debug( DB_LVL_DRIVER,
               "  dmabuf:" );
    ado_debug( DB_LVL_DRIVER,
               "    addr:        %x",
               config->dmabuf.addr );
    ado_debug( DB_LVL_DRIVER,
               "    phys_addr:   %x",
               config->dmabuf.phys_addr );
    ado_debug( DB_LVL_DRIVER,
               "    size:        %d",
               config->dmabuf.size );
    ado_debug( DB_LVL_DRIVER,
               "    name:        '%s'",
               config->dmabuf.name );
    ado_debug( DB_LVL_DRIVER,
               "  mixer_device:  %d",
               config->mixer_device );
    ado_debug( DB_LVL_DRIVER,
               "  mixer_eid:" );
    ado_debug( DB_LVL_DRIVER,
               "    type:        %d",
               config->mixer_eid.type );
    ado_debug( DB_LVL_DRIVER,
               "    name:        '%s'",
               config->mixer_eid.name );
    ado_debug( DB_LVL_DRIVER,
               "    index:       %d",
               config->mixer_eid.index );
    ado_debug( DB_LVL_DRIVER,
               "    weight:      %d",
               config->mixer_eid.weight );
    ado_debug( DB_LVL_DRIVER,
               "  mixer_gid:" );
    ado_debug( DB_LVL_DRIVER,
               "    type:        %d",
               config->mixer_gid.type );
    ado_debug( DB_LVL_DRIVER,
               "    name:        '%s'",
               config->mixer_gid.name );
    ado_debug( DB_LVL_DRIVER,
               "    index:       %d",
               config->mixer_gid.index );
    ado_debug( DB_LVL_DRIVER,
               "    weight:      %d",
               config->mixer_gid.weight );

    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: creating ringbuffer of %u bytes",
               config->dmabuf.size );

    /*
     * Create shared memory region for ringbuffer
     */
    config->dmabuf.addr = ado_shm_alloc( config->dmabuf.size,
                                         config->dmabuf.name,
                                         ADO_SHM_DMA_SAFE,
                                         &config->dmabuf.phys_addr );
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: ado_shm_alloc() has finished. " );
    ado_debug( DB_LVL_DRIVER,
               "  dmabuf:" );
    ado_debug( DB_LVL_DRIVER,
               "    addr:        %x",
               config->dmabuf.addr );
    ado_debug( DB_LVL_DRIVER,
               "    phys_addr:   %x",
               config->dmabuf.phys_addr );
    ado_debug( DB_LVL_DRIVER,
               "    size:        %d",
               config->dmabuf.size );
    ado_debug( DB_LVL_DRIVER,
               "    name:        '%s'",
               config->dmabuf.name );

    /*
     * Set up JACK ringbuffer structure to use SHM region instead of own
     * buffer.
     */
    jack_card->ringbuffer.buf = (char*)config->dmabuf.addr;
    jack_card->ringbuffer.size = config->dmabuf.size;
    jack_card->ringbuffer.size_mask = config->dmabuf.size - 1;
    jack_ringbuffer_reset( &jack_card->ringbuffer );

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
    pthread_mutex_lock( &jack_card->process_lock );

    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: release()" );
    jack_card->subchn.go = 0;

    jack_ringbuffer_reset( &jack_card->ringbuffer );

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
    jack_ringbuffer_reset( &jack_card->ringbuffer );
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

    size_t total_size = nframes * sizeof(sample_t);
    size_t size_completed;

    if( ( jack_card->fragsize != total_size ) )
        {
        ado_error(
                   "deva_ctrl-jack: mismatch of fragment size with number of frames requested" );
        }

    /*
     * Try to lock the process mutex. If the lock can't be acquired, assume something is happening with the subchannel
     * and write zeroes to JACK instead.
     */
    if( jack_card->subchn.go
        && ( 0 == pthread_mutex_trylock( &jack_card->process_lock ) ) )
        {
        ado_debug( DB_LVL_PCM,
                   "deva-ctrl-jack: jack_process( %d )",
                   nframes );
        ado_debug( DB_LVL_PCM,
                   "  total_size: %d",
                   total_size );

        int voices = jack_card->subchn.pcm_config->format.voices;
        ado_debug( DB_LVL_PCM,
                   "  voices: %d",
                   voices );

        size_t size_per_voice = total_size / voices;
        ado_debug( DB_LVL_PCM,
                   "  size_per_voice: %d",
                   size_per_voice );

        void* jack_buf[voices];
        for( v = 0; v < voices; ++v )
            {
            jack_buf[v] = jack_port_get_buffer( jack_card->ports[v],
                                                nframes );
            }

        for( size_completed = 0; size_completed < total_size; size_completed +=
            size_per_voice )
            {
            ado_debug( DB_LVL_PCM,
                       "  size_completed: %d",
                       size_completed );
            for( v = 0; v < voices; ++v )
                {
                ado_debug( DB_LVL_PCM,
                           "  voice: %d",
                           v );
                /*
                 * Advance ringbuffer write pointer on the assumption that io-audio has filled in nframes of data
                 */
                jack_ringbuffer_write_advance( &jack_card->ringbuffer,
                                               size_per_voice );

                jack_ringbuffer_data_t read_buf[2];
                jack_ringbuffer_get_read_vector( &jack_card->ringbuffer,
                                                 read_buf );

                if( SND_PCM_SFMT_FLOAT_LE
                    == jack_card->subchn.pcm_config->format.format )
                    {
                    ado_debug( DB_LVL_PCM,
                               "  ringbuffer:" );
                    ado_debug( DB_LVL_PCM,
                               "    - { buf: %x, len: %d }",
                               read_buf[0].buf,
                               read_buf[0].len );
                    ado_debug( DB_LVL_PCM,
                               "    - { buf: %x, len: %d }",
                               read_buf[1].buf,
                               read_buf[1].len );
                    jack_ringbuffer_read( &jack_card->ringbuffer,
                                          jack_buf[v],
                                          size_per_voice );
                    }
                else if( SND_PCM_SFMT_S32_LE
                    == jack_card->subchn.pcm_config->format.format )
                    {
                    int s;
                    size_t remaining = size_per_voice / sizeof(sample_t);
                    read_buf[0].len /= sizeof(sample_t);
                    read_buf[1].len /= sizeof(sample_t);
                    ado_debug( DB_LVL_PCM,
                               "  samples_remaining: %d",
                               remaining );
                    ado_debug( DB_LVL_PCM,
                               "  ringbuffer:" );
                    ado_debug( DB_LVL_PCM,
                               "    - { buf: %x, len: %d }",
                               read_buf[0].buf,
                               read_buf[0].len );
                    ado_debug( DB_LVL_PCM,
                               "    - { buf: %x, len: %d }",
                               read_buf[1].buf,
                               read_buf[1].len );

                    int32_t* src = (sample_t*)read_buf[0].buf;
                    sample_t* dest = (sample_t*)jack_buf[v];
                    size_t amt = min( read_buf[0].len,
                                      remaining );
                    for( s = 0; s < amt; s += sizeof(sample_t) )
                        {
                        dest[s] = ( (sample_t)src[s] ) * ( (sample_t)1.0 )
                            / ( (sample_t)INT_MAX );
//						ado_debug(DB_LVL_PCM, "  %x : %d : %f", &src[s], src[s], dest[s]);
                        }
                    remaining -= amt;
                    ado_debug( DB_LVL_PCM,
                               "  samples_remaining: %d",
                               remaining );
                    ado_debug( DB_LVL_PCM,
                               "  ringbuffer:" );
                    ado_debug( DB_LVL_PCM,
                               "    - { buf: %x, len: %d }",
                               read_buf[0].buf,
                               read_buf[0].len );
                    ado_debug( DB_LVL_PCM,
                               "    - { buf: %x, len: %d }",
                               read_buf[1].buf,
                               read_buf[1].len );
                    if( remaining > 0 )
                        {
                        src = (int32_t*)read_buf[1].buf;
                        dest += amt;
                        amt = min( read_buf[1].len,
                                   remaining );
                        for( s = 0; s < amt; s += sizeof(sample_t) )
                            {
                            dest[s] = ( (sample_t)src[s] ) * ( (sample_t)1.0 )
                                / ( (sample_t)INT_MAX );
                            }
                        }
                    jack_ringbuffer_read_advance( &jack_card->ringbuffer,
                                                  size_per_voice );
                    }

                jack_buf[v] += size_per_voice;

                }
            dma_interrupt( jack_card->subchn.pcm_subchn );
            }
        ado_debug( DB_LVL_PCM,
                   "  size_completed: %d",
                   size_completed );
        pthread_mutex_unlock( &jack_card->process_lock );
        }
    else
        {
        for( v = 0; v < jack_card->voices; ++v )
            {
            void* jack_buf = jack_port_get_buffer( jack_card->ports[v],
                                                   nframes );
            memset( jack_buf,
                    0,
                    total_size );
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

    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: allocating card" );
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

    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: setting card name" );
    ado_card_set_shortname( card,
                            "jack_card" );
    ado_card_set_longname( card,
                           "jack_card",
                           0x1000 );

    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: parsing arguments '%s'",
               args );
    retval = parse_commandline( jack_card,
                                args );
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: returned from parsing arguments" );
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
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: Opening jack client with name '%s' on server '%s'",
               jack_card->name,
               jack_card->server ? jack_card->server : "DEFAULT" );
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
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: registering callbacks" );
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
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: allocating array of %d ports",
               voices );
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
        ado_debug( DB_LVL_DRIVER,
                   "deva-ctrl-jack: registering port %d",
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
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: initializing driver capabilities" );
    jack_card->caps.chn_flags = SND_PCM_CHNINFO_BLOCK
        | SND_PCM_CHNINFO_NONINTERLEAVE;
    jack_card->caps.formats = 0xFFFF;
    jack_card->caps.formats = SND_PCM_FMT_FLOAT_LE | SND_PCM_FMT_S32_LE;
//	jack_card->caps.formats = SND_PCM_FMT_FLOAT_LE;

    jack_nframes_t rate = jack_get_sample_rate( jack_card->client );
    uint32_t rateflag = ado_pcm_rate2flag( rate );
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: sample rate of JACK: %u; flag: 0x%x",
               rate,
               rateflag );
    jack_card->caps.rates = rateflag;

    jack_card->caps.min_voices = 1;
    jack_card->caps.max_voices = jack_card->voices;

    /*
     * Get size of ringbuffer from JACK server
     */
    jack_card->fragsize = jack_get_buffer_size( jack_card->client )
        * sizeof(sample_t);
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: fragment is %u bytes",
               jack_card->fragsize );
    jack_card->caps.min_fragsize = jack_card->fragsize;
    jack_card->caps.max_fragsize = jack_card->fragsize;
    jack_card->caps.max_dma_size = 0;
    jack_card->caps.max_frags = 0;

    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: initializing driver callbacks" );
    jack_card->funcs.capabilities2 = cb_capabilities;
    jack_card->funcs.aquire = cb_aquire;
    jack_card->funcs.release = cb_release;
    jack_card->funcs.prepare = cb_prepare;
    jack_card->funcs.trigger = cb_trigger;
    jack_card->funcs.position = cb_position;

    /*
     * Create Audio PCM Device
     */
    ado_debug( DB_LVL_DRIVER,
               "deva-ctrl-jack: creating PCM device" );
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

