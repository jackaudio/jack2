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

jack_net_slave_t* net;
jack_adapter_t* adapter;

int buffer_size;
int sample_rate ;


static int net_process(jack_nframes_t buffer_size,
                        int audio_input, 
                        float** audio_input_buffer, 
                        int midi_input,
                        void** midi_input_buffer,
                        int audio_output,
                        float** audio_output_buffer, 
                        int midi_output, 
                        void** midi_output_buffer, 
                        void* data)
{

    jack_adapter_pull_and_push(adapter, audio_output_buffer, audio_input_buffer, buffer_size);
    return 0;
}

static void SlaveAudioCallback(int frames, float** inputs, float** outputs, void* arg)
{
    jack_adapter_push_and_pull(adapter, inputs, outputs, frames);
}

int main(int argc, char *argv[]) {
    
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    
    jack_slave_t request = { NUM_INPUT, NUM_OUTPUT, 0, 0, DEFAULT_MTU, -1, JackSlowMode };
    jack_master_t result;

    if ((net = jack_net_slave_open(DEFAULT_MULTICAST_IP, DEFAULT_PORT, "iPhone", &request, &result))  == 0) {
        return -1;
    }
    
    if ((adapter = jack_create_adapter(NUM_INPUT, 
                                    NUM_OUTPUT, 
                                    result.buffer_size, 
                                    result.sample_rate, 
                                    result.buffer_size, 
                                    result.sample_rate)) == 0) {
        return -1;
    }
    
    TiPhoneCoreAudioRenderer audio_device(NUM_INPUT, NUM_OUTPUT);

    jack_set_net_slave_process_callback(net, net_process, NULL);
    if (jack_net_slave_activate(net) != 0) {
        return -1;
    }
    
    if (audio_device.OpenDefault(result.buffer_size, result.sample_rate) < 0) {
        return -1;
    }
    
    audio_device.SetAudioCallback(SlaveAudioCallback, NULL);
   
    if (audio_device.Start() < 0) {
        return -1;
    }
    
    int retVal = UIApplicationMain(argc, argv, nil, nil);
    [pool release];
    
    audio_device.Stop();
    audio_device.Close();
    
    // Wait for application end
    jack_net_slave_deactivate(net);
    jack_net_slave_close(net);
    jack_destroy_adapter(adapter);
    return retVal;
}
