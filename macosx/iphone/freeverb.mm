//-----------------------------------------------------
// name: "freeverb"
// version: "1.0"
// author: "Grame"
// license: "BSD"
// copyright: "(c)GRAME 2006"
//
// Code generated with Faust 0.9.9.5b2 (http://faust.grame.fr)
//-----------------------------------------------------
/* link with  */

/* link with  */
#include <math.h>
/* link with  */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include <assert.h>
#include <pthread.h>
#include <sys/wait.h>
#include <libgen.h>
#include <jack/net.h>

#include <list>
#include <vector>
#include <iostream>
#include <fstream>
#include <stack>
#include <list>
#include <map>

#include "JackAudioQueueAdapter.h"

using namespace std;

// On Intel set FZ (Flush to Zero) and DAZ (Denormals Are Zero)
// flags to avoid costly denormals
#ifdef __SSE__
    #include <xmmintrin.h>
    #ifdef __SSE2__
        #define AVOIDDENORMALS _mm_setcsr(_mm_getcsr() | 0x8040)
    #else
        #define AVOIDDENORMALS _mm_setcsr(_mm_getcsr() | 0x8000)
    #endif
#else
    #define AVOIDDENORMALS
#endif

//#define BENCHMARKMODE

struct Meta : map<const char*, const char*>
{
    void declare (const char* key, const char* value) { (*this)[key]=value; }
};


#define max(x,y) (((x)>(y)) ? (x) : (y))
#define min(x,y) (((x)<(y)) ? (x) : (y))

inline int		lsr (int x, int n)	{ return int(((unsigned int)x) >> n); }
inline int 		int2pow2 (int x)	{ int r = 0; while ((1<<r)<x) r++; return r; }


/******************************************************************************
*******************************************************************************

							       VECTOR INTRINSICS

*******************************************************************************
*******************************************************************************/


/******************************************************************************
*******************************************************************************

								USER INTERFACE

*******************************************************************************
*******************************************************************************/

class UI
{
	bool	fStopped;
public:

	UI() : fStopped(false) {}
	virtual ~UI() {}

	// -- active widgets

	virtual void addButton(const char* label, float* zone) = 0;
	virtual void addToggleButton(const char* label, float* zone) = 0;
	virtual void addCheckButton(const char* label, float* zone) = 0;
	virtual void addVerticalSlider(const char* label, float* zone, float init, float min, float max, float step) = 0;
	virtual void addHorizontalSlider(const char* label, float* zone, float init, float min, float max, float step) = 0;
	virtual void addNumEntry(const char* label, float* zone, float init, float min, float max, float step) = 0;

	// -- passive widgets

	virtual void addNumDisplay(const char* label, float* zone, int precision) = 0;
	virtual void addTextDisplay(const char* label, float* zone, char* names[], float min, float max) = 0;
	virtual void addHorizontalBargraph(const char* label, float* zone, float min, float max) = 0;
	virtual void addVerticalBargraph(const char* label, float* zone, float min, float max) = 0;

	// -- frames and labels

	virtual void openFrameBox(const char* label) = 0;
	virtual void openTabBox(const char* label) = 0;
	virtual void openHorizontalBox(const char* label) = 0;
	virtual void openVerticalBox(const char* label) = 0;
	virtual void closeBox() = 0;

	virtual void show() = 0;
	virtual void run() = 0;

	void stop()		{ fStopped = true; }
	bool stopped() 	{ return fStopped; }

    virtual void declare(float* zone, const char* key, const char* value) {}
};

struct param {
	float* fZone; float fMin; float fMax;
	param(float* z, float a, float b) : fZone(z), fMin(a), fMax(b) {}
};

class CMDUI : public UI
{
	int					fArgc;
	char**				fArgv;
	stack<string>		fPrefix;
	map<string, param>	fKeyParam;

	void addOption(const char* label, float* zone, float min, float max)
	{
		string fullname = fPrefix.top() + label;
		fKeyParam.insert(make_pair(fullname, param(zone, min, max)));
	}

