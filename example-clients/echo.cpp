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

/* link with  */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <vector>
#include <stack>
#include <string>
#include <map>
#include <iostream>

#include <libgen.h>
#include <jack/jack.h>

// g++ -O3 -lm -lsndfile  myfx.cpp

using namespace std;


#define max(x,y) (((x)>(y)) ? (x) : (y))
#define min(x,y) (((x)<(y)) ? (x) : (y))

// abs is now predefined
//template<typename T> T abs (T a)			{ return (a<T(0)) ? -a : a; }


inline int	lsr (int x, int n)
{
    return int(((unsigned int)x) >> n);
}


/******************************************************************************
*******************************************************************************
 
							       VECTOR INTRINSICS
 
*******************************************************************************
*******************************************************************************/

inline void *aligned_calloc(size_t nmemb, size_t size)
{
    return (void*)((unsigned)(calloc((nmemb*size) + 15, sizeof(char))) + 15 & 0xfffffff0);
}

/******************************************************************************
*******************************************************************************
 
								USER INTERFACE
 
*******************************************************************************
*******************************************************************************/

class UI
{
        bool	fStopped;
    public:

        UI() : fStopped(false)
        {}
        virtual ~UI()
        {}

        // -- active widgets

        virtual void addButton(char* label, float* zone) = 0;
        virtual void addToggleButton(char* label, float* zone) = 0;
        virtual void addCheckButton(char* label, float* zone) = 0;
        virtual void addVerticalSlider(char* label, float* zone, float init, float min, float max, float step) = 0;
        virtual void addHorizontalSlider(char* label, float* zone, float init, float min, float max, float step) = 0;
        virtual void addNumEntry(char* label, float* zone, float init, float min, float max, float step) = 0;

        // -- passive widgets

        virtual void addNumDisplay(char* label, float* zone, int precision) = 0;
        virtual void addTextDisplay(char* label, float* zone, char* names[], float min, float max) = 0;
        virtual void addHorizontalBargraph(char* label, float* zone, float min, float max) = 0;
        virtual void addVerticalBargraph(char* label, float* zone, float min, float max) = 0;

        // -- frames and labels

        virtual void openFrameBox(char* label) = 0;
        virtual void openTabBox(char* label) = 0;
        virtual void openHorizontalBox(char* label) = 0;
        virtual void openVerticalBox(char* label) = 0;
        virtual void closeBox() = 0;

        virtual void show() = 0;
        virtual void run() = 0;

        void stop()
        {
            fStopped = true;
        }
        bool stopped()
        {
            return fStopped;
        }
};

struct param
{
    float* fZone;
    float fMin;
    float fMax;
    param(float* z, float a, float b) : fZone(z), fMin(a), fMax(b)
    {}
};

class CMDUI : public UI
{
        int	fArgc;
        char**	fArgv;
        stack<string>	fPrefix;
        map<string, param>	fKeyParam;

        void addOption(char* label, float* zone, float min, float max)
        {
            string fullname = fPrefix.top() + label;
            fKeyParam.insert(make_pair(fullname, param(zone, min, max)));
        }

        void openAnyBox(char* label)
        {
            string prefix;

            if (label && label[0]) {
                prefix = fPrefix.top() + "-" + label;
            } else {
                prefix = fPrefix.top();
            }
            fPrefix.push(prefix);
        }


    public:

        CMDUI(int argc, char *argv[]) : UI(), fArgc(argc), fArgv(argv)
        {
            fPrefix.push("--");
        }
        virtual ~CMDUI()
        {}

        virtual void addButton(char* label, float* zone)
        {}
        ;
        virtual void addToggleButton(char* label, float* zone)
        {}
        ;
        virtual void addCheckButton(char* label, float* zone)
        {}
        ;

        virtual void addVerticalSlider(char* label, float* zone, float init, float min, float max, float step)
        {
            addOption(label, zone, min, max);
        }

        virtual void addHorizontalSlider(char* label, float* zone, float init, float min, float max, float step)
        {
            addOption(label, zone, min, max);
        }

        virtual void addNumEntry(char* label, float* zone, float init, float min, float max, float step)
        {
            addOption(label, zone, min, max);
        }

        // -- passive widgets

        virtual void addNumDisplay(char* label, float* zone, int precision)
        {}
        virtual void addTextDisplay(char* label, float* zone, char* names[], float min, float max)
        {}
        virtual void addHorizontalBargraph(char* label, float* zone, float min, float max)
        {}
        virtual void addVerticalBargraph(char* label, float* zone, float min, float max)
        {}

