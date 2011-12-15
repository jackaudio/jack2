/*
Copyright (C) 2008 Grame & RTL 2008

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

#include "JackOSSAdapter.h"
#include "JackServerGlobals.h"
#include "JackEngineControl.h"
#include "memops.h"

#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <iostream>
#include <assert.h>

namespace Jack
{

inline int int2pow2(int x)	{ int r = 0; while ((1 << r) < x) r++; return r; }

static inline void CopyAndConvertIn(jack_sample_t *dst, void *src, size_t nframes, int channel, int chcount, int bits)
{
    switch (bits) {

		case 16: {
		    signed short *s16src = (signed short*)src;
            s16src += channel;
            sample_move_dS_s16(dst, (char*)s16src, nframes, chcount<<1);
			break;
        }
		case 24: {
			signed int *s32src = (signed int*)src;
            s32src += channel;
            sample_move_dS_s24(dst, (char*)s32src, nframes, chcount<<2);
			break;
        }
		case 32: {
			signed int *s32src = (signed int*)src;
            s32src += channel;
            sample_move_dS_s32u24(dst, (char*)s32src, nframes, chcount<<2);
			break;
        }
	}
}

static inline void CopyAndConvertOut(void *dst, jack_sample_t *src, size_t nframes, int channel, int chcount, int bits)
{
	switch (bits) {

		case 16: {
			signed short *s16dst = (signed short*)dst;
            s16dst += channel;
            sample_move_d16_sS((char*)s16dst, src, nframes, chcount<<1, NULL); // No dithering for now...
			break;
        }
		case 24: {
			signed int *s32dst = (signed int*)dst;
            s32dst += channel;
            sample_move_d24_sS((char*)s32dst, src, nframes, chcount<<2, NULL); // No dithering for now...
			break;
        }
		case 32: {
            signed int *s32dst = (signed int*)dst;
            s32dst += channel;
            sample_move_d32u24_sS((char*)s32dst, src, nframes, chcount<<2, NULL);
			break;
        }
	}
}

void JackOSSAdapter::SetSampleFormat()
{
    switch (fBits) {

	    case 24:	/* native-endian LSB aligned 24-bits in 32-bits  integer */
            fSampleFormat = AFMT_S24_NE;
            fSampleSize = sizeof(int);
			break;
		case 32:	/* native-endian 32-bit integer */
            fSampleFormat = AFMT_S32_NE;
            fSampleSize = sizeof(int);
			break;
		case 16:	/* native-endian 16-bit integer */
		default:
            fSampleFormat = AFMT_S16_NE;
            fSampleSize = sizeof(short);
			break;
    }
}

JackOSSAdapter::JackOSSAdapter(jack_nframes_t buffer_size, jack_nframes_t sample_rate, const JSList* params)
                :JackAudioAdapterInterface(buffer_size, sample_rate)
                ,fThread(this),
                fInFD(-1), fOutFD(-1), fBits(OSS_DRIVER_DEF_BITS),
                fSampleFormat(0), fNperiods(OSS_DRIVER_DEF_NPERIODS), fRWMode(0), fIgnoreHW(true), fExcl(false),
                fInputBufferSize(0), fOutputBufferSize(0),
                fInputBuffer(NULL), fOutputBuffer(NULL), fFirstCycle(true)
{
    const JSList* node;
    const jack_driver_param_t* param;

    fCaptureChannels = 2;
    fPlaybackChannels = 2;

    strcpy(fCaptureDriverName, OSS_DRIVER_DEF_DEV);
    strcpy(fPlaybackDriverName, OSS_DRIVER_DEF_DEV);

    for (node = params; node; node = jack_slist_next(node)) {
        param = (const jack_driver_param_t*) node->data;

        switch (param->character) {

            case 'r':
                SetAdaptedSampleRate(param->value.ui);
                break;

            case 'p':
                SetAdaptedBufferSize(param->value.ui);
                break;

            case 'n':
                fNperiods = param->value.ui;
                break;

            case 'w':
                fBits = param->value.i;
                break;

            case 'i':
                fCaptureChannels = param->value.ui;
                break;

            case 'o':
                fPlaybackChannels = param->value.ui;
                break;

            case 'e':
                fExcl = true;
                break;

            case 'C':
                fRWMode |= kRead;
                if (strcmp(param->value.str, "none") != 0) {
                   strcpy(fCaptureDriverName, param->value.str);
                }
                break;

            case 'P':
                fRWMode |= kWrite;
                if (strcmp(param->value.str, "none") != 0) {
                   strcpy(fPlaybackDriverName, param->value.str);
                }
                break;

            case 'd':
               fRWMode |= kRead;
               fRWMode |= kWrite;
               strcpy(fCaptureDriverName, param->value.str);
               strcpy(fPlaybackDriverName, param->value.str);
               break;

            case 'b':
                fIgnoreHW = true;
                break;

            case 'q':
                fQuality = param->value.ui;
                break;

            case 'g':
                fRingbufferCurSize = param->value.ui;
                fAdaptative = false;
                break;

           }
    }

    fRWMode |= kRead;
    fRWMode |= kWrite;
}