	void openAnyBox(const char* label)
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

	CMDUI(int argc, char *argv[]) : UI(), fArgc(argc), fArgv(argv) { fPrefix.push("--"); }
	virtual ~CMDUI() {}

	virtual void addButton(const char* label, float* zone) 		{};
	virtual void addToggleButton(const char* label, float* zone) 	{};
	virtual void addCheckButton(const char* label, float* zone) 	{};

	virtual void addVerticalSlider(const char* label, float* zone, float init, float min, float max, float step)
	{
		addOption(label,zone,min,max);
	}

	virtual void addHorizontalSlider(const char* label, float* zone, float init, float min, float max, float step)
	{
		addOption(label,zone,min,max);
	}

	virtual void addNumEntry(const char* label, float* zone, float init, float min, float max, float step)
	{
		addOption(label,zone,min,max);
	}

	// -- passive widgets

	virtual void addNumDisplay(const char* label, float* zone, int precision) 						{}
	virtual void addTextDisplay(const char* label, float* zone, char* names[], float min, float max) 	{}
	virtual void addHorizontalBargraph(const char* label, float* zone, float min, float max) 			{}
	virtual void addVerticalBargraph(const char* label, float* zone, float min, float max) 			{}

	virtual void openFrameBox(const char* label)		{ openAnyBox(label); }
	virtual void openTabBox(const char* label)		{ openAnyBox(label); }
	virtual void openHorizontalBox(const char* label)	{ openAnyBox(label); }
	virtual void openVerticalBox(const char* label)	{ openAnyBox(label); }

	virtual void closeBox() 					{ fPrefix.pop(); }

	virtual void show() {}
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
			cout << "[ " << i->first << " " << i->second.fMin << ".." << i->second.fMax <<" ] ";
		}
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
				*(p->second.fZone) = float(strtod(fArgv[i+1], &end));
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
				*(p->second.fZone) = float(strtod(fArgv[i+1], &end));
				i++;
			}
		}
	}
};


//----------------------------------------------------------------
//  Signal processor definition
//----------------------------------------------------------------

class dsp {
 protected:
	int fSamplingFreq;
 public:
	dsp() {}
	virtual ~dsp() {}

	virtual int getNumInputs() 										= 0;
	virtual int getNumOutputs() 									= 0;
	virtual void buildUserInterface(UI* interface) 					= 0;
	virtual void init(int samplingRate) 							= 0;
 	virtual void compute(int len, float** inputs, float** outputs) 	= 0;
 	virtual void conclude() 										{}
};


//----------------------------------------------------------------------------
// 	FAUST generated code
//----------------------------------------------------------------------------


class mydsp : public dsp {
  private:
	float 	fslider0;
	float 	fRec9[2];
	float 	fslider1;
	int 	IOTA;
	float 	fVec0[2048];
	float 	fRec8[2];
	float 	fRec11[2];
	float 	fVec1[2048];
	float 	fRec10[2];
	float 	fRec13[2];
	float 	fVec2[2048];
	float 	fRec12[2];
	float 	fRec15[2];
	float 	fVec3[2048];
	float 	fRec14[2];
	float 	fRec17[2];
	float 	fVec4[2048];
	float 	fRec16[2];
	float 	fRec19[2];
	float 	fVec5[2048];
	float 	fRec18[2];
	float 	fRec21[2];
	float 	fVec6[2048];
	float 	fRec20[2];
	float 	fRec23[2];
	float 	fVec7[2048];
	float 	fRec22[2];
	float 	fVec8[1024];
	float 	fRec6[2];
	float 	fVec9[512];
	float 	fRec4[2];
	float 	fVec10[512];
	float 	fRec2[2];
	float 	fVec11[256];
	float 	fRec0[2];
	float 	fslider2;
	float 	fRec33[2];
	float 	fVec12[2048];
	float 	fRec32[2];
	float 	fRec35[2];
	float 	fVec13[2048];
	float 	fRec34[2];
	float 	fRec37[2];
	float 	fVec14[2048];
	float 	fRec36[2];
	float 	fRec39[2];
	float 	fVec15[2048];
	float 	fRec38[2];
	float 	fRec41[2];
	float 	fVec16[2048];
	float 	fRec40[2];
	float 	fRec43[2];
	float 	fVec17[2048];
	float 	fRec42[2];
	float 	fRec45[2];
	float 	fVec18[2048];
	float 	fRec44[2];
	float 	fRec47[2];
	float 	fVec19[2048];
	float 	fRec46[2];
	float 	fVec20[1024];
	float 	fRec30[2];
	float 	fVec21[512];
	float 	fRec28[2];
	float 	fVec22[512];
	float 	fRec26[2];
	float 	fVec23[256];
	float 	fRec24[2];
  public:
	static void metadata(Meta* m) 	{
		m->declare("name", "freeverb");
		m->declare("version", "1.0");
		m->declare("author", "Grame");
		m->declare("license", "BSD");
		m->declare("copyright", "(c)GRAME 2006");
	}

