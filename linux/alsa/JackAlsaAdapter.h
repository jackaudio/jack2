/*
Copyright (C) 2008 Grame

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

#ifndef __JackAlsaAdapter__
#define __JackAlsaAdapter__

#include <math.h>
#include <limits.h>
#include <assert.h>
#include <alsa/asoundlib.h>
#include "JackAudioAdapterInterface.h"
#include "JackPlatformThread.h"
#include "JackError.h"
#include "jack.h"
#include "jslist.h"

namespace Jack
{

//inline void* aligned_calloc(size_t nmemb, size_t size) { return (void*)((size_t)(calloc((nmemb * size) + 15, sizeof(char))) + 15 & ~15); }
inline void* aligned_calloc(size_t nmemb, size_t size) { return (void*)calloc(nmemb, size); }

#define max(x,y) (((x)>(y)) ? (x) : (y))
#define min(x,y) (((x)<(y)) ? (x) : (y))

#define check_error(err) if (err) { jack_error("%s:%d, alsa error %d : %s", __FILE__, __LINE__, err, snd_strerror(err)); return err; }
#define check_error_msg(err,msg) if (err) { jack_error("%s:%d, %s : %s(%d)", __FILE__, __LINE__, msg, snd_strerror(err), err); return err; }
#define display_error_msg(err,msg) if (err) { jack_error("%s:%d, %s : %s(%d)", __FILE__, __LINE__, msg, snd_strerror(err), err); }

/**
 * A convenient class to pass parameters to AudioInterface
 */
class AudioParam
{

  public:
            
    const char*     fCardName;					
    unsigned int	fFrequency;
    int             fBuffering; 
    
    unsigned int	fSoftInputs;
    unsigned int	fSoftOutputs;
    
  public:
  
    AudioParam() : 
        fCardName("hw:0"),
        fFrequency(44100),
        fBuffering(512),
        fSoftInputs(2),
        fSoftOutputs(2)
    {}
    
    AudioParam(int input, int output, jack_nframes_t buffer_size, jack_nframes_t sample_rate) : 
        fCardName("hw:0"),
        fFrequency(sample_rate),
        fBuffering(buffer_size),
        fSoftInputs(input),
        fSoftOutputs(output)
    {}
    
    AudioParam&	cardName(const char* n)	{ fCardName = n; 	return *this; }
    AudioParam&	frequency(int f)	{ fFrequency = f; 	return *this; }
    AudioParam&	buffering(int fpb)	{ fBuffering = fpb; 	return *this; }
    AudioParam&	inputs(int n)		{ fSoftInputs = n; 	return *this; }
    AudioParam&	outputs(int n)		{ fSoftOutputs = n; 	return *this; }
};

/**
 * An ALSA audio interface
 */
class AudioInterface : public AudioParam
{

 public:
 
    snd_pcm_t*	fOutputDevice;		
    snd_pcm_t*	fInputDevice;			
    snd_pcm_hw_params_t* fInputParams;
    snd_pcm_hw_params_t* fOutputParams;
    
    snd_pcm_format_t fSampleFormat;
    snd_pcm_access_t fSampleAccess;
    
    unsigned int fCardInputs;
    unsigned int fCardOutputs;
    
    unsigned int fChanInputs;
    unsigned int fChanOutputs;
    
    // interleaved mode audiocard buffers
    void* fInputCardBuffer;
    void* fOutputCardBuffer;
    
    // non interleaved mode audiocard buffers
    void* fInputCardChannels[256];
    void* fOutputCardChannels[256];
    
    // non interleaved mod, floating point software buffers
    float* fInputSoftChannels[256];
    float* fOutputSoftChannels[256];

 public:
 
    const char*	cardName()	{ return fCardName; }
    int		frequency()		{ return fFrequency; }
    int		buffering()		{ return fBuffering; }
    
    float**		inputSoftChannels()	{ return fInputSoftChannels; }
    float**		outputSoftChannels()	{ return fOutputSoftChannels; }
    
    AudioInterface(const AudioParam& ap = AudioParam()) : AudioParam(ap)
    {
        fInputDevice 	= 0;
        fOutputDevice 	= 0;
        fInputParams	= 0;
        fOutputParams	= 0;
    }

