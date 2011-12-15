//
//  main.m
//  iPhoneNet
//
//  Created by Stéphane  LETZ on 16/02/09.
//  Copyright Grame 2009. All rights reserved.
//

#import <UIKit/UIKit.h>
#include <jack/net.h>

#include "TiPhoneCoreAudioRenderer.h"

#define NUM_INPUT 2
#define NUM_OUTPUT 2

jack_net_master_t* net;
jack_adapter_t* adapter;

float** audio_input_buffer = NULL;
float** audio_output_buffer = NULL;

int buffer_size = 1024;
int sample_rate = 22050;
//int sample_rate = 32000;

jack_master_t request = { -1, -1, -1, -1, buffer_size, sample_rate, "master" };
jack_slave_t result;

static void MixAudio(float** dst, float** src1, float** src2, int channels, int buffer_size)
{
    for (int chan = 0; chan < channels; chan++) {
        for (int frame = 0; frame < buffer_size; frame++) {
            dst[chan][frame] = src1[chan][frame] + src2[chan][frame];
        }
    }
}

static void MasterAudioCallback(int frames, float** inputs, float** outputs, void* arg)
{
    int i;

    // Copy from iPod input to network buffers
    for (i = 0; i < result.audio_input; i++) {
        memcpy(audio_input_buffer[i], inputs[i], buffer_size * sizeof(float));
    }

    /*
    // Copy from network out buffers to network in buffers (audio thru)
    for (i = 0; i < result.audio_input; i++) {
        memcpy(audio_input_buffer[i], audio_output_buffer[i], buffer_size * sizeof(float));
    }
    */

    // Mix iPod input and network in buffers to network out buffers
    //MixAudio(audio_input_buffer, inputs, audio_output_buffer, result.audio_input, buffer_size);

    // Send network buffers
    if (jack_net_master_send(net, result.audio_input, audio_input_buffer, 0, NULL) < 0) {
        printf("jack_net_master_send error..\n");
    }

    // Recv network buffers
    if (jack_net_master_recv(net, result.audio_output, audio_output_buffer, 0, NULL) < 0) {
        printf("jack_net_master_recv error..\n");
    }

    // Copy from network buffers to iPod output
    for (i = 0; i < result.audio_output; i++) {
        memcpy(outputs[i], audio_output_buffer[i], buffer_size * sizeof(float));
    }
}

int main(int argc, char *argv[]) {

    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    int i;

    if ((net = jack_net_master_open(DEFAULT_MULTICAST_IP, DEFAULT_PORT, "iPhone", &request, &result))  == 0) {
        printf("jack_net_master_open error..\n");
        return -1;
    }

    TiPhoneCoreAudioRenderer audio_device(result.audio_input, result.audio_output);

    // Allocate buffers
    if (result.audio_input > 0) {
        audio_input_buffer = (float**)calloc(result.audio_input, sizeof(float*));
        for (i = 0; i < result.audio_input; i++) {
            audio_input_buffer[i] = (float*)(calloc(buffer_size, sizeof(float)));
        }
    }

    if (result.audio_output > 0) {
        audio_output_buffer = (float**)calloc(result.audio_output, sizeof(float*));
        for (i = 0; i < result.audio_output; i++) {
            audio_output_buffer[i] = (float*)(calloc(buffer_size, sizeof(float)));
        }
    }

    if (audio_device.Open(buffer_size, sample_rate) < 0) {
        return -1;
    }

    audio_device.SetAudioCallback(MasterAudioCallback, NULL);

    if (audio_device.Start() < 0) {
        return -1;
    }

    /*
    // Quite brutal way, the application actually does not start completely, the netjack audio processing loop is used instead...
    // Run until interrupted

    int wait_usec = (unsigned long)((((float)buffer_size) / ((float)sample_rate)) * 1000000.0f);

  	while (1) {

        // Copy input to output
        for (i = 0; i < result.audio_input; i++) {
            memcpy(audio_output_buffer[i], audio_input_buffer[i], buffer_size * sizeof(float));
        }

        if (jack_net_master_send(net, result.audio_output, audio_output_buffer, 0, NULL) < 0) {
            printf("jack_net_master_send error..\n");
        }

        if (jack_net_master_recv(net, result.audio_input, audio_input_buffer, 0, NULL) < 0) {
            printf("jack_net_master_recv error..\n");
        }
        usleep(wait_usec);
	};
    */

    int retVal = UIApplicationMain(argc, argv, nil, nil);

    audio_device.Stop();
    audio_device.Close();

    // Wait for application end
    jack_net_master_close(net);

    for (i = 0; i < result.audio_input; i++) {
        free(audio_input_buffer[i]);
    }
    free(audio_input_buffer);

    for (i = 0; i < result.audio_output; i++) {
        free(audio_output_buffer[i]);
    }
    free(audio_output_buffer);

    [pool release];
    return retVal;
}