	virtual int getNumInputs() 	{ return 2; }
	virtual int getNumOutputs() 	{ return 2; }
	static void classInit(int samplingFreq) {
	}
	virtual void instanceInit(int samplingFreq) {
		fSamplingFreq = samplingFreq;
		fslider0 = 0.5f;
		for (int i=0; i<2; i++) fRec9[i] = 0;
		fslider1 = 0.8f;
		IOTA = 0;
		for (int i=0; i<2048; i++) fVec0[i] = 0;
		for (int i=0; i<2; i++) fRec8[i] = 0;
		for (int i=0; i<2; i++) fRec11[i] = 0;
		for (int i=0; i<2048; i++) fVec1[i] = 0;
		for (int i=0; i<2; i++) fRec10[i] = 0;
		for (int i=0; i<2; i++) fRec13[i] = 0;
		for (int i=0; i<2048; i++) fVec2[i] = 0;
		for (int i=0; i<2; i++) fRec12[i] = 0;
		for (int i=0; i<2; i++) fRec15[i] = 0;
		for (int i=0; i<2048; i++) fVec3[i] = 0;
		for (int i=0; i<2; i++) fRec14[i] = 0;
		for (int i=0; i<2; i++) fRec17[i] = 0;
		for (int i=0; i<2048; i++) fVec4[i] = 0;
		for (int i=0; i<2; i++) fRec16[i] = 0;
		for (int i=0; i<2; i++) fRec19[i] = 0;
		for (int i=0; i<2048; i++) fVec5[i] = 0;
		for (int i=0; i<2; i++) fRec18[i] = 0;
		for (int i=0; i<2; i++) fRec21[i] = 0;
		for (int i=0; i<2048; i++) fVec6[i] = 0;
		for (int i=0; i<2; i++) fRec20[i] = 0;
		for (int i=0; i<2; i++) fRec23[i] = 0;
		for (int i=0; i<2048; i++) fVec7[i] = 0;
		for (int i=0; i<2; i++) fRec22[i] = 0;
		for (int i=0; i<1024; i++) fVec8[i] = 0;
		for (int i=0; i<2; i++) fRec6[i] = 0;
		for (int i=0; i<512; i++) fVec9[i] = 0;
		for (int i=0; i<2; i++) fRec4[i] = 0;
		for (int i=0; i<512; i++) fVec10[i] = 0;
		for (int i=0; i<2; i++) fRec2[i] = 0;
		for (int i=0; i<256; i++) fVec11[i] = 0;
		for (int i=0; i<2; i++) fRec0[i] = 0;
		fslider2 = 0.8f;
		for (int i=0; i<2; i++) fRec33[i] = 0;
		for (int i=0; i<2048; i++) fVec12[i] = 0;
		for (int i=0; i<2; i++) fRec32[i] = 0;
		for (int i=0; i<2; i++) fRec35[i] = 0;
		for (int i=0; i<2048; i++) fVec13[i] = 0;
		for (int i=0; i<2; i++) fRec34[i] = 0;
		for (int i=0; i<2; i++) fRec37[i] = 0;
		for (int i=0; i<2048; i++) fVec14[i] = 0;
		for (int i=0; i<2; i++) fRec36[i] = 0;
		for (int i=0; i<2; i++) fRec39[i] = 0;
		for (int i=0; i<2048; i++) fVec15[i] = 0;
		for (int i=0; i<2; i++) fRec38[i] = 0;
		for (int i=0; i<2; i++) fRec41[i] = 0;
		for (int i=0; i<2048; i++) fVec16[i] = 0;
		for (int i=0; i<2; i++) fRec40[i] = 0;
		for (int i=0; i<2; i++) fRec43[i] = 0;
		for (int i=0; i<2048; i++) fVec17[i] = 0;
		for (int i=0; i<2; i++) fRec42[i] = 0;
		for (int i=0; i<2; i++) fRec45[i] = 0;
		for (int i=0; i<2048; i++) fVec18[i] = 0;
		for (int i=0; i<2; i++) fRec44[i] = 0;
		for (int i=0; i<2; i++) fRec47[i] = 0;
		for (int i=0; i<2048; i++) fVec19[i] = 0;
		for (int i=0; i<2; i++) fRec46[i] = 0;
		for (int i=0; i<1024; i++) fVec20[i] = 0;
		for (int i=0; i<2; i++) fRec30[i] = 0;
		for (int i=0; i<512; i++) fVec21[i] = 0;
		for (int i=0; i<2; i++) fRec28[i] = 0;
		for (int i=0; i<512; i++) fVec22[i] = 0;
		for (int i=0; i<2; i++) fRec26[i] = 0;
		for (int i=0; i<256; i++) fVec23[i] = 0;
		for (int i=0; i<2; i++) fRec24[i] = 0;
	}
	virtual void init(int samplingFreq) {
		classInit(samplingFreq);
		instanceInit(samplingFreq);
	}
	virtual void buildUserInterface(UI* interface) {
		interface->openVerticalBox("Freeverb");
		interface->addHorizontalSlider("Damp", &fslider0, 0.5f, 0.0f, 1.0f, 2.500000e-02f);
		interface->addHorizontalSlider("RoomSize", &fslider1, 0.8f, 0.0f, 1.0f, 2.500000e-02f);
		interface->addHorizontalSlider("Wet", &fslider2, 0.8f, 0.0f, 1.0f, 2.500000e-02f);
		interface->closeBox();
	}
	virtual void compute (int count, float** input, float** output) {
		float 	fSlow0 = (0.4f * fslider0);
		float 	fSlow1 = (1 - fSlow0);
		float 	fSlow2 = (0.7f + (0.28f * fslider1));
		float 	fSlow3 = fslider2;
		float 	fSlow4 = (1 - fSlow3);
		float* input0 = input[0];
		float* input1 = input[1];
		float* output0 = output[0];
		float* output1 = output[1];
		for (int i=0; i<count; i++) {
			fRec9[0] = ((fSlow1 * fRec8[1]) + (fSlow0 * fRec9[1]));
			float fTemp0 = input1[i];
			float fTemp1 = input0[i];
			float fTemp2 = (1.500000e-02f * (fTemp1 + fTemp0));
			fVec0[IOTA&2047] = (fTemp2 + (fSlow2 * fRec9[0]));
			fRec8[0] = fVec0[(IOTA-1617)&2047];
			fRec11[0] = ((fSlow1 * fRec10[1]) + (fSlow0 * fRec11[1]));
			fVec1[IOTA&2047] = (fTemp2 + (fSlow2 * fRec11[0]));
			fRec10[0] = fVec1[(IOTA-1557)&2047];
			fRec13[0] = ((fSlow1 * fRec12[1]) + (fSlow0 * fRec13[1]));
			fVec2[IOTA&2047] = (fTemp2 + (fSlow2 * fRec13[0]));
			fRec12[0] = fVec2[(IOTA-1491)&2047];
			fRec15[0] = ((fSlow1 * fRec14[1]) + (fSlow0 * fRec15[1]));
			fVec3[IOTA&2047] = (fTemp2 + (fSlow2 * fRec15[0]));
			fRec14[0] = fVec3[(IOTA-1422)&2047];
			fRec17[0] = ((fSlow1 * fRec16[1]) + (fSlow0 * fRec17[1]));
			fVec4[IOTA&2047] = (fTemp2 + (fSlow2 * fRec17[0]));
			fRec16[0] = fVec4[(IOTA-1356)&2047];
			fRec19[0] = ((fSlow1 * fRec18[1]) + (fSlow0 * fRec19[1]));
			fVec5[IOTA&2047] = (fTemp2 + (fSlow2 * fRec19[0]));
			fRec18[0] = fVec5[(IOTA-1277)&2047];
			fRec21[0] = ((fSlow1 * fRec20[1]) + (fSlow0 * fRec21[1]));
			fVec6[IOTA&2047] = (fTemp2 + (fSlow2 * fRec21[0]));
			fRec20[0] = fVec6[(IOTA-1188)&2047];
			fRec23[0] = ((fSlow1 * fRec22[1]) + (fSlow0 * fRec23[1]));
			fVec7[IOTA&2047] = (fTemp2 + (fSlow2 * fRec23[0]));
			fRec22[0] = fVec7[(IOTA-1116)&2047];
			float fTemp3 = (((((((fRec22[0] + fRec20[0]) + fRec18[0]) + fRec16[0]) + fRec14[0]) + fRec12[0]) + fRec10[0]) + fRec8[0]);
			fVec8[IOTA&1023] = (fTemp3 + (0.5f * fRec6[1]));
			fRec6[0] = fVec8[(IOTA-556)&1023];
			float 	fRec7 = (0 - (fTemp3 - fRec6[1]));
			fVec9[IOTA&511] = (fRec7 + (0.5f * fRec4[1]));
			fRec4[0] = fVec9[(IOTA-441)&511];
			float 	fRec5 = (fRec4[1] - fRec7);
			fVec10[IOTA&511] = (fRec5 + (0.5f * fRec2[1]));
			fRec2[0] = fVec10[(IOTA-341)&511];
			float 	fRec3 = (fRec2[1] - fRec5);
			fVec11[IOTA&255] = (fRec3 + (0.5f * fRec0[1]));
			fRec0[0] = fVec11[(IOTA-225)&255];
			float 	fRec1 = (fRec0[1] - fRec3);
			output0[i] = ((fSlow4 * fTemp1) + (fSlow3 * fRec1));
			fRec33[0] = ((fSlow1 * fRec32[1]) + (fSlow0 * fRec33[1]));
			fVec12[IOTA&2047] = (fTemp2 + (fSlow2 * fRec33[0]));
			fRec32[0] = fVec12[(IOTA-1640)&2047];
			fRec35[0] = ((fSlow1 * fRec34[1]) + (fSlow0 * fRec35[1]));
			fVec13[IOTA&2047] = (fTemp2 + (fSlow2 * fRec35[0]));
			fRec34[0] = fVec13[(IOTA-1580)&2047];
			fRec37[0] = ((fSlow1 * fRec36[1]) + (fSlow0 * fRec37[1]));
			fVec14[IOTA&2047] = (fTemp2 + (fSlow2 * fRec37[0]));
			fRec36[0] = fVec14[(IOTA-1514)&2047];
			fRec39[0] = ((fSlow1 * fRec38[1]) + (fSlow0 * fRec39[1]));
			fVec15[IOTA&2047] = (fTemp2 + (fSlow2 * fRec39[0]));
			fRec38[0] = fVec15[(IOTA-1445)&2047];
			fRec41[0] = ((fSlow1 * fRec40[1]) + (fSlow0 * fRec41[1]));
			fVec16[IOTA&2047] = (fTemp2 + (fSlow2 * fRec41[0]));
			fRec40[0] = fVec16[(IOTA-1379)&2047];
			fRec43[0] = ((fSlow1 * fRec42[1]) + (fSlow0 * fRec43[1]));
			fVec17[IOTA&2047] = (fTemp2 + (fSlow2 * fRec43[0]));
			fRec42[0] = fVec17[(IOTA-1300)&2047];
			fRec45[0] = ((fSlow1 * fRec44[1]) + (fSlow0 * fRec45[1]));
			fVec18[IOTA&2047] = (fTemp2 + (fSlow2 * fRec45[0]));
			fRec44[0] = fVec18[(IOTA-1211)&2047];
			fRec47[0] = ((fSlow1 * fRec46[1]) + (fSlow0 * fRec47[1]));
			fVec19[IOTA&2047] = (fTemp2 + (fSlow2 * fRec47[0]));
			fRec46[0] = fVec19[(IOTA-1139)&2047];
			float fTemp4 = (((((((fRec46[0] + fRec44[0]) + fRec42[0]) + fRec40[0]) + fRec38[0]) + fRec36[0]) + fRec34[0]) + fRec32[0]);
			fVec20[IOTA&1023] = (fTemp4 + (0.5f * fRec30[1]));
			fRec30[0] = fVec20[(IOTA-579)&1023];
			float 	fRec31 = (0 - (fTemp4 - fRec30[1]));
			fVec21[IOTA&511] = (fRec31 + (0.5f * fRec28[1]));
			fRec28[0] = fVec21[(IOTA-464)&511];
			float 	fRec29 = (fRec28[1] - fRec31);
			fVec22[IOTA&511] = (fRec29 + (0.5f * fRec26[1]));
			fRec26[0] = fVec22[(IOTA-364)&511];
			float 	fRec27 = (fRec26[1] - fRec29);
			fVec23[IOTA&255] = (fRec27 + (0.5f * fRec24[1]));
			fRec24[0] = fVec23[(IOTA-248)&255];
			float 	fRec25 = (fRec24[1] - fRec27);
			output1[i] = ((fSlow4 * fTemp0) + (fSlow3 * fRec25));
			// post processing
			fRec24[1] = fRec24[0];
			fRec26[1] = fRec26[0];
			fRec28[1] = fRec28[0];
			fRec30[1] = fRec30[0];
			fRec46[1] = fRec46[0];
			fRec47[1] = fRec47[0];
			fRec44[1] = fRec44[0];
			fRec45[1] = fRec45[0];
			fRec42[1] = fRec42[0];
			fRec43[1] = fRec43[0];
			fRec40[1] = fRec40[0];
			fRec41[1] = fRec41[0];
			fRec38[1] = fRec38[0];
			fRec39[1] = fRec39[0];
			fRec36[1] = fRec36[0];
			fRec37[1] = fRec37[0];
			fRec34[1] = fRec34[0];
			fRec35[1] = fRec35[0];
			fRec32[1] = fRec32[0];
			fRec33[1] = fRec33[0];
			fRec0[1] = fRec0[0];
			fRec2[1] = fRec2[0];
			fRec4[1] = fRec4[0];
			fRec6[1] = fRec6[0];
			fRec22[1] = fRec22[0];
			fRec23[1] = fRec23[0];
			fRec20[1] = fRec20[0];
			fRec21[1] = fRec21[0];
			fRec18[1] = fRec18[0];
			fRec19[1] = fRec19[0];
			fRec16[1] = fRec16[0];
			fRec17[1] = fRec17[0];
			fRec14[1] = fRec14[0];
			fRec15[1] = fRec15[0];
			fRec12[1] = fRec12[0];
			fRec13[1] = fRec13[0];
			fRec10[1] = fRec10[0];
			fRec11[1] = fRec11[0];
			fRec8[1] = fRec8[0];
			IOTA = IOTA+1;
			fRec9[1] = fRec9[0];
		}
	}
};