    AudioInterface(int input, int output, jack_nframes_t buffer_size, jack_nframes_t sample_rate) : 
        AudioParam(input, output, buffer_size, sample_rate)
    {
        fInputCardBuffer = 0;
        fOutputCardBuffer = 0;

        for (int i = 0; i < 256; i++) {
            fInputCardChannels[i] = 0;
            fOutputCardChannels[i] = 0;
            fInputSoftChannels[i] = 0;
            fOutputSoftChannels[i] = 0;
        }
    }
    
    /**
     * Open the audio interface
     */
    int open()
    {
        int err;
        // allocation d'un stream d'entree et d'un stream de sortie
        err = snd_pcm_open(&fInputDevice,  fCardName, SND_PCM_STREAM_CAPTURE, 0); 	check_error(err)
        err = snd_pcm_open(&fOutputDevice, fCardName, SND_PCM_STREAM_PLAYBACK, 0); 	check_error(err)

        // recherche des parametres d'entree
        err = snd_pcm_hw_params_malloc(&fInputParams); 	check_error(err);
        setAudioParams(fInputDevice, fInputParams);
        snd_pcm_hw_params_get_channels(fInputParams, &fCardInputs);

        // recherche des parametres de sortie
        err = snd_pcm_hw_params_malloc(&fOutputParams); check_error(err)
        setAudioParams(fOutputDevice, fOutputParams);
        snd_pcm_hw_params_get_channels(fOutputParams, &fCardOutputs);

        jack_info("inputs : %ud, outputs : %ud", fCardInputs, fCardOutputs);

        // enregistrement des parametres d'entree-sortie
        err = snd_pcm_hw_params(fInputDevice,  fInputParams);	 	check_error (err);
        err = snd_pcm_hw_params(fOutputDevice, fOutputParams);	check_error (err);

        // allocation of alsa buffers
        if (fSampleAccess == SND_PCM_ACCESS_RW_INTERLEAVED) {
            fInputCardBuffer = aligned_calloc(interleavedBufferSize(fInputParams), 1);
            fOutputCardBuffer = aligned_calloc(interleavedBufferSize(fOutputParams), 1);
        } else {
            for (unsigned int i = 0; i < fCardInputs; i++) {
                fInputCardChannels[i] = aligned_calloc(noninterleavedBufferSize(fInputParams), 1);
            }
            for (unsigned int i = 0; i < fCardOutputs; i++) {
                fOutputCardChannels[i] = aligned_calloc(noninterleavedBufferSize(fOutputParams), 1);
            }
        }
        
        // allocation of floating point buffers needed by the dsp code
        fChanInputs = max(fSoftInputs, fCardInputs);		assert (fChanInputs < 256);
        fChanOutputs = max(fSoftOutputs, fCardOutputs);		assert (fChanOutputs < 256);

        for (unsigned int i = 0; i < fChanInputs; i++) {
            fInputSoftChannels[i] = (float*) aligned_calloc (fBuffering, sizeof(float));
            for (int j = 0; j < fBuffering; j++) {
                fInputSoftChannels[i][j] = 0.0;
            }
        }

        for (unsigned int i = 0; i < fChanOutputs; i++) {
            fOutputSoftChannels[i] = (float*) aligned_calloc (fBuffering, sizeof(float));
            for (int j = 0; j < fBuffering; j++) {
                fOutputSoftChannels[i][j] = 0.0;
            }
        }
        return 0;
    }
    
    int close()
    {
        snd_pcm_hw_params_free(fInputParams);
        snd_pcm_hw_params_free(fOutputParams);
        snd_pcm_close(fInputDevice);
        snd_pcm_close(fOutputDevice);

        for (unsigned int i = 0; i < fChanInputs; i++) {
            if (fInputSoftChannels[i])
                free(fInputSoftChannels[i]);
        }

        for (unsigned int i = 0; i < fChanOutputs; i++) {
            if (fOutputSoftChannels[i])
                free(fOutputSoftChannels[i]);
        }

        for (unsigned int i = 0; i < fCardInputs; i++) {
            if (fInputCardChannels[i])
                free(fInputCardChannels[i]);
        }

        for (unsigned int i = 0; i < fCardOutputs; i++) {
            if (fOutputCardChannels[i])
                free(fOutputCardChannels[i]);
        }

        if (fInputCardBuffer)
            free(fInputCardBuffer);
        if (fOutputCardBuffer)
            free(fOutputCardBuffer);
           
        return 0;
    }
    
