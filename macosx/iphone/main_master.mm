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

float** audio_input_buffer;
float** audio_output_buffer;

int buffer_size = 2048;
int sample_rate = 44100;

jack_master_t request = { buffer_size, sample_rate, "master" };
jack_slave_t result;

void MasterAudioCallback(int frames, float** inputs, float** outputs, void* arg)
{
    int i; 
    
    for (i = 0; i < result.audio_input; i++) {
        memcpy(audio_output_buffer[i], inputs[i], buffer_size * sizeof(float));
    }
   
    if (jack_net_master_send(net, result.audio_output, audio_output_buffer, 0, NULL) < 0) {
        printf("jack_net_master_send error..\n");
    }
    
    if (jack_net_master_recv(net, result.audio_input, audio_input_buffer, 0, NULL) < 0) {
        printf("jack_net_master_recv error..\n");
    }
    
    for (i = 0; i < result.audio_output; i++) {
        memcpy(outputs[i], audio_input_buffer[i], buffer_size * sizeof(float));
    }
}

int main(int argc, char *argv[]) {
    
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    
    int i;
    
    TiPhoneCoreAudioRenderer audio_device(NUM_INPUT, NUM_OUTPUT);
 
    if ((net = jack_net_master_open(DEFAULT_MULTICAST_IP, DEFAULT_PORT, "iPhone", &request, &result))  == 0) {
        return -1;
    }
    
    // Allocate buffers
    audio_input_buffer = (float**)calloc(result.audio_input, sizeof(float*));
    for (i = 0; i < result.audio_input; i++) {
        audio_input_buffer[i] = (float*)(calloc(buffer_size, sizeof(float)));
    }
    
    audio_output_buffer = (float**)calloc(result.audio_output, sizeof(float*));
    for (i = 0; i < result.audio_output; i++) {
        audio_output_buffer[i] = (float*)(calloc(buffer_size, sizeof(float)));
    }
    
    if (audio_device.OpenDefault(buffer_size, sample_rate) < 0) {
        return -1;
    }
    
    audio_device.SetAudioCallback(MasterAudioCallback, NULL);
   
    if (audio_device.Start() < 0) {
        return -1;
    }
    
     // Run until interrupted 
  	while (1) {}
    
    audio_device.Stop();
    audio_device.Close();

    /*
    // Quite brutal way, the application actually does not start completely, the netjack audio processing loop is used instead...
    // Run until interrupted 
  	while (1) {
    
        // Copy input to output
        for (i = 0; i < result.audio_input; i++) {
            memcpy(audio_output_buffer[i], audio_input_buffer[i], buffer_size * sizeof(float));
        }
        
        if (jack_net_master_send(net, result.audio_output, audio_output_buffer, 0, NULL) < 0) {
            printf("jack_net_master_send error..\n");
            break;
        }
        
        if (jack_net_master_recv(net, result.audio_input, audio_input_buffer, 0, NULL) < 0) {
            printf("jack_net_master_recv error..\n");
            break;
        }
        usleep(wait_usec);
	};
    */
    
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
   
    //int retVal = UIApplicationMain(argc, argv, nil, nil);
    [pool release];
    
    //return retVal;
    return 0;
}