mydsp	DSP;

/******************************************************************************
*******************************************************************************

							NETJACK AUDIO INTERFACE

*******************************************************************************
*******************************************************************************/

//----------------------------------------------------------------------------
// 	number of input and output channels
//----------------------------------------------------------------------------

int		gNumInChans;
int		gNumOutChans;

//----------------------------------------------------------------------------
// Jack Callbacks
//----------------------------------------------------------------------------

static void net_shutdown(void *)
{
	exit(1);
}

#ifdef BENCHMARKMODE
// mesuring jack performances
static __inline__ unsigned long long int rdtsc(void)
{
  unsigned long long int x;
     __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
     return x;
}

#define KSKIP 10
#define KMESURE 1024
int mesure = 0;
unsigned long long int starts[KMESURE];
unsigned long long int stops [KMESURE];

#define STARTMESURE starts[mesure%KMESURE] = rdtsc();
#define STOPMESURE stops[mesure%KMESURE] = rdtsc(); mesure = mesure+1;

void printstats()
{
    unsigned long long int low, hi, tot;
    low = hi = tot = (stops[KSKIP] - starts[KSKIP]);

    if (mesure < KMESURE) {

        for (int i = KSKIP+1; i<mesure; i++) {
            unsigned long long int m = stops[i] - starts[i];
            if (m<low) low = m;
            if (m>hi) hi = m;
            tot += m;
        }
        cout << low << ' ' << tot/(mesure-KSKIP) << ' ' << hi << endl;

    } else {

        for (int i = KSKIP+1; i<KMESURE; i++) {
            unsigned long long int m = stops[i] - starts[i];
            if (m<low) low = m;
            if (m>hi) hi = m;
            tot += m;
        }
        cout << low << ' ' << tot/(KMESURE-KSKIP) << ' ' << hi << endl;

    }
}

