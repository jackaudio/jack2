//
//  main.m
//  iPhoneNet
//
//  Created by Stéphane  LETZ on 16/02/09.
//  Copyright Grame 2009. All rights reserved.
//

#import <UIKit/UIKit.h>
#include <jack/net.h>

#define NUM_INPUT 2
#define NUM_OUTPUT 2

jack_net_slave_t* net;
jack_adapter_t* adapter;

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
    // Process input, produce output
    if (audio_input == audio_output) {
        // Copy input to output
        for (int i = 0; i < audio_input; i++) {
            memcpy(audio_output_buffer[i], audio_input_buffer[i], buffer_size * sizeof(float));
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    
    jack_slave_t request = { NUM_INPUT, NUM_OUTPUT, 0, 0, DEFAULT_MTU, -1, JackSlowMode };
    jack_master_t result;

    if ((net = jack_net_slave_open(DEFAULT_MULTICAST_IP, DEFAULT_PORT, "iPhone", &request, &result))  == 0) {
        return -1;
    }

    jack_set_net_slave_process_callback(net, net_process, NULL);
    if (jack_net_slave_activate(net) != 0) {
        return -1;
    }
    
    int retVal = UIApplicationMain(argc, argv, nil, nil);
    [pool release];
    
    // Wait for application end
    jack_net_slave_deactivate(net);
    jack_net_slave_close(net);
    return retVal;
}