        virtual void openFrameBox(char* label)
        {
            openAnyBox(label);
        }
        virtual void openTabBox(char* label)
        {
            openAnyBox(label);
        }
        virtual void openHorizontalBox(char* label)
        {
            openAnyBox(label);
        }
        virtual void openVerticalBox(char* label)
        {
            openAnyBox(label);
        }

        virtual void closeBox()
        {
            fPrefix.pop();
        }

        virtual void show()
        {}
        virtual void run()
        {
            char c;
            printf("Type 'q' to quit\n");
            while ((c = getchar()) != 'q') {
                sleep(1);
            }
        }

        void print()
        {
            map<string, param>::iterator i;
            cout << fArgc << "\n";
            cout << fArgv[0] << " option list : ";
            for (i = fKeyParam.begin(); i != fKeyParam.end(); i++) {
                cout << "[ " << i->first << " " << i->second.fMin << ".." << i->second.fMax << " ] ";
            }
            //cout << " infile outfile\n";
        }

        void process_command()
        {
            map<string, param>::iterator p;
            for (int i = 1; i < fArgc; i++) {
                if (fArgv[i][0] == '-') {
                    p = fKeyParam.find(fArgv[i]);
                    if (p == fKeyParam.end()) {
                        cout << fArgv[0] << " : unrecognized option " << fArgv[i] << "\n";
                        print();
                        exit(1);
                    }
                    char*	end;
                    *(p->second.fZone) = float(strtod(fArgv[i + 1], &end));
                    i++;
                }
            }
        }

        void process_init()
        {
            map<string, param>::iterator p;
            for (int i = 1; i < fArgc; i++) {
                if (fArgv[i][0] == '-') {
                    p = fKeyParam.find(fArgv[i]);
                    if (p == fKeyParam.end()) {
                        cout << fArgv[0] << " : unrecognized option " << fArgv[i] << "\n";
                        exit(1);
                    }
                    char*	end;
                    *(p->second.fZone) = float(strtod(fArgv[i + 1], &end));
                    i++;
                }
            }
        }
};

//----------------------------------------------------------------
//  d�inition du processeur de signal
//----------------------------------------------------------------

class dsp
{
    protected:
        int fSamplingFreq;
    public:
        dsp()
        {}
        virtual ~dsp()
        {}

        virtual int getNumInputs() = 0;
        virtual int getNumOutputs() = 0;
        virtual void buildUserInterface(UI* interface) = 0;
        virtual void init(int samplingRate) = 0;
        virtual void compute(int len, float** inputs, float** outputs) = 0;
        virtual void conclude()
        {}
};

//----------------------------------------------------------------------------
// 	FAUST generated code
//----------------------------------------------------------------------------

class mydsp : public dsp
{
    private:
        class SIG0
        {
            private:
                int fSamplingFreq;
            public:
                int getNumInputs()
                {
                    return 0;
                }
                int getNumOutputs()
                {
                    return 1;
                }
                void init(int samplingFreq)
                {
                    fSamplingFreq = samplingFreq;
                }
                void fill (int count, float output[])
                {
                    for (int i = 0; i < count; i++) {
                        output[i] = 0.000000f;
                    }
                }
        };


        float R0_0;
        float fslider0;
        float ftbl0[65536];
        int R1_0;
        float fdata0;
        float fslider1;
    public:
        virtual int getNumInputs()
        {
            return 1;
        }
        virtual int getNumOutputs()
        {
            return 1;
        }
        virtual void init(int samplingFreq)
        {
            fSamplingFreq = samplingFreq;
            R0_0 = 0.0;
            fslider0 = 0.000000f;
            SIG0 sig0;
            sig0.init(samplingFreq);
            sig0.fill(65536, ftbl0);
            R1_0 = 0;
            fdata0 = (1.000000e-03f * fSamplingFreq);
            fslider1 = 0.000000f;
        }
        virtual void buildUserInterface(UI* interface)
        {
            interface->openVerticalBox("echo-simple");
            interface->openVerticalBox("echo  1000");
            interface->addHorizontalSlider("feedback", &fslider0, 0.000000f, 0.000000f, 100.000000f, 0.100000f);
            interface->addHorizontalSlider("millisecond", &fslider1, 0.000000f, 0.000000f, 1000.000000f, 0.100000f);
            interface->closeBox();
            interface->closeBox();
        }
        virtual void compute (int count, float** input, float** output)
        {
            float* input0 __attribute__ ((aligned(16)));
            input0 = input[0];
            float* output0 __attribute__ ((aligned(16)));
            output0 = output[0];
            float ftemp0 = (1.000000e-02f * fslider0);
            int itemp0 = int((int((fdata0 * fslider1)) - 1));
            for (int i = 0; i < count; i++) {
                R1_0 = ((1 + R1_0) & 65535);
                ftbl0[R1_0] = R0_0;
                R0_0 = (input0[i] + (ftemp0 * ftbl0[((R1_0 - itemp0) & 65535)]));
                output0[i] = R0_0;
            }
        }
};