#else

#define STARTMESURE
#define STOPMESURE

#endif

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
    AVOIDDENORMALS;
    STARTMESURE
    DSP.compute(buffer_size, audio_input_buffer, audio_output_buffer);
    STOPMESURE
	return 0;
}

/******************************************************************************
*******************************************************************************

								MAIN PLAY THREAD

*******************************************************************************
*******************************************************************************/

//-------------------------------------------------------------------------
// 									MAIN
//-------------------------------------------------------------------------


#define TEST_MASTER "194.5.49.5"

int main(int argc, char *argv[]) {

    UI* interface = new CMDUI(argc, argv);
    jack_net_slave_t* net;
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    //Jack::JackAudioQueueAdapter audio(2, 2, 1024, 44100, NULL);

    gNumInChans = DSP.getNumInputs();
	gNumOutChans = DSP.getNumOutputs();

    jack_slave_t request = { gNumInChans, gNumOutChans, 0, 0, DEFAULT_MTU, -1, 2 };
    jack_master_t result;

    printf("Network\n");

    //if (audio.Open() < 0) {
    //    fprintf(stderr, "Cannot open audio\n");
    //    return 1;
    //}

     //audio.Start();

    // Hang around forever...
	 //while(1) CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.25, false);

    if ((net = jack_net_slave_open(TEST_MASTER, DEFAULT_PORT, "iPhone", &request, &result)) == 0) {
	    fprintf(stderr, "jack remote server not running ?\n");
	    return 1;
	}

    jack_set_net_slave_process_callback(net, net_process, NULL);

    // We want to restart (that is "wait for available master" again)
    //jack_set_net_shutdown_callback(net, net_shutdown, 0);

    DSP.init(result.sample_rate);
	DSP.buildUserInterface(interface);

    if (jack_net_slave_activate(net) != 0) {
	    fprintf(stderr, "cannot activate net");
	    return 1;
	}

    int retVal = UIApplicationMain(argc, argv, nil, nil);
    [pool release];

    // Wait for application end
    jack_net_slave_deactivate(net);
    jack_net_slave_close(net);

    //if (audio.Close() < 0) {
    //    fprintf(stderr, "Cannot close audio\n");
    //}

    return retVal;
}