void JackOSSAdapter::DisplayDeviceInfo()
{
    audio_buf_info info;
    oss_audioinfo ai_in, ai_out;
    memset(&info, 0, sizeof(audio_buf_info));
    int cap = 0;

    // Duplex cards : http://manuals.opensound.com/developer/full_duplex.html

    jack_info("Audio Interface Description :");
    jack_info("Sampling Frequency : %d, Sample Format : %d, Mode : %d", fAdaptedSampleRate, fSampleFormat, fRWMode);

    if (fRWMode & kWrite) {

       oss_sysinfo si;
        if (ioctl(fOutFD, OSS_SYSINFO, &si) == -1) {
            jack_error("JackOSSAdapter::DisplayDeviceInfo OSS_SYSINFO failed : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        } else {
            jack_info("OSS product %s", si.product);
            jack_info("OSS version %s", si.version);
            jack_info("OSS version num %d", si.versionnum);
            jack_info("OSS numaudios %d", si.numaudios);
            jack_info("OSS numaudioengines %d", si.numaudioengines);
            jack_info("OSS numcards %d", si.numcards);
        }

        jack_info("Output capabilities - %d channels : ", fPlaybackChannels);
        jack_info("Output block size = %d", fOutputBufferSize);

        if (ioctl(fOutFD, SNDCTL_DSP_GETOSPACE, &info) == -1)  {
            jack_error("JackOSSAdapter::DisplayDeviceInfo SNDCTL_DSP_GETOSPACE failed : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        } else {
            jack_info("output space info: fragments = %d, fragstotal = %d, fragsize = %d, bytes = %d",
                info.fragments, info.fragstotal, info.fragsize, info.bytes);
        }

        if (ioctl(fOutFD, SNDCTL_DSP_GETCAPS, &cap) == -1)  {
            jack_error("JackOSSAdapter::DisplayDeviceInfo SNDCTL_DSP_GETCAPS failed : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        } else {
            if (cap & DSP_CAP_DUPLEX) 	jack_info(" DSP_CAP_DUPLEX");
            if (cap & DSP_CAP_REALTIME) jack_info(" DSP_CAP_REALTIME");
            if (cap & DSP_CAP_BATCH) 	jack_info(" DSP_CAP_BATCH");
            if (cap & DSP_CAP_COPROC) 	jack_info(" DSP_CAP_COPROC");
            if (cap & DSP_CAP_TRIGGER)  jack_info(" DSP_CAP_TRIGGER");
            if (cap & DSP_CAP_MMAP) 	jack_info(" DSP_CAP_MMAP");
            if (cap & DSP_CAP_MULTI) 	jack_info(" DSP_CAP_MULTI");
            if (cap & DSP_CAP_BIND) 	jack_info(" DSP_CAP_BIND");
        }
    }

    if (fRWMode & kRead) {

        oss_sysinfo si;
        if (ioctl(fInFD, OSS_SYSINFO, &si) == -1) {
            jack_error("JackOSSAdapter::DisplayDeviceInfo OSS_SYSINFO failed : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        } else {
            jack_info("OSS product %s", si.product);
            jack_info("OSS version %s", si.version);
            jack_info("OSS version num %d", si.versionnum);
            jack_info("OSS numaudios %d", si.numaudios);
            jack_info("OSS numaudioengines %d", si.numaudioengines);
            jack_info("OSS numcards %d", si.numcards);
        }

        jack_info("Input capabilities - %d channels : ", fCaptureChannels);
        jack_info("Input block size = %d", fInputBufferSize);

        if (ioctl(fInFD, SNDCTL_DSP_GETOSPACE, &info) == -1) {
            jack_error("JackOSSAdapter::DisplayDeviceInfo SNDCTL_DSP_GETOSPACE failed : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        } else {
            jack_info("input space info: fragments = %d, fragstotal = %d, fragsize = %d, bytes = %d",
                info.fragments, info.fragstotal, info.fragsize, info.bytes);
        }

        if (ioctl(fInFD, SNDCTL_DSP_GETCAPS, &cap) == -1) {
            jack_error("JackOSSAdapter::DisplayDeviceInfo SNDCTL_DSP_GETCAPS failed : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        } else {
            if (cap & DSP_CAP_DUPLEX) 	jack_info(" DSP_CAP_DUPLEX");
            if (cap & DSP_CAP_REALTIME) jack_info(" DSP_CAP_REALTIME");
            if (cap & DSP_CAP_BATCH) 	jack_info(" DSP_CAP_BATCH");
            if (cap & DSP_CAP_COPROC) 	jack_info(" DSP_CAP_COPROC");
            if (cap & DSP_CAP_TRIGGER)  jack_info(" DSP_CAP_TRIGGER");
            if (cap & DSP_CAP_MMAP) 	jack_info(" DSP_CAP_MMAP");
            if (cap & DSP_CAP_MULTI) 	jack_info(" DSP_CAP_MULTI");
            if (cap & DSP_CAP_BIND) 	jack_info(" DSP_CAP_BIND");
        }
    }

    if (ioctl(fInFD, SNDCTL_AUDIOINFO, &ai_in) != -1) {
        jack_info("Using audio engine %d = %s for input", ai_in.dev, ai_in.name);
    }

    if (ioctl(fOutFD, SNDCTL_AUDIOINFO, &ai_out) != -1) {
        jack_info("Using audio engine %d = %s for output", ai_out.dev, ai_out.name);
    }

    if (ai_in.rate_source != ai_out.rate_source) {
        jack_info("Warning : input and output are not necessarily driven by the same clock!");
    }
}

int JackOSSAdapter::OpenInput()
{
    int flags = 0;
    int gFragFormat;
    int cur_sample_format, cur_capture_channels;
    jack_nframes_t cur_sample_rate;

    if (fCaptureChannels == 0) fCaptureChannels = 2;

    if ((fInFD = open(fCaptureDriverName, O_RDONLY | ((fExcl) ? O_EXCL : 0))) < 0) {
        jack_error("JackOSSAdapter::OpenInput failed to open device : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        return -1;
    }

    if (fExcl) {
        if (ioctl(fInFD, SNDCTL_DSP_COOKEDMODE, &flags) == -1) {
            jack_error("JackOSSAdapter::OpenInput failed to set cooked mode : %s@%i, errno = %d", __FILE__, __LINE__, errno);
            goto error;
        }
    }

    gFragFormat = (2 << 16) + int2pow2(fAdaptedBufferSize * fSampleSize * fCaptureChannels);
    if (ioctl(fInFD, SNDCTL_DSP_SETFRAGMENT, &gFragFormat) == -1) {
        jack_error("JackOSSAdapter::OpenInput failed to set fragments : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }

    cur_sample_format = fSampleFormat;
    if (ioctl(fInFD, SNDCTL_DSP_SETFMT, &fSampleFormat) == -1) {
        jack_error("JackOSSAdapter::OpenInput failed to set format : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }
    if (cur_sample_format != fSampleFormat) {
        jack_info("JackOSSAdapter::OpenInput driver forced the sample format %ld", fSampleFormat);
    }

    cur_capture_channels = fCaptureChannels;
    if (ioctl(fInFD, SNDCTL_DSP_CHANNELS, &fCaptureChannels) == -1) {
        jack_error("JackOSSAdapter::OpenInput failed to set channels : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }
    if (cur_capture_channels != fCaptureChannels) {
        jack_info("JackOSSAdapter::OpenInput driver forced the number of capture channels %ld", fCaptureChannels);
    }

    cur_sample_rate = fAdaptedSampleRate;
    if (ioctl(fInFD, SNDCTL_DSP_SPEED, &fAdaptedSampleRate) == -1) {
        jack_error("JackOSSAdapter::OpenInput failed to set sample rate : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }
    if (cur_sample_rate != fAdaptedSampleRate) {
        jack_info("JackOSSAdapter::OpenInput driver forced the sample rate %ld", fAdaptedSampleRate);
    }

    fInputBufferSize = 0;
    if (ioctl(fInFD, SNDCTL_DSP_GETBLKSIZE, &fInputBufferSize) == -1) {
        jack_error("JackOSSAdapter::OpenInput failed to get fragments : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }

    if (fInputBufferSize != fAdaptedBufferSize * fSampleSize * fCaptureChannels) {
       if (fIgnoreHW) {
           jack_info("JackOSSAdapter::OpenInput driver forced buffer size %ld", fOutputBufferSize);
       } else {
           jack_error("JackOSSAdapter::OpenInput wanted buffer size cannot be obtained");
           goto error;
       }
    }

    fInputBuffer = (void*)calloc(fInputBufferSize, 1);
    assert(fInputBuffer);

    fInputSampleBuffer = (float**)malloc(fCaptureChannels * sizeof(float*));
    assert(fInputSampleBuffer);

    for (int i = 0; i < fCaptureChannels; i++) {
        fInputSampleBuffer[i] = (float*)malloc(fAdaptedBufferSize * sizeof(float));
        assert(fInputSampleBuffer[i]);
    }
    return 0;

error:
    ::close(fInFD);
    return -1;
}

int JackOSSAdapter::OpenOutput()
{
    int flags = 0;
    int gFragFormat;
    int cur_sample_format, cur_playback_channels;
    jack_nframes_t cur_sample_rate;

    if (fPlaybackChannels == 0) fPlaybackChannels = 2;

    if ((fOutFD = open(fPlaybackDriverName, O_WRONLY | ((fExcl) ? O_EXCL : 0))) < 0) {
       jack_error("JackOSSAdapter::OpenOutput failed to open device : %s@%i, errno = %d", __FILE__, __LINE__, errno);
       return -1;
    }

    if (fExcl) {
        if (ioctl(fOutFD, SNDCTL_DSP_COOKEDMODE, &flags) == -1) {
            jack_error("JackOSSAdapter::OpenOutput failed to set cooked mode : %s@%i, errno = %d", __FILE__, __LINE__, errno);
            goto error;
        }
    }

    gFragFormat = (2 << 16) + int2pow2(fAdaptedBufferSize * fSampleSize * fPlaybackChannels);
    if (ioctl(fOutFD, SNDCTL_DSP_SETFRAGMENT, &gFragFormat) == -1) {
        jack_error("JackOSSAdapter::OpenOutput failed to set fragments : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }

    cur_sample_format = fSampleFormat;
    if (ioctl(fOutFD, SNDCTL_DSP_SETFMT, &fSampleFormat) == -1) {
        jack_error("JackOSSAdapter::OpenOutput failed to set format : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }
    if (cur_sample_format != fSampleFormat) {
        jack_info("JackOSSAdapter::OpenOutput driver forced the sample format %ld", fSampleFormat);
    }

    cur_playback_channels = fPlaybackChannels;
    if (ioctl(fOutFD, SNDCTL_DSP_CHANNELS, &fPlaybackChannels) == -1) {
        jack_error("JackOSSAdapter::OpenOutput failed to set channels : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }
    if (cur_playback_channels != fPlaybackChannels) {
        jack_info("JackOSSAdapter::OpenOutput driver forced the number of playback channels %ld", fPlaybackChannels);
    }

    cur_sample_rate = fAdaptedSampleRate;
    if (ioctl(fOutFD, SNDCTL_DSP_SPEED, &fAdaptedSampleRate) == -1) {
        jack_error("JackOSSAdapter::OpenOutput failed to set sample rate : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }
    if (cur_sample_rate != fAdaptedSampleRate) {
        jack_info("JackOSSAdapter::OpenInput driver forced the sample rate %ld", fAdaptedSampleRate);
    }

    fOutputBufferSize = 0;
    if (ioctl(fOutFD, SNDCTL_DSP_GETBLKSIZE, &fOutputBufferSize) == -1) {
        jack_error("JackOSSAdapter::OpenOutput failed to get fragments : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }

    if (fOutputBufferSize != fAdaptedBufferSize * fSampleSize * fPlaybackChannels) {
       if (fIgnoreHW) {
           jack_info("JackOSSAdapter::OpenOutput driver forced buffer size %ld", fOutputBufferSize);
       } else {
           jack_error("JackOSSAdapter::OpenInput wanted buffer size cannot be obtained");
           goto error;
       }
    }

    fOutputBuffer = (void*)calloc(fOutputBufferSize, 1);
    assert(fOutputBuffer);

    fOutputSampleBuffer = (float**)malloc(fPlaybackChannels * sizeof(float*));
    assert(fOutputSampleBuffer);

    for (int i = 0; i < fPlaybackChannels; i++) {
        fOutputSampleBuffer[i] = (float*)malloc(fAdaptedBufferSize * sizeof(float));
        assert(fOutputSampleBuffer[i]);
    }

    fFirstCycle = true;
    return 0;

error:
    ::close(fOutFD);
    return -1;
}

int JackOSSAdapter::Open()
{
    SetSampleFormat();

    if ((fRWMode & kRead) && (OpenInput() < 0)) {
        return -1;
    }

    if ((fRWMode & kWrite) && (OpenOutput() < 0)) {
       return -1;
    }

    // In duplex mode, check that input and output use the same buffer size
    if ((fRWMode & kRead) && (fRWMode & kWrite) && (fInputBufferSize != fOutputBufferSize)) {
       jack_error("JackOSSAdapter::OpenAux input and output buffer size are not the same!!");
       goto error;
    }

    DisplayDeviceInfo();

    //start adapter thread
    if (fThread.StartSync() < 0) {
        jack_error ( "Cannot start audioadapter thread" );
        return -1;
    }

    //turn the thread realtime
    fThread.AcquireRealTime(JackServerGlobals::fInstance->GetEngineControl()->fClientPriority);
    return 0;

error:
    CloseAux();
    return -1;
}


int JackOSSAdapter::Close()
{
#ifdef JACK_MONITOR
    fTable.Save(fHostBufferSize, fHostSampleRate, fAdaptedSampleRate, fAdaptedBufferSize);
#endif
    fThread.Stop();
    CloseAux();
    return 0;
}

void JackOSSAdapter::CloseAux()
{
    if (fRWMode & kRead) {
        close(fInFD);
        fInFD = -1;
    }

    if (fRWMode & kWrite) {
        close(fOutFD);
        fOutFD = -1;
    }

    free(fInputBuffer);
    fInputBuffer = NULL;

    free(fOutputBuffer);
    fOutputBuffer = NULL;

    for (int i = 0; i < fCaptureChannels; i++) {
        free(fInputSampleBuffer[i]);
    }
    free(fInputSampleBuffer);

    for (int i = 0; i < fPlaybackChannels; i++) {
        free(fOutputSampleBuffer[i]);
    }
    free(fOutputSampleBuffer);
 }

int JackOSSAdapter::Read()
{
    ssize_t count = ::read(fInFD, fInputBuffer, fInputBufferSize);

    if (count < fInputBufferSize) {
        jack_error("JackOSSAdapter::Read error bytes read = %ld", count);
        return -1;
    } else {
        for (int i = 0; i < fCaptureChannels; i++) {
             CopyAndConvertIn(fInputSampleBuffer[i], fInputBuffer, fAdaptedBufferSize, i, fCaptureChannels, fBits);
        }
        return 0;
    }
}

int JackOSSAdapter::Write()
{
    ssize_t count;

    // Maybe necessay to write an empty output buffer first time : see http://manuals.opensound.com/developer/fulldup.c.html
    if (fFirstCycle) {

        fFirstCycle = false;
        memset(fOutputBuffer, 0, fOutputBufferSize);

        // Prefill ouput buffer
        for (int i = 0; i < fNperiods; i++) {
           count = ::write(fOutFD, fOutputBuffer, fOutputBufferSize);
           if (count < fOutputBufferSize) {
                jack_error("JackOSSDriver::Write error bytes written = %ld", count);
                return -1;
            }
        }

        int delay;
        if (ioctl(fOutFD, SNDCTL_DSP_GETODELAY, &delay) == -1) {
            jack_error("JackOSSDriver::Write error get out delay : %s@%i, errno = %d", __FILE__, __LINE__, errno);
            return -1;
        }

        delay /= fSampleSize * fPlaybackChannels;
        jack_info("JackOSSDriver::Write output latency frames = %ld", delay);
    }

    for (int i = 0; i < fPlaybackChannels; i++) {
        CopyAndConvertOut(fOutputBuffer, fOutputSampleBuffer[i], fAdaptedBufferSize, i, fCaptureChannels, fBits);
    }

    count = ::write(fOutFD, fOutputBuffer, fOutputBufferSize);

    if (count < fOutputBufferSize) {
        jack_error("JackOSSAdapter::Write error bytes written = %ld", count);
        return -1;
    } else {
        return 0;
    }
}

bool JackOSSAdapter::Execute()
{
    //read data from audio interface
    if (Read() < 0)
        return false;

    PushAndPull(fInputSampleBuffer, fOutputSampleBuffer, fAdaptedBufferSize);

    //write data to audio interface
    if (Write() < 0)
        return false;

    return true;
}

int JackOSSAdapter::SetBufferSize(jack_nframes_t buffer_size)
{
    JackAudioAdapterInterface::SetBufferSize(buffer_size);
    Close();
    return Open();
}

} // namespace

#ifdef __cplusplus
extern "C"
{
#endif

    SERVER_EXPORT jack_driver_desc_t* jack_get_descriptor()
    {
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("audioadapter", JackDriverNone, "netjack audio <==> net backend adapter", &filler);

        value.ui = OSS_DRIVER_DEF_FS;
        jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

        value.ui = OSS_DRIVER_DEF_BLKSIZE;
        jack_driver_descriptor_add_parameter(desc, &filler, "period", 'p', JackDriverParamUInt, &value, NULL, "Frames per period", NULL);

        value.ui = OSS_DRIVER_DEF_NPERIODS;
        jack_driver_descriptor_add_parameter(desc, &filler, "nperiods", 'n', JackDriverParamUInt, &value, NULL, "Number of periods to prefill output buffer", NULL);

        value.i = OSS_DRIVER_DEF_BITS;
        jack_driver_descriptor_add_parameter(desc, &filler, "wordlength", 'w', JackDriverParamInt, &value, NULL, "Word length", NULL);

        value.ui = OSS_DRIVER_DEF_INS;
        jack_driver_descriptor_add_parameter(desc, &filler, "in-channels", 'i', JackDriverParamUInt, &value, NULL, "Capture channels", NULL);

        value.ui = OSS_DRIVER_DEF_OUTS;
        jack_driver_descriptor_add_parameter(desc, &filler, "out-channels", 'o', JackDriverParamUInt, &value, NULL, "Playback channels", NULL);

        value.i  = false;
        jack_driver_descriptor_add_parameter(desc, &filler, "excl", 'e', JackDriverParamBool, &value, NULL, "Exclusif (O_EXCL) access mode", NULL);

        strcpy(value.str, OSS_DRIVER_DEF_DEV);
        jack_driver_descriptor_add_parameter(desc, &filler, "capture", 'C', JackDriverParamString, &value, NULL, "Input device", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "playback", 'P', JackDriverParamString, &value, NULL, "Output device", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "device", 'd', JackDriverParamString, &value, NULL, "OSS device name", NULL);

        value.i  = true;
        jack_driver_descriptor_add_parameter(desc, &filler, "ignorehwbuf", 'b', JackDriverParamBool, &value, NULL, "Ignore hardware period size", NULL);

        value.ui  = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "quality", 'q', JackDriverParamInt, &value, NULL, "Resample algorithm quality (0 - 4)", NULL);

        value.i = 32768;
        jack_driver_descriptor_add_parameter(desc, &filler, "ring-buffer", 'g', JackDriverParamInt, &value, NULL, "Fixed ringbuffer size", "Fixed ringbuffer size (if not set => automatic adaptative)");

        return desc;
    }

#ifdef __cplusplus
}
#endif