    int setAudioParams(snd_pcm_t* stream, snd_pcm_hw_params_t* params)
    {	
        int	err;

        // set params record with initial values
        err = snd_pcm_hw_params_any	( stream, params ); 	
        check_error_msg(err, "unable to init parameters")

        // set alsa access mode (and fSampleAccess field) either to non interleaved or interleaved
                
        err = snd_pcm_hw_params_set_access (stream, params, SND_PCM_ACCESS_RW_NONINTERLEAVED );
        if (err) {
            err = snd_pcm_hw_params_set_access (stream, params, SND_PCM_ACCESS_RW_INTERLEAVED );
            check_error_msg(err, "unable to set access mode neither to non-interleaved or to interleaved");
        }
        snd_pcm_hw_params_get_access(params, &fSampleAccess);  

        // search for 32-bits or 16-bits format
        err = snd_pcm_hw_params_set_format (stream, params, SND_PCM_FORMAT_S32);
        if (err) {
            err = snd_pcm_hw_params_set_format (stream, params, SND_PCM_FORMAT_S16);
            check_error_msg(err, "unable to set format to either 32-bits or 16-bits");
        }
        snd_pcm_hw_params_get_format(params, &fSampleFormat);
        // set sample frequency
        snd_pcm_hw_params_set_rate_near (stream, params, &fFrequency, 0); 

        // set period and period size (buffering)
        err = snd_pcm_hw_params_set_period_size	(stream, params, fBuffering, 0); 	
        check_error_msg(err, "period size not available");
        
        err = snd_pcm_hw_params_set_periods (stream, params, 2, 0); 			
        check_error_msg(err, "number of periods not available");
        return 0;
    }

    ssize_t interleavedBufferSize(snd_pcm_hw_params_t* params)
    {
        _snd_pcm_format 	format;  	snd_pcm_hw_params_get_format(params, &format);
        snd_pcm_uframes_t 	psize;		snd_pcm_hw_params_get_period_size(params, &psize, NULL);
        unsigned int 		channels; 	snd_pcm_hw_params_get_channels(params, &channels);
        ssize_t bsize = snd_pcm_format_size(format, psize * channels);
        return bsize;
    }

    ssize_t noninterleavedBufferSize(snd_pcm_hw_params_t* params)
    {
        _snd_pcm_format 	format;  	snd_pcm_hw_params_get_format(params, &format);
        snd_pcm_uframes_t 	psize;		snd_pcm_hw_params_get_period_size(params, &psize, NULL);
        ssize_t bsize = snd_pcm_format_size(format, psize);
        return bsize;
    }

    /**
     * Read audio samples from the audio card. Convert samples to floats and take 
     * care of interleaved buffers
     */
    int read()
    {
        if (fSampleAccess == SND_PCM_ACCESS_RW_INTERLEAVED) {
            
            int count = snd_pcm_readi(fInputDevice, fInputCardBuffer, fBuffering); 	
            if (count<0) { 
                display_error_msg(count, "reading samples");
                int err = snd_pcm_prepare(fInputDevice);	
                check_error_msg(err, "preparing input stream");
            }
            
            if (fSampleFormat == SND_PCM_FORMAT_S16) {

                short* 	buffer16b = (short*) fInputCardBuffer;
                for (int s = 0; s < fBuffering; s++) {
                    for (unsigned int c = 0; c < fCardInputs; c++) {
                        fInputSoftChannels[c][s] = float(buffer16b[c + s*fCardInputs])*(1.0/float(SHRT_MAX));
                    }
                }

            } else { // SND_PCM_FORMAT_S32

                long* 	buffer32b = (long*) fInputCardBuffer;
                for (int s = 0; s < fBuffering; s++) {
                    for (unsigned int c = 0; c < fCardInputs; c++) {
                        fInputSoftChannels[c][s] = float(buffer32b[c + s*fCardInputs])*(1.0/float(LONG_MAX));
                    }
                }
            }
            
        } else if (fSampleAccess == SND_PCM_ACCESS_RW_NONINTERLEAVED) {
            
            int count = snd_pcm_readn(fInputDevice, fInputCardChannels, fBuffering); 	
            if (count < 0) { 
                display_error_msg(count, "reading samples");
                int err = snd_pcm_prepare(fInputDevice);	
                check_error_msg(err, "preparing input stream");
            }
            
            if (fSampleFormat == SND_PCM_FORMAT_S16) {

                for (unsigned int c = 0; c < fCardInputs; c++) {
                    short* 	chan16b = (short*) fInputCardChannels[c];
                    for (int s = 0; s < fBuffering; s++) {
                        fInputSoftChannels[c][s] = float(chan16b[s])*(1.0/float(SHRT_MAX));
                    }
                }

            } else { // SND_PCM_FORMAT_S32

                for (unsigned int c = 0; c < fCardInputs; c++) {
                    long* 	chan32b = (long*) fInputCardChannels[c];
                    for (int s = 0; s < fBuffering; s++) {
                        fInputSoftChannels[c][s] = float(chan32b[s])*(1.0/float(LONG_MAX));
                    }
                }
            }
            
        } else {
            check_error_msg(-10000, "unknow access mode");
        }

        return 0;
    }

