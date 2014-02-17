/*
Copyright (C) 2013 Matt Flax <flatmax@flatmax.org>

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

#include "JackIIODriver.h"
#include "driver_interface.h"
#include "JackEngineControl.h"
#include "JackGraphManager.h"

#include <values.h>

#define IIO_DEFAULT_CHIP "AD7476A" ///< The default IIO recording chip to look for.
#define IIO_DEFAULT_READ_FS 1.e6 ///< The default IIO sample rate for the default chip.
#define IIO_DEFAULT_PERIOD_SIZE 2048 ///< The default period size is in the ms range
#define IIO_DEFAULT_PERIOD_COUNT 2 ///< The default number of periods
#define IIO_DEFAULT_CAPUTURE_PORT_COUNT MAXINT ///< The default number of capture ports is exceedingly big, trimmed down to a realistic size in driver_initialize
//#define IIO_SAFETY_FACTOR 2./3. ///< The default safety factor, allow consumption of this fraction of the available DMA buffer before we don't allow the driver to continue.
#define IIO_SAFETY_FACTOR 1. ///< The default safety factor, allow consumption of this fraction of the available DMA buffer before we don't allow the driver to continue.

namespace Jack {

int JackIIODriver::Open(jack_nframes_t buffer_size, jack_nframes_t samplerate, bool capturing, bool playing, int inchannels, int outchannels, bool monitor, const char* capture_driver_name, const char* playback_driver_name, jack_nframes_t capture_latency, jack_nframes_t playback_latency) {
    //cout<<"JackIIODriver::Open\n";

    int ret=JackAudioDriver::Open(buffer_size, samplerate, capturing, playing, inchannels, outchannels, monitor, capture_driver_name, playback_driver_name, capture_latency, playback_latency);
    if (ret!=NO_ERROR) // check whether the JackAudioDriver opened OK
        return ret;
    ret=iio.enable(true); // start the DMA
    return ret;
}

int JackIIODriver::Close() {
    //cout<<"JackIIODriver::Close\n";
    iio.enable(false); // stop the DMA
    return JackAudioDriver::Close();
}

int JackIIODriver::Read() {
    //cout<<"JackIIODriver::Read\n";

    if (iio.getDeviceCnt()<1) {
        jack_error("JackIIODriver:: No IIO devices are present ");
        return -1;
    }
    uint devChCnt=iio[0].getChCnt(); // the number of channels per device

    jack_nframes_t nframes=data.rows()/devChCnt;

    // This is left here for future debugging.
    //    if (nframes != fEngineControl->fBufferSize)
    //        jack_error("JackIIODriver::Read warning : Jack period size = %ld IIO period size = %ld", fEngineControl->fBufferSize, nframes);
    //    cout<<"processing buffer size : "<<fEngineControl->fBufferSize<<endl;
    //    cout<<"processing channel count : "<<fCaptureChannels<<endl;

    int ret=iio.read(nframes, data); // read the data from the IIO subsystem
    if (ret!=NO_ERROR)
        return -1;


    // Keep begin cycle time
    JackDriver::CycleTakeBeginTime(); // is this necessary ?

    jack_default_audio_sample_t scaleFactor=1./32768.;

    // This is left in for future debugging.
    //int maxAvailChCnt=data.cols()*devChCnt;
    //    if (fCaptureChannels>maxAvailChCnt)
    //        jack_error("JackIIODriver::Read warning : Jack capture ch. cnt = %ld IIO capture ch. cnt = %ld", fCaptureChannels, maxAvailChCnt);

    for (int i = 0; i < fCaptureChannels; i++) {
        int col=i/devChCnt; // find the column and offset to read from
        int rowOffset=i%devChCnt;
        if (fGraphManager->GetConnectionsNum(fCapturePortList[i]) > 0) {
            jack_default_audio_sample_t *dest=GetInputBuffer(i);

            for (jack_nframes_t j=0; j<nframes; j++)
                dest[j]=(jack_default_audio_sample_t)(data(j*devChCnt+rowOffset, col))*scaleFactor;
        }
    }

    return 0;
}

int JackIIODriver::Write() {
    //        cout<<"JackIIODriver::Write\n";
    JackDriver::CycleTakeEndTime(); // is this necessary ?
    return 0;
}

} // end namespace Jack


#ifdef __cplusplus
extern "C"
{
#endif

SERVER_EXPORT const jack_driver_desc_t *
driver_get_descriptor () {
    jack_driver_desc_t * desc;
    jack_driver_desc_filler_t filler;
    jack_driver_param_value_t value;

    desc = jack_driver_descriptor_construct("iio", JackDriverMaster, "Linux Industrial IO backend", &filler);

    strcpy(value.str, IIO_DEFAULT_CHIP);
    jack_driver_descriptor_add_parameter(desc, &filler, "chip", 'C', JackDriverParamString, &value, NULL, "The name of the chip to search for in the IIO devices", NULL);

    value.ui = IIO_DEFAULT_CAPUTURE_PORT_COUNT;
    jack_driver_descriptor_add_parameter(desc, &filler, "capture", 'i', JackDriverParamUInt, &value, NULL, "Provide capture count (block size).", NULL);

    value.ui = IIO_DEFAULT_PERIOD_SIZE;
    jack_driver_descriptor_add_parameter(desc, &filler, "period", 'p', JackDriverParamUInt, &value, NULL, "Frames (samples per channel) per period", NULL);

    value.ui = IIO_DEFAULT_PERIOD_COUNT;
    jack_driver_descriptor_add_parameter(desc, &filler, "nperiods", 'n', JackDriverParamUInt, &value, NULL, "Number of available periods (block count)", NULL);

    return desc;
}

SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params) {

    // As of this implementation the IIO driver is only capture... to be expanded.
    int ret, colCnt;
    Jack::JackDriverClientInterface* threaded_driver=NULL;

    string chipName(IIO_DEFAULT_CHIP); // the default chip name to search for in the IIO devices.
    float fs = IIO_DEFAULT_READ_FS; // IIO sample rate is fixed.
    jack_nframes_t periodSize = IIO_DEFAULT_PERIOD_SIZE; // default block size
    jack_nframes_t periodCount = IIO_DEFAULT_PERIOD_COUNT; // default block count
    uint inChCnt = IIO_DEFAULT_CAPUTURE_PORT_COUNT; // The default number of physical input channels - a very large number, to be reduced.

    for (const JSList *node = params; node; node = jack_slist_next (node)) {
        jack_driver_param_t *param = (jack_driver_param_t *) node->data;

        switch (param->character) {
        case 'C': // we are specifying a new chip name
            chipName = param->value.str;
            break;
        case 'i': // we are specifying the number of capture channels
            inChCnt = param->value.ui;
            break;
        case 'p':
            periodSize = param->value.ui;
            break;
        case 'n':
            periodCount = param->value.ui;
            break;
        }
    }

    // create the driver which contains the IIO class
    Jack::JackIIODriver* iio_driver = new Jack::JackIIODriver("system", "iio_pcm", engine, table);
    if (!iio_driver) {
        jack_error("\nHave you run out of memory ? I tried to create the IIO driver in memory but failed!\n");
        return NULL;
    }

    // interrogate the available iio devices searching for the chip name
    if (iio_driver->iio.findDevicesByChipName(chipName)!=NO_ERROR) { // find all devices with a particular chip which are present.
        jack_error("\nThe iio driver found no devices by the name %s\n", chipName.c_str());
        goto initError;
    }

    if (iio_driver->iio.getDeviceCnt()<1) { // If there are no devices found by that chip name, then indicate.
        jack_error("\nThe iio driver found no devices by the name %s\n", chipName.c_str());
        goto initError;
    }

    iio_driver->iio.printInfo(); // print out detail about the devices which were found ...

    // if the available number of ports is less then the requested number, then restrict to the number of physical ports.
    if (iio_driver->iio.getChCnt()<inChCnt)
        inChCnt=iio_driver->iio.getChCnt();

    // resize the data buffer column count to match the device count
    colCnt=(int)ceil((float)inChCnt/(float)iio_driver->iio[0].getChCnt()); // check whether we require less then the available number of channels
    ret=iio_driver->iio.getReadArray(periodSize, iio_driver->data); // resize the array to be able to read enough memory
    if (ret!=NO_ERROR) {
        jack_error("iio::getReadArray couldn't create the data buffer, indicating the problem.");
        goto initError;
    }
    if (iio_driver->data.cols()>colCnt) // resize the data columns to match the specified number of columns (channels / channels per device)
        iio_driver->data.resize(iio_driver->data.rows(), colCnt);

    ret=iio_driver->iio.open(periodCount, periodSize); // try to open all IIO devices
    if (ret!=NO_ERROR)
        goto initError;

    threaded_driver = new Jack::JackThreadedDriver(iio_driver);
    if (threaded_driver) {
        bool capture=true, playback=false, monitor=false;
        int outChCnt=0;
        jack_nframes_t inputLatency = periodSize*periodCount, outputLatency=0;
        // Special open for OSS driver...
        if (iio_driver->Open(periodSize, (jack_nframes_t)fs, capture, playback, inChCnt, outChCnt, monitor, "iio:device", "iio:device", inputLatency, outputLatency)!=0) {
            delete threaded_driver;
            delete iio_driver;
            return NULL;
        }
    } else
        jack_error("\nHave you run out of memory ? I tried to create Jack's standard threaded driver in memory but failed! The good news is that you had enough memory to create the IIO driver.\n");

    return threaded_driver;

initError: // error during intialisation, delete and return NULL
        delete iio_driver;
        return NULL;
}

#ifdef __cplusplus
}
#endif
