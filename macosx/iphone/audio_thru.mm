
#include "JackAudioQueueAdapter.h"

#define CHANNELS 2

static void DSPcompute(int count, float** input, float** output)
{
    for (int i = 0; i < CHANNELS; i++) {
        memcpy(output[i], input[i], count * sizeof(float));
    }
}

int main(int argc, char *argv[]) {
    
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    
    Jack::JackAudioQueueAdapter audio(2, 2, 512, 44100, DSPcompute);
    
    if (audio.Open() < 0) {
        fprintf(stderr, "Cannot open audio\n");
	    return 1;
	}
    
    // Hang around forever...
	while(1) CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.25, false);
   
    int retVal = UIApplicationMain(argc, argv, nil, nil);
    [pool release];
     
    if (audio.Close() < 0) {
        fprintf(stderr, "Cannot close audio\n");
	}
    
    return retVal;
}