    /**
     * write the output soft channels to the audio card. Convert sample 
     * format and interleaves buffers when needed
     */
    int write()
    {
        recovery:
                
        if (fSampleAccess == SND_PCM_ACCESS_RW_INTERLEAVED) {
            
            if (fSampleFormat == SND_PCM_FORMAT_S16) {

                short* buffer16b = (short*) fOutputCardBuffer;
                for (int f = 0; f < fBuffering; f++) {
                    for (unsigned int c = 0; c < fCardOutputs; c++) {
                        float x = fOutputSoftChannels[c][f];
                        buffer16b[c + f * fCardOutputs] = short(max(min(x, 1.0), -1.0) * float(SHRT_MAX));
                    }
                }

            } else { // SND_PCM_FORMAT_S32

                long* buffer32b = (long*) fOutputCardBuffer;
                for (int f = 0; f < fBuffering; f++) {
                    for (unsigned int c = 0; c < fCardOutputs; c++) {
                        float x = fOutputSoftChannels[c][f];
                        buffer32b[c + f * fCardOutputs] = long( max(min(x, 1.0), -1.0) * float(LONG_MAX));
                    }
                }
            }

            int count = snd_pcm_writei(fOutputDevice, fOutputCardBuffer, fBuffering); 	
            if (count < 0) { 
                display_error_msg(count, "w3"); 
                int err = snd_pcm_prepare(fOutputDevice);	
                check_error_msg(err, "preparing output stream");
                goto recovery;
            }
            
            
        } else if (fSampleAccess == SND_PCM_ACCESS_RW_NONINTERLEAVED) {
            
            if (fSampleFormat == SND_PCM_FORMAT_S16) {

                for (unsigned int c = 0; c < fCardOutputs; c++) {
                    short* chan16b = (short*) fOutputCardChannels[c];
                    for (int f = 0; f < fBuffering; f++) {
                        float x = fOutputSoftChannels[c][f];
                        chan16b[f] = short(max(min(x,1.0), -1.0) * float(SHRT_MAX)) ;
                    }
                }

            } else { // SND_PCM_FORMAT_S32

                for (unsigned int c = 0; c < fCardOutputs; c++) {
                    long* chan32b = (long*) fOutputCardChannels[c];
                    for (int f = 0; f < fBuffering; f++) {
                        float x = fOutputSoftChannels[c][f];
                        chan32b[f] = long(max(min(x,1.0),-1.0) * float(LONG_MAX)) ;
                    }
                }
            }

            int count = snd_pcm_writen(fOutputDevice, fOutputCardChannels, fBuffering); 	
            if (count<0) { 
                display_error_msg(count, "w3"); 
                int err = snd_pcm_prepare(fOutputDevice);	
                check_error_msg(err, "preparing output stream");
                goto recovery;
            }
            
        } else {
            check_error_msg(-10000, "unknow access mode");
        }

        return 0;
    }
  
    /**
     *  print short information on the audio device
     */
    int shortinfo()
    {
        int	err;
        snd_ctl_card_info_t* card_info;
        snd_ctl_t* ctl_handle;
        err = snd_ctl_open(&ctl_handle, fCardName, 0);	check_error(err);
        snd_ctl_card_info_alloca(&card_info);
        err = snd_ctl_card_info(ctl_handle, card_info);	check_error(err);
        jack_info("%s|%d|%d|%d|%d|%s", 
                snd_ctl_card_info_get_driver(card_info),
                fCardInputs, fCardOutputs,
                fFrequency, fBuffering,
                snd_pcm_format_name((_snd_pcm_format)fSampleFormat));
    }
                    