mydsp	DSP;


/******************************************************************************
*******************************************************************************
 
							JACK AUDIO INTERFACE
 
*******************************************************************************
*******************************************************************************/



//----------------------------------------------------------------------------
// 	number of input and output channels
//----------------------------------------------------------------------------

int	gNumInChans;
int	gNumOutChans;


//----------------------------------------------------------------------------
// Jack ports
//----------------------------------------------------------------------------

jack_port_t *input_ports[256];
jack_port_t *output_ports[256];

//----------------------------------------------------------------------------
// tables of noninterleaved input and output channels for FAUST
//----------------------------------------------------------------------------

float* gInChannel[256];
float* gOutChannel[256];

//----------------------------------------------------------------------------
// Jack Callbacks
//----------------------------------------------------------------------------

int srate(jack_nframes_t nframes, void *arg)
{
    printf("the sample rate is now %u/sec\n", nframes);
    return 0;
}

void jack_shutdown(void *arg)
{
    exit(1);
}

int process (jack_nframes_t nframes, void *arg)
{
    for (int i = 0; i < gNumInChans; i++) {
        gInChannel[i] = (float *)jack_port_get_buffer(input_ports[i], nframes);
    }
    for (int i = 0; i < gNumOutChans; i++) {
        gOutChannel[i] = (float *)jack_port_get_buffer(output_ports[i], nframes);
    }
    DSP.compute(nframes, gInChannel, gOutChannel);
    return 0;
}

//-------------------------------------------------------------------------
// 									MAIN
//-------------------------------------------------------------------------

int main(int argc, char *argv[] )
{
    char jackname[256];
    char**	physicalInPorts;
    char**	physicalOutPorts;
    jack_client_t*	client;

    CMDUI* interface = new CMDUI(argc, argv);
    DSP.buildUserInterface(interface);

    snprintf(jackname, 255, "%s", basename(argv[0]));

    if ((client = jack_client_new(jackname)) == 0) {
        fprintf(stderr, "jack server not running?\n");
        return 1;
    }

    jack_set_process_callback(client, process, 0);

    jack_set_sample_rate_callback(client, srate, 0);

    jack_on_shutdown(client, jack_shutdown, 0);

    gNumInChans = DSP.getNumInputs();
    gNumOutChans = DSP.getNumOutputs();

    for (int i = 0; i < gNumInChans; i++) {
        char buf[256];
        snprintf(buf, 256, "in_%d", i);
        input_ports[i] = jack_port_register(client, buf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    }
    for (int i = 0; i < gNumOutChans; i++) {
        char buf[256];
        snprintf(buf, 256, "out_%d", i);
        output_ports[i] = jack_port_register(client, buf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    }

    DSP.init(jack_get_sample_rate(client));
    DSP.buildUserInterface(interface);

    interface->process_command();

    physicalInPorts = (char **)jack_get_ports(client, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
    physicalOutPorts = (char **)jack_get_ports(client, NULL, NULL, JackPortIsPhysical | JackPortIsOutput);

    if (jack_activate(client)) {
        fprintf(stderr, "cannot activate client");
        return 1;
    }

    if (physicalOutPorts != NULL) {
        for (int i = 0; i < gNumInChans && physicalOutPorts[i]; i++) {
            jack_connect(client, physicalOutPorts[i], jack_port_name(input_ports[i]));
        }
    }

    if (physicalInPorts != NULL) {
        for (int i = 0; i < gNumOutChans && physicalInPorts[i]; i++) {
            jack_connect(client, jack_port_name(output_ports[i]), physicalInPorts[i]);
        }
    }

    interface->run();

    jack_deactivate(client);

    for (int i = 0; i < gNumInChans; i++) {
        jack_port_unregister(client, input_ports[i]);
    }
    for (int i = 0; i < gNumOutChans; i++) {
        jack_port_unregister(client, output_ports[i]);
    }

    jack_client_close(client);

    return 0;
}