    /**
     *  print more detailled information on the audio device
     */
    int longinfo()
    {
        int	err;
        snd_ctl_card_info_t* card_info;
        snd_ctl_t* ctl_handle;

        jack_info("Audio Interface Description :");
        jack_info("Sampling Frequency : %d, Sample Format : %s, buffering : %d", 
                fFrequency, snd_pcm_format_name((_snd_pcm_format)fSampleFormat), fBuffering);
        jack_info("Software inputs : %2d, Software outputs : %2d", fSoftInputs, fSoftOutputs);
        jack_info("Hardware inputs : %2d, Hardware outputs : %2d", fCardInputs, fCardOutputs);
        jack_info("Channel inputs  : %2d, Channel outputs  : %2d", fChanInputs, fChanOutputs);
        
        // affichage des infos de la carte
        err = snd_ctl_open (&ctl_handle, fCardName, 0);		check_error(err);
        snd_ctl_card_info_alloca (&card_info);
        err = snd_ctl_card_info(ctl_handle, card_info);		check_error(err);
        printCardInfo(card_info);

        // affichage des infos liees aux streams d'entree-sortie
        if (fSoftInputs > 0)	printHWParams(fInputParams);
        if (fSoftOutputs > 0)	printHWParams(fOutputParams);
        return 0;
    }
    
    void printCardInfo(snd_ctl_card_info_t*	ci)
    {
        jack_info("Card info (address : %p)", ci);
        jack_info("\tID         = %s", snd_ctl_card_info_get_id(ci));
        jack_info("\tDriver     = %s", snd_ctl_card_info_get_driver(ci));
        jack_info("\tName       = %s", snd_ctl_card_info_get_name(ci));
        jack_info("\tLongName   = %s", snd_ctl_card_info_get_longname(ci));
        jack_info("\tMixerName  = %s", snd_ctl_card_info_get_mixername(ci));
        jack_info("\tComponents = %s", snd_ctl_card_info_get_components(ci));
        jack_info("--------------");
    }

    void printHWParams(snd_pcm_hw_params_t* params)
    {
        jack_info("HW Params info (address : %p)\n", params);
#if 0
        jack_info("\tChannels    = %d", snd_pcm_hw_params_get_channels(params));
        jack_info("\tFormat      = %s", snd_pcm_format_name((_snd_pcm_format)snd_pcm_hw_params_get_format(params)));
        jack_info("\tAccess      = %s", snd_pcm_access_name((_snd_pcm_access)snd_pcm_hw_params_get_access(params)));
        jack_info("\tRate        = %d", snd_pcm_hw_params_get_rate(params, NULL));
        jack_info("\tPeriods     = %d", snd_pcm_hw_params_get_periods(params, NULL));
        jack_info("\tPeriod size = %d", (int)snd_pcm_hw_params_get_period_size(params, NULL));
        jack_info("\tPeriod time = %d", snd_pcm_hw_params_get_period_time(params, NULL));
        jack_info("\tBuffer size = %d", (int)snd_pcm_hw_params_get_buffer_size(params));
        jack_info("\tBuffer time = %d", snd_pcm_hw_params_get_buffer_time(params, NULL));
#endif
        jack_info("--------------");
    }
};

/*!
\brief Audio adapter suing ALSA API.
*/

class JackAlsaAdapter : public JackAudioAdapterInterface, public JackRunnableInterface
{

    private:
        
        JackThread fThread;
        AudioInterface fAudioInterface;
   
    public:
    
        JackAlsaAdapter(jack_nframes_t buffer_size, jack_nframes_t sample_rate, const JSList* params);
        ~JackAlsaAdapter()
        {}
        
        virtual int Open();
        virtual int Close();
         
        virtual int SetBufferSize(jack_nframes_t buffer_size);
        
        virtual bool Init();
        virtual bool Execute();
         
};

}

#ifdef __cplusplus
extern "C"
{
#endif

#include "JackExports.h"
#include "driver_interface.h"

EXPORT jack_driver_desc_t* jack_get_descriptor();

#ifdef __cplusplus
}
#endif

#endif
