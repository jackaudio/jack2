//-----------------------------------------------------
// name: "freeverb"
// version: "1.0"
// author: "Grame"
// license: "BSD"
// copyright: "(c)GRAME 2006"
//
// Code generated with Faust 0.9.10 (http://faust.grame.fr)
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

#include <list>
#include <vector>
#include <iostream>
#include <fstream>
#include <stack>
#include <list>
#include <map>
#include <libgen.h>

#include <AudioToolbox/AudioConverter.h>
#include <AudioToolbox/AudioServices.h>
#include <AudioUnit/AudioUnit.h>

#include "HardwareClock.h"

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
    void declare (const char* key, const char* value) { (*this)[key] = value; }
};

//inline void *aligned_calloc(size_t nmemb, size_t size) { return (void*)((unsigned)(calloc((nmemb*size)+15,sizeof(char)))+15 & 0xfffffff0); }

// g++ -O3 -lm -ljack `gtk-config --cflags --libs` ex2.cpp
 
	
#define max(x,y) (((x)>(y)) ? (x) : (y))
#define min(x,y) (((x)<(y)) ? (x) : (y))

inline int	lsr(int x, int n)	{ return int(((unsigned int)x) >> n); }
inline int 	int2pow2(int x)     { int r=0; while ((1<<r)<x) r++; return r; }

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
        bool fStopped;
        
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
	
	void addOption(const char* label, float* zone, float init, float min, float max)
	{
		string fullname = fPrefix.top() + label;
		fKeyParam.insert(make_pair(fullname, param(zone, min, max)));
        *zone = init;
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
		addOption(label,zone,init, min,max);
	}
		
	virtual void addHorizontalSlider(const char* label, float* zone, float init, float min, float max, float step)
	{
		addOption(label,zone,init, min,max);
	}

	virtual void addNumEntry(const char* label, float* zone, float init, float min, float max, float step)
	{
		addOption(label,zone,init, min,max);
	}
		
	// -- passive widgets
	
	virtual void addNumDisplay(const char* label, float* zone, int precision)                           {}
	virtual void addTextDisplay(const char* label, float* zone, char* names[], float min, float max) 	{}
	virtual void addHorizontalBargraph(const char* label, float* zone, float min, float max) 			{}
	virtual void addVerticalBargraph(const char* label, float* zone, float min, float max)              {}

	virtual void openFrameBox(const char* label)		{ openAnyBox(label); }
	virtual void openTabBox(const char* label)          { openAnyBox(label); }
	virtual void openHorizontalBox(const char* label)	{ openAnyBox(label); }
	virtual void openVerticalBox(const char* label)     { openAnyBox(label); }
	
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

/******************************************************************************
*******************************************************************************

			    FAUST DSP

*******************************************************************************
*******************************************************************************/


//----------------------------------------------------------------
//  abstract definition of a signal processor
//----------------------------------------------------------------
			
class dsp {
 protected:
	int fSamplingFreq;
 public:
	dsp() {}
	virtual ~dsp() {}

	virtual int getNumInputs() 						= 0;
	virtual int getNumOutputs() 					= 0;
	virtual void buildUserInterface(UI* interface) 	= 0;
	virtual void init(int samplingRate) 			= 0;
 	virtual void compute(int len, float** inputs, float** outputs) 	= 0;
};
		
//----------------------------------------------------------------------------
//  FAUST generated signal processor
//----------------------------------------------------------------------------
		
#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif  

typedef long double quad;

class mydsp : public dsp{
  private:
	FAUSTFLOAT 	fslider0;
	float 	fRec9_perm[4];
	FAUSTFLOAT 	fslider1;
	float 	fRec19_perm[4];
	float 	fYec0[4096];
	int 	fYec0_idx;
	int 	fYec0_idx_save;
	float 	fRec18_perm[4];
	float 	fRec21_perm[4];
	float 	fYec1[4096];
	int 	fYec1_idx;
	int 	fYec1_idx_save;
	float 	fRec20_perm[4];
	float 	fRec23_perm[4];
	float 	fYec2[4096];
	int 	fYec2_idx;
	int 	fYec2_idx_save;
	float 	fRec22_perm[4];
	float 	fRec25_perm[4];
	float 	fYec3[4096];
	int 	fYec3_idx;
	int 	fYec3_idx_save;
	float 	fRec24_perm[4];
	float 	fRec27_perm[4];
	float 	fYec4[4096];
	int 	fYec4_idx;
	int 	fYec4_idx_save;
	float 	fRec26_perm[4];
	float 	fRec29_perm[4];
	float 	fYec5[4096];
	int 	fYec5_idx;
	int 	fYec5_idx_save;
	float 	fRec28_perm[4];
	float 	fRec31_perm[4];
	float 	fYec6[4096];
	int 	fYec6_idx;
	int 	fYec6_idx_save;
	float 	fRec30_perm[4];
	float 	fRec33_perm[4];
	float 	fYec7[4096];
	int 	fYec7_idx;
	int 	fYec7_idx_save;
	float 	fRec32_perm[4];
	float 	fYec8[2048];
	int 	fYec8_idx;
	int 	fYec8_idx_save;
	float 	fRec16_perm[4];
	float 	fYec9[2048];
	int 	fYec9_idx;
	int 	fYec9_idx_save;
	float 	fRec14_perm[4];
	float 	fYec10[2048];
	int 	fYec10_idx;
	int 	fYec10_idx_save;
	float 	fRec12_perm[4];
	float 	fYec11[2048];
	int 	fYec11_idx;
	int 	fYec11_idx_save;
	float 	fRec10_perm[4];
	FAUSTFLOAT 	fslider2;
	float 	fRec43_perm[4];
	float 	fYec12[4096];
	int 	fYec12_idx;
	int 	fYec12_idx_save;
	float 	fRec42_perm[4];
	float 	fRec45_perm[4];
	float 	fYec13[4096];
	int 	fYec13_idx;
	int 	fYec13_idx_save;
	float 	fRec44_perm[4];
	float 	fRec47_perm[4];
	float 	fYec14[4096];
	int 	fYec14_idx;
	int 	fYec14_idx_save;
	float 	fRec46_perm[4];
	float 	fRec49_perm[4];
	float 	fYec15[4096];
	int 	fYec15_idx;
	int 	fYec15_idx_save;
	float 	fRec48_perm[4];
	float 	fRec51_perm[4];
	float 	fYec16[4096];
	int 	fYec16_idx;
	int 	fYec16_idx_save;
	float 	fRec50_perm[4];
	float 	fRec53_perm[4];
	float 	fYec17[4096];
	int 	fYec17_idx;
	int 	fYec17_idx_save;
	float 	fRec52_perm[4];
	float 	fRec55_perm[4];
	float 	fYec18[4096];
	int 	fYec18_idx;
	int 	fYec18_idx_save;
	float 	fRec54_perm[4];
	float 	fRec57_perm[4];
	float 	fYec19[4096];
	int 	fYec19_idx;
	int 	fYec19_idx_save;
	float 	fRec56_perm[4];
	float 	fYec20[2048];
	int 	fYec20_idx;
	int 	fYec20_idx_save;
	float 	fRec40_perm[4];
	float 	fYec21[2048];
	int 	fYec21_idx;
	int 	fYec21_idx_save;
	float 	fRec38_perm[4];
	float 	fYec22[2048];
	int 	fYec22_idx;
	int 	fYec22_idx_save;
	float 	fRec36_perm[4];
	float 	fYec23[2048];
	int 	fYec23_idx;
	int 	fYec23_idx_save;
	float 	fRec34_perm[4];
	float 	fYec24[4096];
	int 	fYec24_idx;
	int 	fYec24_idx_save;
	float 	fRec8_perm[4];
	float 	fRec59_perm[4];
	float 	fYec25[4096];
	int 	fYec25_idx;
	int 	fYec25_idx_save;
	float 	fRec58_perm[4];
	float 	fRec61_perm[4];
	float 	fYec26[4096];
	int 	fYec26_idx;
	int 	fYec26_idx_save;
	float 	fRec60_perm[4];
	float 	fRec63_perm[4];
	float 	fYec27[4096];
	int 	fYec27_idx;
	int 	fYec27_idx_save;
	float 	fRec62_perm[4];
	float 	fRec65_perm[4];
	float 	fYec28[4096];
	int 	fYec28_idx;
	int 	fYec28_idx_save;
	float 	fRec64_perm[4];
	float 	fRec67_perm[4];
	float 	fYec29[4096];
	int 	fYec29_idx;
	int 	fYec29_idx_save;
	float 	fRec66_perm[4];
	float 	fRec69_perm[4];
	float 	fYec30[4096];
	int 	fYec30_idx;
	int 	fYec30_idx_save;
	float 	fRec68_perm[4];
	float 	fRec71_perm[4];
	float 	fYec31[4096];
	int 	fYec31_idx;
	int 	fYec31_idx_save;
	float 	fRec70_perm[4];
	float 	fYec32[2048];
	int 	fYec32_idx;
	int 	fYec32_idx_save;
	float 	fRec6_perm[4];
	float 	fYec33[2048];
	int 	fYec33_idx;
	int 	fYec33_idx_save;
	float 	fRec4_perm[4];
	float 	fYec34[2048];
	int 	fYec34_idx;
	int 	fYec34_idx_save;
	float 	fRec2_perm[4];
	float 	fYec35[2048];
	int 	fYec35_idx;
	int 	fYec35_idx_save;
	float 	fRec0_perm[4];
	float 	fRec81_perm[4];
	float 	fYec36[4096];
	int 	fYec36_idx;
	int 	fYec36_idx_save;
	float 	fRec80_perm[4];
	float 	fRec83_perm[4];
	float 	fYec37[4096];
	int 	fYec37_idx;
	int 	fYec37_idx_save;
	float 	fRec82_perm[4];
	float 	fRec85_perm[4];
	float 	fYec38[4096];
	int 	fYec38_idx;
	int 	fYec38_idx_save;
	float 	fRec84_perm[4];
	float 	fRec87_perm[4];
	float 	fYec39[4096];
	int 	fYec39_idx;
	int 	fYec39_idx_save;
	float 	fRec86_perm[4];
	float 	fRec89_perm[4];
	float 	fYec40[4096];
	int 	fYec40_idx;
	int 	fYec40_idx_save;
	float 	fRec88_perm[4];
	float 	fRec91_perm[4];
	float 	fYec41[4096];
	int 	fYec41_idx;
	int 	fYec41_idx_save;
	float 	fRec90_perm[4];
	float 	fRec93_perm[4];
	float 	fYec42[4096];
	int 	fYec42_idx;
	int 	fYec42_idx_save;
	float 	fRec92_perm[4];
	float 	fRec95_perm[4];
	float 	fYec43[4096];
	int 	fYec43_idx;
	int 	fYec43_idx_save;
	float 	fRec94_perm[4];
	float 	fYec44[2048];
	int 	fYec44_idx;
	int 	fYec44_idx_save;
	float 	fRec78_perm[4];
	float 	fYec45[2048];
	int 	fYec45_idx;
	int 	fYec45_idx_save;
	float 	fRec76_perm[4];
	float 	fYec46[2048];
	int 	fYec46_idx;
	int 	fYec46_idx_save;
	float 	fRec74_perm[4];
	float 	fYec47[2048];
	int 	fYec47_idx;
	int 	fYec47_idx_save;
	float 	fRec72_perm[4];
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
		for (int i=0; i<4; i++) fRec9_perm[i]=0;
		fslider1 = 0.5f;
		for (int i=0; i<4; i++) fRec19_perm[i]=0;
		for (int i=0; i<4096; i++) fYec0[i]=0;
		fYec0_idx = 0;
		fYec0_idx_save = 0;
		for (int i=0; i<4; i++) fRec18_perm[i]=0;
		for (int i=0; i<4; i++) fRec21_perm[i]=0;
		for (int i=0; i<4096; i++) fYec1[i]=0;
		fYec1_idx = 0;
		fYec1_idx_save = 0;
		for (int i=0; i<4; i++) fRec20_perm[i]=0;
		for (int i=0; i<4; i++) fRec23_perm[i]=0;
		for (int i=0; i<4096; i++) fYec2[i]=0;
		fYec2_idx = 0;
		fYec2_idx_save = 0;
		for (int i=0; i<4; i++) fRec22_perm[i]=0;
		for (int i=0; i<4; i++) fRec25_perm[i]=0;
		for (int i=0; i<4096; i++) fYec3[i]=0;
		fYec3_idx = 0;
		fYec3_idx_save = 0;
		for (int i=0; i<4; i++) fRec24_perm[i]=0;
		for (int i=0; i<4; i++) fRec27_perm[i]=0;
		for (int i=0; i<4096; i++) fYec4[i]=0;
		fYec4_idx = 0;
		fYec4_idx_save = 0;
		for (int i=0; i<4; i++) fRec26_perm[i]=0;
		for (int i=0; i<4; i++) fRec29_perm[i]=0;
		for (int i=0; i<4096; i++) fYec5[i]=0;
		fYec5_idx = 0;
		fYec5_idx_save = 0;
		for (int i=0; i<4; i++) fRec28_perm[i]=0;
		for (int i=0; i<4; i++) fRec31_perm[i]=0;
		for (int i=0; i<4096; i++) fYec6[i]=0;
		fYec6_idx = 0;
		fYec6_idx_save = 0;
		for (int i=0; i<4; i++) fRec30_perm[i]=0;
		for (int i=0; i<4; i++) fRec33_perm[i]=0;
		for (int i=0; i<4096; i++) fYec7[i]=0;
		fYec7_idx = 0;
		fYec7_idx_save = 0;
		for (int i=0; i<4; i++) fRec32_perm[i]=0;
		for (int i=0; i<2048; i++) fYec8[i]=0;
		fYec8_idx = 0;
		fYec8_idx_save = 0;
		for (int i=0; i<4; i++) fRec16_perm[i]=0;
		for (int i=0; i<2048; i++) fYec9[i]=0;
		fYec9_idx = 0;
		fYec9_idx_save = 0;
		for (int i=0; i<4; i++) fRec14_perm[i]=0;
		for (int i=0; i<2048; i++) fYec10[i]=0;
		fYec10_idx = 0;
		fYec10_idx_save = 0;
		for (int i=0; i<4; i++) fRec12_perm[i]=0;
		for (int i=0; i<2048; i++) fYec11[i]=0;
		fYec11_idx = 0;
		fYec11_idx_save = 0;
		for (int i=0; i<4; i++) fRec10_perm[i]=0;
		fslider2 = 0.3333f;
		for (int i=0; i<4; i++) fRec43_perm[i]=0;
		for (int i=0; i<4096; i++) fYec12[i]=0;
		fYec12_idx = 0;
		fYec12_idx_save = 0;
		for (int i=0; i<4; i++) fRec42_perm[i]=0;
		for (int i=0; i<4; i++) fRec45_perm[i]=0;
		for (int i=0; i<4096; i++) fYec13[i]=0;
		fYec13_idx = 0;
		fYec13_idx_save = 0;
		for (int i=0; i<4; i++) fRec44_perm[i]=0;
		for (int i=0; i<4; i++) fRec47_perm[i]=0;
		for (int i=0; i<4096; i++) fYec14[i]=0;
		fYec14_idx = 0;
		fYec14_idx_save = 0;
		for (int i=0; i<4; i++) fRec46_perm[i]=0;
		for (int i=0; i<4; i++) fRec49_perm[i]=0;
		for (int i=0; i<4096; i++) fYec15[i]=0;
		fYec15_idx = 0;
		fYec15_idx_save = 0;
		for (int i=0; i<4; i++) fRec48_perm[i]=0;
		for (int i=0; i<4; i++) fRec51_perm[i]=0;
		for (int i=0; i<4096; i++) fYec16[i]=0;
		fYec16_idx = 0;
		fYec16_idx_save = 0;
		for (int i=0; i<4; i++) fRec50_perm[i]=0;
		for (int i=0; i<4; i++) fRec53_perm[i]=0;
		for (int i=0; i<4096; i++) fYec17[i]=0;
		fYec17_idx = 0;
		fYec17_idx_save = 0;
		for (int i=0; i<4; i++) fRec52_perm[i]=0;
		for (int i=0; i<4; i++) fRec55_perm[i]=0;
		for (int i=0; i<4096; i++) fYec18[i]=0;
		fYec18_idx = 0;
		fYec18_idx_save = 0;
		for (int i=0; i<4; i++) fRec54_perm[i]=0;
		for (int i=0; i<4; i++) fRec57_perm[i]=0;
		for (int i=0; i<4096; i++) fYec19[i]=0;
		fYec19_idx = 0;
		fYec19_idx_save = 0;
		for (int i=0; i<4; i++) fRec56_perm[i]=0;
		for (int i=0; i<2048; i++) fYec20[i]=0;
		fYec20_idx = 0;
		fYec20_idx_save = 0;
		for (int i=0; i<4; i++) fRec40_perm[i]=0;
		for (int i=0; i<2048; i++) fYec21[i]=0;
		fYec21_idx = 0;
		fYec21_idx_save = 0;
		for (int i=0; i<4; i++) fRec38_perm[i]=0;
		for (int i=0; i<2048; i++) fYec22[i]=0;
		fYec22_idx = 0;
		fYec22_idx_save = 0;
		for (int i=0; i<4; i++) fRec36_perm[i]=0;
		for (int i=0; i<2048; i++) fYec23[i]=0;
		fYec23_idx = 0;
		fYec23_idx_save = 0;
		for (int i=0; i<4; i++) fRec34_perm[i]=0;
		for (int i=0; i<4096; i++) fYec24[i]=0;
		fYec24_idx = 0;
		fYec24_idx_save = 0;
		for (int i=0; i<4; i++) fRec8_perm[i]=0;
		for (int i=0; i<4; i++) fRec59_perm[i]=0;
		for (int i=0; i<4096; i++) fYec25[i]=0;
		fYec25_idx = 0;
		fYec25_idx_save = 0;
		for (int i=0; i<4; i++) fRec58_perm[i]=0;
		for (int i=0; i<4; i++) fRec61_perm[i]=0;
		for (int i=0; i<4096; i++) fYec26[i]=0;
		fYec26_idx = 0;
		fYec26_idx_save = 0;
		for (int i=0; i<4; i++) fRec60_perm[i]=0;
		for (int i=0; i<4; i++) fRec63_perm[i]=0;
		for (int i=0; i<4096; i++) fYec27[i]=0;
		fYec27_idx = 0;
		fYec27_idx_save = 0;
		for (int i=0; i<4; i++) fRec62_perm[i]=0;
		for (int i=0; i<4; i++) fRec65_perm[i]=0;
		for (int i=0; i<4096; i++) fYec28[i]=0;
		fYec28_idx = 0;
		fYec28_idx_save = 0;
		for (int i=0; i<4; i++) fRec64_perm[i]=0;
		for (int i=0; i<4; i++) fRec67_perm[i]=0;
		for (int i=0; i<4096; i++) fYec29[i]=0;
		fYec29_idx = 0;
		fYec29_idx_save = 0;
		for (int i=0; i<4; i++) fRec66_perm[i]=0;
		for (int i=0; i<4; i++) fRec69_perm[i]=0;
		for (int i=0; i<4096; i++) fYec30[i]=0;
		fYec30_idx = 0;
		fYec30_idx_save = 0;
		for (int i=0; i<4; i++) fRec68_perm[i]=0;
		for (int i=0; i<4; i++) fRec71_perm[i]=0;
		for (int i=0; i<4096; i++) fYec31[i]=0;
		fYec31_idx = 0;
		fYec31_idx_save = 0;
		for (int i=0; i<4; i++) fRec70_perm[i]=0;
		for (int i=0; i<2048; i++) fYec32[i]=0;
		fYec32_idx = 0;
		fYec32_idx_save = 0;
		for (int i=0; i<4; i++) fRec6_perm[i]=0;
		for (int i=0; i<2048; i++) fYec33[i]=0;
		fYec33_idx = 0;
		fYec33_idx_save = 0;
		for (int i=0; i<4; i++) fRec4_perm[i]=0;
		for (int i=0; i<2048; i++) fYec34[i]=0;
		fYec34_idx = 0;
		fYec34_idx_save = 0;
		for (int i=0; i<4; i++) fRec2_perm[i]=0;
		for (int i=0; i<2048; i++) fYec35[i]=0;
		fYec35_idx = 0;
		fYec35_idx_save = 0;
		for (int i=0; i<4; i++) fRec0_perm[i]=0;
		for (int i=0; i<4; i++) fRec81_perm[i]=0;
		for (int i=0; i<4096; i++) fYec36[i]=0;
		fYec36_idx = 0;
		fYec36_idx_save = 0;
		for (int i=0; i<4; i++) fRec80_perm[i]=0;
		for (int i=0; i<4; i++) fRec83_perm[i]=0;
		for (int i=0; i<4096; i++) fYec37[i]=0;
		fYec37_idx = 0;
		fYec37_idx_save = 0;
		for (int i=0; i<4; i++) fRec82_perm[i]=0;
		for (int i=0; i<4; i++) fRec85_perm[i]=0;
		for (int i=0; i<4096; i++) fYec38[i]=0;
		fYec38_idx = 0;
		fYec38_idx_save = 0;
		for (int i=0; i<4; i++) fRec84_perm[i]=0;
		for (int i=0; i<4; i++) fRec87_perm[i]=0;
		for (int i=0; i<4096; i++) fYec39[i]=0;
		fYec39_idx = 0;
		fYec39_idx_save = 0;
		for (int i=0; i<4; i++) fRec86_perm[i]=0;
		for (int i=0; i<4; i++) fRec89_perm[i]=0;
		for (int i=0; i<4096; i++) fYec40[i]=0;
		fYec40_idx = 0;
		fYec40_idx_save = 0;
		for (int i=0; i<4; i++) fRec88_perm[i]=0;
		for (int i=0; i<4; i++) fRec91_perm[i]=0;
		for (int i=0; i<4096; i++) fYec41[i]=0;
		fYec41_idx = 0;
		fYec41_idx_save = 0;
		for (int i=0; i<4; i++) fRec90_perm[i]=0;
		for (int i=0; i<4; i++) fRec93_perm[i]=0;
		for (int i=0; i<4096; i++) fYec42[i]=0;
		fYec42_idx = 0;
		fYec42_idx_save = 0;
		for (int i=0; i<4; i++) fRec92_perm[i]=0;
		for (int i=0; i<4; i++) fRec95_perm[i]=0;
		for (int i=0; i<4096; i++) fYec43[i]=0;
		fYec43_idx = 0;
		fYec43_idx_save = 0;
		for (int i=0; i<4; i++) fRec94_perm[i]=0;
		for (int i=0; i<2048; i++) fYec44[i]=0;
		fYec44_idx = 0;
		fYec44_idx_save = 0;
		for (int i=0; i<4; i++) fRec78_perm[i]=0;
		for (int i=0; i<2048; i++) fYec45[i]=0;
		fYec45_idx = 0;
		fYec45_idx_save = 0;
		for (int i=0; i<4; i++) fRec76_perm[i]=0;
		for (int i=0; i<2048; i++) fYec46[i]=0;
		fYec46_idx = 0;
		fYec46_idx_save = 0;
		for (int i=0; i<4; i++) fRec74_perm[i]=0;
		for (int i=0; i<2048; i++) fYec47[i]=0;
		fYec47_idx = 0;
		fYec47_idx_save = 0;
		for (int i=0; i<4; i++) fRec72_perm[i]=0;
	}
	virtual void init(int samplingFreq) {
		classInit(samplingFreq);
		instanceInit(samplingFreq);
	}
	virtual void buildUserInterface(UI* interface) {
		interface->openVerticalBox("Freeverb");
		interface->addHorizontalSlider("Damp", &fslider0, 0.5f, 0.0f, 1.0f, 2.500000e-02f);
		interface->addHorizontalSlider("RoomSize", &fslider1, 0.5f, 0.0f, 1.0f, 2.500000e-02f);
		interface->addHorizontalSlider("Wet", &fslider2, 0.3333f, 0.0f, 1.0f, 2.500000e-02f);
		interface->closeBox();
	}
	virtual void compute (int fullcount, FAUSTFLOAT** input, FAUSTFLOAT** output) {
		float 	fRec9_tmp[1024+4];
		float 	fRec19_tmp[1024+4];
		float 	fZec0[1024];
		float 	fRec18_tmp[1024+4];
		float 	fRec21_tmp[1024+4];
		float 	fRec20_tmp[1024+4];
		float 	fRec23_tmp[1024+4];
		float 	fRec22_tmp[1024+4];
		float 	fRec25_tmp[1024+4];
		float 	fRec24_tmp[1024+4];
		float 	fRec27_tmp[1024+4];
		float 	fRec26_tmp[1024+4];
		float 	fRec29_tmp[1024+4];
		float 	fRec28_tmp[1024+4];
		float 	fRec31_tmp[1024+4];
		float 	fRec30_tmp[1024+4];
		float 	fRec33_tmp[1024+4];
		float 	fRec32_tmp[1024+4];
		float 	fZec1[1024];
		float 	fRec16_tmp[1024+4];
		float 	fRec17[1024];
		float 	fRec14_tmp[1024+4];
		float 	fRec15[1024];
		float 	fRec12_tmp[1024+4];
		float 	fRec13[1024];
		float 	fRec10_tmp[1024+4];
		float 	fRec11[1024];
		float 	fZec2[1024];
		float 	fRec43_tmp[1024+4];
		float 	fRec42_tmp[1024+4];
		float 	fRec45_tmp[1024+4];
		float 	fRec44_tmp[1024+4];
		float 	fRec47_tmp[1024+4];
		float 	fRec46_tmp[1024+4];
		float 	fRec49_tmp[1024+4];
		float 	fRec48_tmp[1024+4];
		float 	fRec51_tmp[1024+4];
		float 	fRec50_tmp[1024+4];
		float 	fRec53_tmp[1024+4];
		float 	fRec52_tmp[1024+4];
		float 	fRec55_tmp[1024+4];
		float 	fRec54_tmp[1024+4];
		float 	fRec57_tmp[1024+4];
		float 	fRec56_tmp[1024+4];
		float 	fZec3[1024];
		float 	fRec40_tmp[1024+4];
		float 	fRec41[1024];
		float 	fRec38_tmp[1024+4];
		float 	fRec39[1024];
		float 	fRec36_tmp[1024+4];
		float 	fRec37[1024];
		float 	fRec34_tmp[1024+4];
		float 	fRec35[1024];
		float 	fZec4[1024];
		float 	fZec5[1024];
		float 	fRec8_tmp[1024+4];
		float 	fRec59_tmp[1024+4];
		float 	fRec58_tmp[1024+4];
		float 	fRec61_tmp[1024+4];
		float 	fRec60_tmp[1024+4];
		float 	fRec63_tmp[1024+4];
		float 	fRec62_tmp[1024+4];
		float 	fRec65_tmp[1024+4];
		float 	fRec64_tmp[1024+4];
		float 	fRec67_tmp[1024+4];
		float 	fRec66_tmp[1024+4];
		float 	fRec69_tmp[1024+4];
		float 	fRec68_tmp[1024+4];
		float 	fRec71_tmp[1024+4];
		float 	fRec70_tmp[1024+4];
		float 	fZec6[1024];
		float 	fRec6_tmp[1024+4];
		float 	fRec7[1024];
		float 	fRec4_tmp[1024+4];
		float 	fRec5[1024];
		float 	fRec2_tmp[1024+4];
		float 	fRec3[1024];
		float 	fRec0_tmp[1024+4];
		float 	fRec1[1024];
		float 	fRec81_tmp[1024+4];
		float 	fRec80_tmp[1024+4];
		float 	fRec83_tmp[1024+4];
		float 	fRec82_tmp[1024+4];
		float 	fRec85_tmp[1024+4];
		float 	fRec84_tmp[1024+4];
		float 	fRec87_tmp[1024+4];
		float 	fRec86_tmp[1024+4];
		float 	fRec89_tmp[1024+4];
		float 	fRec88_tmp[1024+4];
		float 	fRec91_tmp[1024+4];
		float 	fRec90_tmp[1024+4];
		float 	fRec93_tmp[1024+4];
		float 	fRec92_tmp[1024+4];
		float 	fRec95_tmp[1024+4];
		float 	fRec94_tmp[1024+4];
		float 	fZec7[1024];
		float 	fRec78_tmp[1024+4];
		float 	fRec79[1024];
		float 	fRec76_tmp[1024+4];
		float 	fRec77[1024];
		float 	fRec74_tmp[1024+4];
		float 	fRec75[1024];
		float 	fRec72_tmp[1024+4];
		float 	fRec73[1024];
		float 	fSlow0 = (0.4f * fslider0);
		float 	fSlow1 = (1 - fSlow0);
		float* 	fRec9 = &fRec9_tmp[4];
		float 	fSlow2 = (0.7f + (0.28f * fslider1));
		float* 	fRec19 = &fRec19_tmp[4];
		float* 	fRec18 = &fRec18_tmp[4];
		float* 	fRec21 = &fRec21_tmp[4];
		float* 	fRec20 = &fRec20_tmp[4];
		float* 	fRec23 = &fRec23_tmp[4];
		float* 	fRec22 = &fRec22_tmp[4];
		float* 	fRec25 = &fRec25_tmp[4];
		float* 	fRec24 = &fRec24_tmp[4];
		float* 	fRec27 = &fRec27_tmp[4];
		float* 	fRec26 = &fRec26_tmp[4];
		float* 	fRec29 = &fRec29_tmp[4];
		float* 	fRec28 = &fRec28_tmp[4];
		float* 	fRec31 = &fRec31_tmp[4];
		float* 	fRec30 = &fRec30_tmp[4];
		float* 	fRec33 = &fRec33_tmp[4];
		float* 	fRec32 = &fRec32_tmp[4];
		float* 	fRec16 = &fRec16_tmp[4];
		float* 	fRec14 = &fRec14_tmp[4];
		float* 	fRec12 = &fRec12_tmp[4];
		float* 	fRec10 = &fRec10_tmp[4];
		float 	fSlow3 = fslider2;
		float 	fSlow4 = (1 - fSlow3);
		float* 	fRec43 = &fRec43_tmp[4];
		float* 	fRec42 = &fRec42_tmp[4];
		float* 	fRec45 = &fRec45_tmp[4];
		float* 	fRec44 = &fRec44_tmp[4];
		float* 	fRec47 = &fRec47_tmp[4];
		float* 	fRec46 = &fRec46_tmp[4];
		float* 	fRec49 = &fRec49_tmp[4];
		float* 	fRec48 = &fRec48_tmp[4];
		float* 	fRec51 = &fRec51_tmp[4];
		float* 	fRec50 = &fRec50_tmp[4];
		float* 	fRec53 = &fRec53_tmp[4];
		float* 	fRec52 = &fRec52_tmp[4];
		float* 	fRec55 = &fRec55_tmp[4];
		float* 	fRec54 = &fRec54_tmp[4];
		float* 	fRec57 = &fRec57_tmp[4];
		float* 	fRec56 = &fRec56_tmp[4];
		float* 	fRec40 = &fRec40_tmp[4];
		float* 	fRec38 = &fRec38_tmp[4];
		float* 	fRec36 = &fRec36_tmp[4];
		float* 	fRec34 = &fRec34_tmp[4];
		float* 	fRec8 = &fRec8_tmp[4];
		float* 	fRec59 = &fRec59_tmp[4];
		float* 	fRec58 = &fRec58_tmp[4];
		float* 	fRec61 = &fRec61_tmp[4];
		float* 	fRec60 = &fRec60_tmp[4];
		float* 	fRec63 = &fRec63_tmp[4];
		float* 	fRec62 = &fRec62_tmp[4];
		float* 	fRec65 = &fRec65_tmp[4];
		float* 	fRec64 = &fRec64_tmp[4];
		float* 	fRec67 = &fRec67_tmp[4];
		float* 	fRec66 = &fRec66_tmp[4];
		float* 	fRec69 = &fRec69_tmp[4];
		float* 	fRec68 = &fRec68_tmp[4];
		float* 	fRec71 = &fRec71_tmp[4];
		float* 	fRec70 = &fRec70_tmp[4];
		float* 	fRec6 = &fRec6_tmp[4];
		float* 	fRec4 = &fRec4_tmp[4];
		float* 	fRec2 = &fRec2_tmp[4];
		float* 	fRec0 = &fRec0_tmp[4];
		float* 	fRec81 = &fRec81_tmp[4];
		float* 	fRec80 = &fRec80_tmp[4];
		float* 	fRec83 = &fRec83_tmp[4];
		float* 	fRec82 = &fRec82_tmp[4];
		float* 	fRec85 = &fRec85_tmp[4];
		float* 	fRec84 = &fRec84_tmp[4];
		float* 	fRec87 = &fRec87_tmp[4];
		float* 	fRec86 = &fRec86_tmp[4];
		float* 	fRec89 = &fRec89_tmp[4];
		float* 	fRec88 = &fRec88_tmp[4];
		float* 	fRec91 = &fRec91_tmp[4];
		float* 	fRec90 = &fRec90_tmp[4];
		float* 	fRec93 = &fRec93_tmp[4];
		float* 	fRec92 = &fRec92_tmp[4];
		float* 	fRec95 = &fRec95_tmp[4];
		float* 	fRec94 = &fRec94_tmp[4];
		float* 	fRec78 = &fRec78_tmp[4];
		float* 	fRec76 = &fRec76_tmp[4];
		float* 	fRec74 = &fRec74_tmp[4];
		float* 	fRec72 = &fRec72_tmp[4];
		int index;
		for (index = 0; index <= fullcount - 1024; index += 1024) {
			// compute by blocks of 1024 samples
			const int count = 1024;
			FAUSTFLOAT* input0 = &input[0][index];
			FAUSTFLOAT* input1 = &input[1][index];
			FAUSTFLOAT* output0 = &output[0][index];
			FAUSTFLOAT* output1 = &output[1][index];
			// SECTION : 1
			// LOOP 0x101350bc0
			// exec code
			for (int i=0; i<count; i++) {
				fZec0[i] = (1.500000e-02f * ((float)input0[i] + (float)input1[i]));
			}
			
			// SECTION : 2
			// LOOP 0x10134f970
			// pre processing
			for (int i=0; i<4; i++) fRec19_tmp[i]=fRec19_perm[i];
			fYec0_idx = (fYec0_idx+fYec0_idx_save)&4095;
			for (int i=0; i<4; i++) fRec18_tmp[i]=fRec18_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec19[i] = ((fSlow1 * fRec18[i-1]) + (fSlow0 * fRec19[i-1]));
				fYec0[(fYec0_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec19[i]));
				fRec18[i] = fYec0[(fYec0_idx+i-1617)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec18_perm[i]=fRec18_tmp[count+i];
			fYec0_idx_save = count;
			for (int i=0; i<4; i++) fRec19_perm[i]=fRec19_tmp[count+i];
			
			// LOOP 0x101351ed0
			// pre processing
			for (int i=0; i<4; i++) fRec21_tmp[i]=fRec21_perm[i];
			fYec1_idx = (fYec1_idx+fYec1_idx_save)&4095;
			for (int i=0; i<4; i++) fRec20_tmp[i]=fRec20_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec21[i] = ((fSlow1 * fRec20[i-1]) + (fSlow0 * fRec21[i-1]));
				fYec1[(fYec1_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec21[i]));
				fRec20[i] = fYec1[(fYec1_idx+i-1557)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec20_perm[i]=fRec20_tmp[count+i];
			fYec1_idx_save = count;
			for (int i=0; i<4; i++) fRec21_perm[i]=fRec21_tmp[count+i];
			
			// LOOP 0x101353a50
			// pre processing
			for (int i=0; i<4; i++) fRec23_tmp[i]=fRec23_perm[i];
			fYec2_idx = (fYec2_idx+fYec2_idx_save)&4095;
			for (int i=0; i<4; i++) fRec22_tmp[i]=fRec22_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec23[i] = ((fSlow1 * fRec22[i-1]) + (fSlow0 * fRec23[i-1]));
				fYec2[(fYec2_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec23[i]));
				fRec22[i] = fYec2[(fYec2_idx+i-1491)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec22_perm[i]=fRec22_tmp[count+i];
			fYec2_idx_save = count;
			for (int i=0; i<4; i++) fRec23_perm[i]=fRec23_tmp[count+i];
			
			// LOOP 0x1013555d0
			// pre processing
			for (int i=0; i<4; i++) fRec25_tmp[i]=fRec25_perm[i];
			fYec3_idx = (fYec3_idx+fYec3_idx_save)&4095;
			for (int i=0; i<4; i++) fRec24_tmp[i]=fRec24_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec25[i] = ((fSlow1 * fRec24[i-1]) + (fSlow0 * fRec25[i-1]));
				fYec3[(fYec3_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec25[i]));
				fRec24[i] = fYec3[(fYec3_idx+i-1422)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec24_perm[i]=fRec24_tmp[count+i];
			fYec3_idx_save = count;
			for (int i=0; i<4; i++) fRec25_perm[i]=fRec25_tmp[count+i];
			
			// LOOP 0x101357120
			// pre processing
			for (int i=0; i<4; i++) fRec27_tmp[i]=fRec27_perm[i];
			fYec4_idx = (fYec4_idx+fYec4_idx_save)&4095;
			for (int i=0; i<4; i++) fRec26_tmp[i]=fRec26_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec27[i] = ((fSlow1 * fRec26[i-1]) + (fSlow0 * fRec27[i-1]));
				fYec4[(fYec4_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec27[i]));
				fRec26[i] = fYec4[(fYec4_idx+i-1356)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec26_perm[i]=fRec26_tmp[count+i];
			fYec4_idx_save = count;
			for (int i=0; i<4; i++) fRec27_perm[i]=fRec27_tmp[count+i];
			
			// LOOP 0x101358c70
			// pre processing
			for (int i=0; i<4; i++) fRec29_tmp[i]=fRec29_perm[i];
			fYec5_idx = (fYec5_idx+fYec5_idx_save)&4095;
			for (int i=0; i<4; i++) fRec28_tmp[i]=fRec28_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec29[i] = ((fSlow1 * fRec28[i-1]) + (fSlow0 * fRec29[i-1]));
				fYec5[(fYec5_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec29[i]));
				fRec28[i] = fYec5[(fYec5_idx+i-1277)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec28_perm[i]=fRec28_tmp[count+i];
			fYec5_idx_save = count;
			for (int i=0; i<4; i++) fRec29_perm[i]=fRec29_tmp[count+i];
			
			// LOOP 0x10135a7e0
			// pre processing
			for (int i=0; i<4; i++) fRec31_tmp[i]=fRec31_perm[i];
			fYec6_idx = (fYec6_idx+fYec6_idx_save)&4095;
			for (int i=0; i<4; i++) fRec30_tmp[i]=fRec30_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec31[i] = ((fSlow1 * fRec30[i-1]) + (fSlow0 * fRec31[i-1]));
				fYec6[(fYec6_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec31[i]));
				fRec30[i] = fYec6[(fYec6_idx+i-1188)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec30_perm[i]=fRec30_tmp[count+i];
			fYec6_idx_save = count;
			for (int i=0; i<4; i++) fRec31_perm[i]=fRec31_tmp[count+i];
			
			// LOOP 0x10135c330
			// pre processing
			for (int i=0; i<4; i++) fRec33_tmp[i]=fRec33_perm[i];
			fYec7_idx = (fYec7_idx+fYec7_idx_save)&4095;
			for (int i=0; i<4; i++) fRec32_tmp[i]=fRec32_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec33[i] = ((fSlow1 * fRec32[i-1]) + (fSlow0 * fRec33[i-1]));
				fYec7[(fYec7_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec33[i]));
				fRec32[i] = fYec7[(fYec7_idx+i-1116)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec32_perm[i]=fRec32_tmp[count+i];
			fYec7_idx_save = count;
			for (int i=0; i<4; i++) fRec33_perm[i]=fRec33_tmp[count+i];
			
			// LOOP 0x101364b70
			// pre processing
			for (int i=0; i<4; i++) fRec43_tmp[i]=fRec43_perm[i];
			fYec12_idx = (fYec12_idx+fYec12_idx_save)&4095;
			for (int i=0; i<4; i++) fRec42_tmp[i]=fRec42_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec43[i] = ((fSlow1 * fRec42[i-1]) + (fSlow0 * fRec43[i-1]));
				fYec12[(fYec12_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec43[i]));
				fRec42[i] = fYec12[(fYec12_idx+i-1640)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec42_perm[i]=fRec42_tmp[count+i];
			fYec12_idx_save = count;
			for (int i=0; i<4; i++) fRec43_perm[i]=fRec43_tmp[count+i];
			
			// LOOP 0x1013667b0
			// pre processing
			for (int i=0; i<4; i++) fRec45_tmp[i]=fRec45_perm[i];
			fYec13_idx = (fYec13_idx+fYec13_idx_save)&4095;
			for (int i=0; i<4; i++) fRec44_tmp[i]=fRec44_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec45[i] = ((fSlow1 * fRec44[i-1]) + (fSlow0 * fRec45[i-1]));
				fYec13[(fYec13_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec45[i]));
				fRec44[i] = fYec13[(fYec13_idx+i-1580)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec44_perm[i]=fRec44_tmp[count+i];
			fYec13_idx_save = count;
			for (int i=0; i<4; i++) fRec45_perm[i]=fRec45_tmp[count+i];
			
			// LOOP 0x101368330
			// pre processing
			for (int i=0; i<4; i++) fRec47_tmp[i]=fRec47_perm[i];
			fYec14_idx = (fYec14_idx+fYec14_idx_save)&4095;
			for (int i=0; i<4; i++) fRec46_tmp[i]=fRec46_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec47[i] = ((fSlow1 * fRec46[i-1]) + (fSlow0 * fRec47[i-1]));
				fYec14[(fYec14_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec47[i]));
				fRec46[i] = fYec14[(fYec14_idx+i-1514)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec46_perm[i]=fRec46_tmp[count+i];
			fYec14_idx_save = count;
			for (int i=0; i<4; i++) fRec47_perm[i]=fRec47_tmp[count+i];
			
			// LOOP 0x101369f40
			// pre processing
			for (int i=0; i<4; i++) fRec49_tmp[i]=fRec49_perm[i];
			fYec15_idx = (fYec15_idx+fYec15_idx_save)&4095;
			for (int i=0; i<4; i++) fRec48_tmp[i]=fRec48_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec49[i] = ((fSlow1 * fRec48[i-1]) + (fSlow0 * fRec49[i-1]));
				fYec15[(fYec15_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec49[i]));
				fRec48[i] = fYec15[(fYec15_idx+i-1445)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec48_perm[i]=fRec48_tmp[count+i];
			fYec15_idx_save = count;
			for (int i=0; i<4; i++) fRec49_perm[i]=fRec49_tmp[count+i];
			
			// LOOP 0x10136bae0
			// pre processing
			for (int i=0; i<4; i++) fRec51_tmp[i]=fRec51_perm[i];
			fYec16_idx = (fYec16_idx+fYec16_idx_save)&4095;
			for (int i=0; i<4; i++) fRec50_tmp[i]=fRec50_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec51[i] = ((fSlow1 * fRec50[i-1]) + (fSlow0 * fRec51[i-1]));
				fYec16[(fYec16_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec51[i]));
				fRec50[i] = fYec16[(fYec16_idx+i-1379)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec50_perm[i]=fRec50_tmp[count+i];
			fYec16_idx_save = count;
			for (int i=0; i<4; i++) fRec51_perm[i]=fRec51_tmp[count+i];
			
			// LOOP 0x10136d660
			// pre processing
			for (int i=0; i<4; i++) fRec53_tmp[i]=fRec53_perm[i];
			fYec17_idx = (fYec17_idx+fYec17_idx_save)&4095;
			for (int i=0; i<4; i++) fRec52_tmp[i]=fRec52_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec53[i] = ((fSlow1 * fRec52[i-1]) + (fSlow0 * fRec53[i-1]));
				fYec17[(fYec17_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec53[i]));
				fRec52[i] = fYec17[(fYec17_idx+i-1300)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec52_perm[i]=fRec52_tmp[count+i];
			fYec17_idx_save = count;
			for (int i=0; i<4; i++) fRec53_perm[i]=fRec53_tmp[count+i];
			
			// LOOP 0x10136f1e0
			// pre processing
			for (int i=0; i<4; i++) fRec55_tmp[i]=fRec55_perm[i];
			fYec18_idx = (fYec18_idx+fYec18_idx_save)&4095;
			for (int i=0; i<4; i++) fRec54_tmp[i]=fRec54_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec55[i] = ((fSlow1 * fRec54[i-1]) + (fSlow0 * fRec55[i-1]));
				fYec18[(fYec18_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec55[i]));
				fRec54[i] = fYec18[(fYec18_idx+i-1211)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec54_perm[i]=fRec54_tmp[count+i];
			fYec18_idx_save = count;
			for (int i=0; i<4; i++) fRec55_perm[i]=fRec55_tmp[count+i];
			
			// LOOP 0x101370d60
			// pre processing
			for (int i=0; i<4; i++) fRec57_tmp[i]=fRec57_perm[i];
			fYec19_idx = (fYec19_idx+fYec19_idx_save)&4095;
			for (int i=0; i<4; i++) fRec56_tmp[i]=fRec56_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec57[i] = ((fSlow1 * fRec56[i-1]) + (fSlow0 * fRec57[i-1]));
				fYec19[(fYec19_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec57[i]));
				fRec56[i] = fYec19[(fYec19_idx+i-1139)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec56_perm[i]=fRec56_tmp[count+i];
			fYec19_idx_save = count;
			for (int i=0; i<4; i++) fRec57_perm[i]=fRec57_tmp[count+i];
			
			// SECTION : 3
			// LOOP 0x10134f870
			// exec code
			for (int i=0; i<count; i++) {
				fZec1[i] = (((((((fRec32[i] + fRec30[i]) + fRec28[i]) + fRec26[i]) + fRec24[i]) + fRec22[i]) + fRec20[i]) + fRec18[i]);
			}
			
			// LOOP 0x101364a70
			// exec code
			for (int i=0; i<count; i++) {
				fZec3[i] = (((((((fRec56[i] + fRec54[i]) + fRec52[i]) + fRec50[i]) + fRec48[i]) + fRec46[i]) + fRec44[i]) + fRec42[i]);
			}
			
			// SECTION : 4
			// LOOP 0x10134f120
			// pre processing
			fYec8_idx = (fYec8_idx+fYec8_idx_save)&2047;
			for (int i=0; i<4; i++) fRec16_tmp[i]=fRec16_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec8[(fYec8_idx+i)&2047] = (fZec1[i] + (0.5f * fRec16[i-1]));
				fRec16[i] = fYec8[(fYec8_idx+i-556)&2047];
				fRec17[i] = (0 - (fZec1[i] - fRec16[i-1]));
			}
			// post processing
			for (int i=0; i<4; i++) fRec16_perm[i]=fRec16_tmp[count+i];
			fYec8_idx_save = count;
			
			// LOOP 0x101364320
			// pre processing
			fYec20_idx = (fYec20_idx+fYec20_idx_save)&2047;
			for (int i=0; i<4; i++) fRec40_tmp[i]=fRec40_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec20[(fYec20_idx+i)&2047] = (fZec3[i] + (0.5f * fRec40[i-1]));
				fRec40[i] = fYec20[(fYec20_idx+i-579)&2047];
				fRec41[i] = (0 - (fZec3[i] - fRec40[i-1]));
			}
			// post processing
			for (int i=0; i<4; i++) fRec40_perm[i]=fRec40_tmp[count+i];
			fYec20_idx_save = count;
			
			// SECTION : 5
			// LOOP 0x10134e9d0
			// pre processing
			fYec9_idx = (fYec9_idx+fYec9_idx_save)&2047;
			for (int i=0; i<4; i++) fRec14_tmp[i]=fRec14_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec9[(fYec9_idx+i)&2047] = (fRec17[i] + (0.5f * fRec14[i-1]));
				fRec14[i] = fYec9[(fYec9_idx+i-441)&2047];
				fRec15[i] = (fRec14[i-1] - fRec17[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec14_perm[i]=fRec14_tmp[count+i];
			fYec9_idx_save = count;
			
			// LOOP 0x101363bd0
			// pre processing
			fYec21_idx = (fYec21_idx+fYec21_idx_save)&2047;
			for (int i=0; i<4; i++) fRec38_tmp[i]=fRec38_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec21[(fYec21_idx+i)&2047] = (fRec41[i] + (0.5f * fRec38[i-1]));
				fRec38[i] = fYec21[(fYec21_idx+i-464)&2047];
				fRec39[i] = (fRec38[i-1] - fRec41[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec38_perm[i]=fRec38_tmp[count+i];
			fYec21_idx_save = count;
			
			// SECTION : 6
			// LOOP 0x10134e2a0
			// pre processing
			fYec10_idx = (fYec10_idx+fYec10_idx_save)&2047;
			for (int i=0; i<4; i++) fRec12_tmp[i]=fRec12_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec10[(fYec10_idx+i)&2047] = (fRec15[i] + (0.5f * fRec12[i-1]));
				fRec12[i] = fYec10[(fYec10_idx+i-341)&2047];
				fRec13[i] = (fRec12[i-1] - fRec15[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec12_perm[i]=fRec12_tmp[count+i];
			fYec10_idx_save = count;
			
			// LOOP 0x1013634a0
			// pre processing
			fYec22_idx = (fYec22_idx+fYec22_idx_save)&2047;
			for (int i=0; i<4; i++) fRec36_tmp[i]=fRec36_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec22[(fYec22_idx+i)&2047] = (fRec39[i] + (0.5f * fRec36[i-1]));
				fRec36[i] = fYec22[(fYec22_idx+i-364)&2047];
				fRec37[i] = (fRec36[i-1] - fRec39[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec36_perm[i]=fRec36_tmp[count+i];
			fYec22_idx_save = count;
			
			// SECTION : 7
			// LOOP 0x10134dba0
			// pre processing
			fYec11_idx = (fYec11_idx+fYec11_idx_save)&2047;
			for (int i=0; i<4; i++) fRec10_tmp[i]=fRec10_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec11[(fYec11_idx+i)&2047] = (fRec13[i] + (0.5f * fRec10[i-1]));
				fRec10[i] = fYec11[(fYec11_idx+i-225)&2047];
				fRec11[i] = (fRec10[i-1] - fRec13[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec10_perm[i]=fRec10_tmp[count+i];
			fYec11_idx_save = count;
			
			// LOOP 0x101362e30
			// pre processing
			fYec23_idx = (fYec23_idx+fYec23_idx_save)&2047;
			for (int i=0; i<4; i++) fRec34_tmp[i]=fRec34_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec23[(fYec23_idx+i)&2047] = (fRec37[i] + (0.5f * fRec34[i-1]));
				fRec34[i] = fYec23[(fYec23_idx+i-248)&2047];
				fRec35[i] = (fRec34[i-1] - fRec37[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec34_perm[i]=fRec34_tmp[count+i];
			fYec23_idx_save = count;
			
			// SECTION : 8
			// LOOP 0x10134daa0
			// exec code
			for (int i=0; i<count; i++) {
				fZec2[i] = ((fSlow4 * (float)input0[i]) + (fSlow3 * fRec11[i]));
			}
			
			// LOOP 0x101362d30
			// exec code
			for (int i=0; i<count; i++) {
				fZec4[i] = ((fSlow4 * (float)input1[i]) + (fSlow3 * fRec35[i]));
			}
			
			// SECTION : 9
			// LOOP 0x10134d9a0
			// exec code
			for (int i=0; i<count; i++) {
				fZec5[i] = (1.500000e-02f * (fZec4[i] + fZec2[i]));
			}
			
			// SECTION : 10
			// LOOP 0x10134b4b0
			// pre processing
			for (int i=0; i<4; i++) fRec9_tmp[i]=fRec9_perm[i];
			fYec24_idx = (fYec24_idx+fYec24_idx_save)&4095;
			for (int i=0; i<4; i++) fRec8_tmp[i]=fRec8_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec9[i] = ((fSlow1 * fRec8[i-1]) + (fSlow0 * fRec9[i-1]));
				fYec24[(fYec24_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec9[i]));
				fRec8[i] = fYec24[(fYec24_idx+i-1617)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec8_perm[i]=fRec8_tmp[count+i];
			fYec24_idx_save = count;
			for (int i=0; i<4; i++) fRec9_perm[i]=fRec9_tmp[count+i];
			
			// LOOP 0x101377dc0
			// pre processing
			for (int i=0; i<4; i++) fRec59_tmp[i]=fRec59_perm[i];
			fYec25_idx = (fYec25_idx+fYec25_idx_save)&4095;
			for (int i=0; i<4; i++) fRec58_tmp[i]=fRec58_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec59[i] = ((fSlow1 * fRec58[i-1]) + (fSlow0 * fRec59[i-1]));
				fYec25[(fYec25_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec59[i]));
				fRec58[i] = fYec25[(fYec25_idx+i-1557)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec58_perm[i]=fRec58_tmp[count+i];
			fYec25_idx_save = count;
			for (int i=0; i<4; i++) fRec59_perm[i]=fRec59_tmp[count+i];
			
			// LOOP 0x101379900
			// pre processing
			for (int i=0; i<4; i++) fRec61_tmp[i]=fRec61_perm[i];
			fYec26_idx = (fYec26_idx+fYec26_idx_save)&4095;
			for (int i=0; i<4; i++) fRec60_tmp[i]=fRec60_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec61[i] = ((fSlow1 * fRec60[i-1]) + (fSlow0 * fRec61[i-1]));
				fYec26[(fYec26_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec61[i]));
				fRec60[i] = fYec26[(fYec26_idx+i-1491)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec60_perm[i]=fRec60_tmp[count+i];
			fYec26_idx_save = count;
			for (int i=0; i<4; i++) fRec61_perm[i]=fRec61_tmp[count+i];
			
			// LOOP 0x10137b480
			// pre processing
			for (int i=0; i<4; i++) fRec63_tmp[i]=fRec63_perm[i];
			fYec27_idx = (fYec27_idx+fYec27_idx_save)&4095;
			for (int i=0; i<4; i++) fRec62_tmp[i]=fRec62_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec63[i] = ((fSlow1 * fRec62[i-1]) + (fSlow0 * fRec63[i-1]));
				fYec27[(fYec27_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec63[i]));
				fRec62[i] = fYec27[(fYec27_idx+i-1422)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec62_perm[i]=fRec62_tmp[count+i];
			fYec27_idx_save = count;
			for (int i=0; i<4; i++) fRec63_perm[i]=fRec63_tmp[count+i];
			
			// LOOP 0x10137d000
			// pre processing
			for (int i=0; i<4; i++) fRec65_tmp[i]=fRec65_perm[i];
			fYec28_idx = (fYec28_idx+fYec28_idx_save)&4095;
			for (int i=0; i<4; i++) fRec64_tmp[i]=fRec64_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec65[i] = ((fSlow1 * fRec64[i-1]) + (fSlow0 * fRec65[i-1]));
				fYec28[(fYec28_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec65[i]));
				fRec64[i] = fYec28[(fYec28_idx+i-1356)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec64_perm[i]=fRec64_tmp[count+i];
			fYec28_idx_save = count;
			for (int i=0; i<4; i++) fRec65_perm[i]=fRec65_tmp[count+i];
			
			// LOOP 0x10137eb80
			// pre processing
			for (int i=0; i<4; i++) fRec67_tmp[i]=fRec67_perm[i];
			fYec29_idx = (fYec29_idx+fYec29_idx_save)&4095;
			for (int i=0; i<4; i++) fRec66_tmp[i]=fRec66_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec67[i] = ((fSlow1 * fRec66[i-1]) + (fSlow0 * fRec67[i-1]));
				fYec29[(fYec29_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec67[i]));
				fRec66[i] = fYec29[(fYec29_idx+i-1277)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec66_perm[i]=fRec66_tmp[count+i];
			fYec29_idx_save = count;
			for (int i=0; i<4; i++) fRec67_perm[i]=fRec67_tmp[count+i];
			
			// LOOP 0x101380700
			// pre processing
			for (int i=0; i<4; i++) fRec69_tmp[i]=fRec69_perm[i];
			fYec30_idx = (fYec30_idx+fYec30_idx_save)&4095;
			for (int i=0; i<4; i++) fRec68_tmp[i]=fRec68_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec69[i] = ((fSlow1 * fRec68[i-1]) + (fSlow0 * fRec69[i-1]));
				fYec30[(fYec30_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec69[i]));
				fRec68[i] = fYec30[(fYec30_idx+i-1188)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec68_perm[i]=fRec68_tmp[count+i];
			fYec30_idx_save = count;
			for (int i=0; i<4; i++) fRec69_perm[i]=fRec69_tmp[count+i];
			
			// LOOP 0x101382280
			// pre processing
			for (int i=0; i<4; i++) fRec71_tmp[i]=fRec71_perm[i];
			fYec31_idx = (fYec31_idx+fYec31_idx_save)&4095;
			for (int i=0; i<4; i++) fRec70_tmp[i]=fRec70_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec71[i] = ((fSlow1 * fRec70[i-1]) + (fSlow0 * fRec71[i-1]));
				fYec31[(fYec31_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec71[i]));
				fRec70[i] = fYec31[(fYec31_idx+i-1116)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec70_perm[i]=fRec70_tmp[count+i];
			fYec31_idx_save = count;
			for (int i=0; i<4; i++) fRec71_perm[i]=fRec71_tmp[count+i];
			
			// LOOP 0x101389fc0
			// pre processing
			for (int i=0; i<4; i++) fRec81_tmp[i]=fRec81_perm[i];
			fYec36_idx = (fYec36_idx+fYec36_idx_save)&4095;
			for (int i=0; i<4; i++) fRec80_tmp[i]=fRec80_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec81[i] = ((fSlow1 * fRec80[i-1]) + (fSlow0 * fRec81[i-1]));
				fYec36[(fYec36_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec81[i]));
				fRec80[i] = fYec36[(fYec36_idx+i-1640)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec80_perm[i]=fRec80_tmp[count+i];
			fYec36_idx_save = count;
			for (int i=0; i<4; i++) fRec81_perm[i]=fRec81_tmp[count+i];
			
			// LOOP 0x10138bc10
			// pre processing
			for (int i=0; i<4; i++) fRec83_tmp[i]=fRec83_perm[i];
			fYec37_idx = (fYec37_idx+fYec37_idx_save)&4095;
			for (int i=0; i<4; i++) fRec82_tmp[i]=fRec82_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec83[i] = ((fSlow1 * fRec82[i-1]) + (fSlow0 * fRec83[i-1]));
				fYec37[(fYec37_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec83[i]));
				fRec82[i] = fYec37[(fYec37_idx+i-1580)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec82_perm[i]=fRec82_tmp[count+i];
			fYec37_idx_save = count;
			for (int i=0; i<4; i++) fRec83_perm[i]=fRec83_tmp[count+i];
			
			// LOOP 0x10138d7b0
			// pre processing
			for (int i=0; i<4; i++) fRec85_tmp[i]=fRec85_perm[i];
			fYec38_idx = (fYec38_idx+fYec38_idx_save)&4095;
			for (int i=0; i<4; i++) fRec84_tmp[i]=fRec84_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec85[i] = ((fSlow1 * fRec84[i-1]) + (fSlow0 * fRec85[i-1]));
				fYec38[(fYec38_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec85[i]));
				fRec84[i] = fYec38[(fYec38_idx+i-1514)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec84_perm[i]=fRec84_tmp[count+i];
			fYec38_idx_save = count;
			for (int i=0; i<4; i++) fRec85_perm[i]=fRec85_tmp[count+i];
			
			// LOOP 0x10138f330
			// pre processing
			for (int i=0; i<4; i++) fRec87_tmp[i]=fRec87_perm[i];
			fYec39_idx = (fYec39_idx+fYec39_idx_save)&4095;
			for (int i=0; i<4; i++) fRec86_tmp[i]=fRec86_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec87[i] = ((fSlow1 * fRec86[i-1]) + (fSlow0 * fRec87[i-1]));
				fYec39[(fYec39_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec87[i]));
				fRec86[i] = fYec39[(fYec39_idx+i-1445)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec86_perm[i]=fRec86_tmp[count+i];
			fYec39_idx_save = count;
			for (int i=0; i<4; i++) fRec87_perm[i]=fRec87_tmp[count+i];
			
			// LOOP 0x101390eb0
			// pre processing
			for (int i=0; i<4; i++) fRec89_tmp[i]=fRec89_perm[i];
			fYec40_idx = (fYec40_idx+fYec40_idx_save)&4095;
			for (int i=0; i<4; i++) fRec88_tmp[i]=fRec88_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec89[i] = ((fSlow1 * fRec88[i-1]) + (fSlow0 * fRec89[i-1]));
				fYec40[(fYec40_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec89[i]));
				fRec88[i] = fYec40[(fYec40_idx+i-1379)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec88_perm[i]=fRec88_tmp[count+i];
			fYec40_idx_save = count;
			for (int i=0; i<4; i++) fRec89_perm[i]=fRec89_tmp[count+i];
			
			// LOOP 0x101392a30
			// pre processing
			for (int i=0; i<4; i++) fRec91_tmp[i]=fRec91_perm[i];
			fYec41_idx = (fYec41_idx+fYec41_idx_save)&4095;
			for (int i=0; i<4; i++) fRec90_tmp[i]=fRec90_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec91[i] = ((fSlow1 * fRec90[i-1]) + (fSlow0 * fRec91[i-1]));
				fYec41[(fYec41_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec91[i]));
				fRec90[i] = fYec41[(fYec41_idx+i-1300)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec90_perm[i]=fRec90_tmp[count+i];
			fYec41_idx_save = count;
			for (int i=0; i<4; i++) fRec91_perm[i]=fRec91_tmp[count+i];
			
			// LOOP 0x1013945b0
			// pre processing
			for (int i=0; i<4; i++) fRec93_tmp[i]=fRec93_perm[i];
			fYec42_idx = (fYec42_idx+fYec42_idx_save)&4095;
			for (int i=0; i<4; i++) fRec92_tmp[i]=fRec92_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec93[i] = ((fSlow1 * fRec92[i-1]) + (fSlow0 * fRec93[i-1]));
				fYec42[(fYec42_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec93[i]));
				fRec92[i] = fYec42[(fYec42_idx+i-1211)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec92_perm[i]=fRec92_tmp[count+i];
			fYec42_idx_save = count;
			for (int i=0; i<4; i++) fRec93_perm[i]=fRec93_tmp[count+i];
			
			// LOOP 0x101396130
			// pre processing
			for (int i=0; i<4; i++) fRec95_tmp[i]=fRec95_perm[i];
			fYec43_idx = (fYec43_idx+fYec43_idx_save)&4095;
			for (int i=0; i<4; i++) fRec94_tmp[i]=fRec94_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec95[i] = ((fSlow1 * fRec94[i-1]) + (fSlow0 * fRec95[i-1]));
				fYec43[(fYec43_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec95[i]));
				fRec94[i] = fYec43[(fYec43_idx+i-1139)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec94_perm[i]=fRec94_tmp[count+i];
			fYec43_idx_save = count;
			for (int i=0; i<4; i++) fRec95_perm[i]=fRec95_tmp[count+i];
			
			// SECTION : 11
			// LOOP 0x10134b3b0
			// exec code
			for (int i=0; i<count; i++) {
				fZec6[i] = (((((((fRec70[i] + fRec68[i]) + fRec66[i]) + fRec64[i]) + fRec62[i]) + fRec60[i]) + fRec58[i]) + fRec8[i]);
			}
			
			// LOOP 0x101389ec0
			// exec code
			for (int i=0; i<count; i++) {
				fZec7[i] = (((((((fRec94[i] + fRec92[i]) + fRec90[i]) + fRec88[i]) + fRec86[i]) + fRec84[i]) + fRec82[i]) + fRec80[i]);
			}
			
			// SECTION : 12
			// LOOP 0x10134acd0
			// pre processing
			fYec32_idx = (fYec32_idx+fYec32_idx_save)&2047;
			for (int i=0; i<4; i++) fRec6_tmp[i]=fRec6_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec32[(fYec32_idx+i)&2047] = (fZec6[i] + (0.5f * fRec6[i-1]));
				fRec6[i] = fYec32[(fYec32_idx+i-556)&2047];
				fRec7[i] = (0 - (fZec6[i] - fRec6[i-1]));
			}
			// post processing
			for (int i=0; i<4; i++) fRec6_perm[i]=fRec6_tmp[count+i];
			fYec32_idx_save = count;
			
			// LOOP 0x1013897e0
			// pre processing
			fYec44_idx = (fYec44_idx+fYec44_idx_save)&2047;
			for (int i=0; i<4; i++) fRec78_tmp[i]=fRec78_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec44[(fYec44_idx+i)&2047] = (fZec7[i] + (0.5f * fRec78[i-1]));
				fRec78[i] = fYec44[(fYec44_idx+i-579)&2047];
				fRec79[i] = (0 - (fZec7[i] - fRec78[i-1]));
			}
			// post processing
			for (int i=0; i<4; i++) fRec78_perm[i]=fRec78_tmp[count+i];
			fYec44_idx_save = count;
			
			// SECTION : 13
			// LOOP 0x10134a5f0
			// pre processing
			fYec33_idx = (fYec33_idx+fYec33_idx_save)&2047;
			for (int i=0; i<4; i++) fRec4_tmp[i]=fRec4_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec33[(fYec33_idx+i)&2047] = (fRec7[i] + (0.5f * fRec4[i-1]));
				fRec4[i] = fYec33[(fYec33_idx+i-441)&2047];
				fRec5[i] = (fRec4[i-1] - fRec7[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec4_perm[i]=fRec4_tmp[count+i];
			fYec33_idx_save = count;
			
			// LOOP 0x101389100
			// pre processing
			fYec45_idx = (fYec45_idx+fYec45_idx_save)&2047;
			for (int i=0; i<4; i++) fRec76_tmp[i]=fRec76_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec45[(fYec45_idx+i)&2047] = (fRec79[i] + (0.5f * fRec76[i-1]));
				fRec76[i] = fYec45[(fYec45_idx+i-464)&2047];
				fRec77[i] = (fRec76[i-1] - fRec79[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec76_perm[i]=fRec76_tmp[count+i];
			fYec45_idx_save = count;
			
			// SECTION : 14
			// LOOP 0x101349f10
			// pre processing
			fYec34_idx = (fYec34_idx+fYec34_idx_save)&2047;
			for (int i=0; i<4; i++) fRec2_tmp[i]=fRec2_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec34[(fYec34_idx+i)&2047] = (fRec5[i] + (0.5f * fRec2[i-1]));
				fRec2[i] = fYec34[(fYec34_idx+i-341)&2047];
				fRec3[i] = (fRec2[i-1] - fRec5[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec2_perm[i]=fRec2_tmp[count+i];
			fYec34_idx_save = count;
			
			// LOOP 0x101388a20
			// pre processing
			fYec46_idx = (fYec46_idx+fYec46_idx_save)&2047;
			for (int i=0; i<4; i++) fRec74_tmp[i]=fRec74_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec46[(fYec46_idx+i)&2047] = (fRec77[i] + (0.5f * fRec74[i-1]));
				fRec74[i] = fYec46[(fYec46_idx+i-364)&2047];
				fRec75[i] = (fRec74[i-1] - fRec77[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec74_perm[i]=fRec74_tmp[count+i];
			fYec46_idx_save = count;
			
			// SECTION : 15
			// LOOP 0x101349700
			// pre processing
			fYec35_idx = (fYec35_idx+fYec35_idx_save)&2047;
			for (int i=0; i<4; i++) fRec0_tmp[i]=fRec0_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec35[(fYec35_idx+i)&2047] = (fRec3[i] + (0.5f * fRec0[i-1]));
				fRec0[i] = fYec35[(fYec35_idx+i-225)&2047];
				fRec1[i] = (fRec0[i-1] - fRec3[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec0_perm[i]=fRec0_tmp[count+i];
			fYec35_idx_save = count;
			
			// LOOP 0x1013883a0
			// pre processing
			fYec47_idx = (fYec47_idx+fYec47_idx_save)&2047;
			for (int i=0; i<4; i++) fRec72_tmp[i]=fRec72_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec47[(fYec47_idx+i)&2047] = (fRec75[i] + (0.5f * fRec72[i-1]));
				fRec72[i] = fYec47[(fYec47_idx+i-248)&2047];
				fRec73[i] = (fRec72[i-1] - fRec75[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec72_perm[i]=fRec72_tmp[count+i];
			fYec47_idx_save = count;
			
			// SECTION : 16
			// LOOP 0x101349600
			// exec code
			for (int i=0; i<count; i++) {
				output0[i] = (FAUSTFLOAT)((fSlow4 * fZec2[i]) + (fSlow3 * fRec1[i]));
			}
			
			// LOOP 0x1013881d0
			// exec code
			for (int i=0; i<count; i++) {
				output1[i] = (FAUSTFLOAT)((fSlow4 * fZec4[i]) + (fSlow3 * fRec73[i]));
			}
			
		}
		if (index < fullcount) {
			// compute the remaining samples if any
			int count = fullcount-index;
			FAUSTFLOAT* input0 = &input[0][index];
			FAUSTFLOAT* input1 = &input[1][index];
			FAUSTFLOAT* output0 = &output[0][index];
			FAUSTFLOAT* output1 = &output[1][index];
			// SECTION : 1
			// LOOP 0x101350bc0
			// exec code
			for (int i=0; i<count; i++) {
				fZec0[i] = (1.500000e-02f * ((float)input0[i] + (float)input1[i]));
			}
			
			// SECTION : 2
			// LOOP 0x10134f970
			// pre processing
			for (int i=0; i<4; i++) fRec19_tmp[i]=fRec19_perm[i];
			fYec0_idx = (fYec0_idx+fYec0_idx_save)&4095;
			for (int i=0; i<4; i++) fRec18_tmp[i]=fRec18_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec19[i] = ((fSlow1 * fRec18[i-1]) + (fSlow0 * fRec19[i-1]));
				fYec0[(fYec0_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec19[i]));
				fRec18[i] = fYec0[(fYec0_idx+i-1617)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec18_perm[i]=fRec18_tmp[count+i];
			fYec0_idx_save = count;
			for (int i=0; i<4; i++) fRec19_perm[i]=fRec19_tmp[count+i];
			
			// LOOP 0x101351ed0
			// pre processing
			for (int i=0; i<4; i++) fRec21_tmp[i]=fRec21_perm[i];
			fYec1_idx = (fYec1_idx+fYec1_idx_save)&4095;
			for (int i=0; i<4; i++) fRec20_tmp[i]=fRec20_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec21[i] = ((fSlow1 * fRec20[i-1]) + (fSlow0 * fRec21[i-1]));
				fYec1[(fYec1_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec21[i]));
				fRec20[i] = fYec1[(fYec1_idx+i-1557)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec20_perm[i]=fRec20_tmp[count+i];
			fYec1_idx_save = count;
			for (int i=0; i<4; i++) fRec21_perm[i]=fRec21_tmp[count+i];
			
			// LOOP 0x101353a50
			// pre processing
			for (int i=0; i<4; i++) fRec23_tmp[i]=fRec23_perm[i];
			fYec2_idx = (fYec2_idx+fYec2_idx_save)&4095;
			for (int i=0; i<4; i++) fRec22_tmp[i]=fRec22_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec23[i] = ((fSlow1 * fRec22[i-1]) + (fSlow0 * fRec23[i-1]));
				fYec2[(fYec2_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec23[i]));
				fRec22[i] = fYec2[(fYec2_idx+i-1491)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec22_perm[i]=fRec22_tmp[count+i];
			fYec2_idx_save = count;
			for (int i=0; i<4; i++) fRec23_perm[i]=fRec23_tmp[count+i];
			
			// LOOP 0x1013555d0
			// pre processing
			for (int i=0; i<4; i++) fRec25_tmp[i]=fRec25_perm[i];
			fYec3_idx = (fYec3_idx+fYec3_idx_save)&4095;
			for (int i=0; i<4; i++) fRec24_tmp[i]=fRec24_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec25[i] = ((fSlow1 * fRec24[i-1]) + (fSlow0 * fRec25[i-1]));
				fYec3[(fYec3_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec25[i]));
				fRec24[i] = fYec3[(fYec3_idx+i-1422)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec24_perm[i]=fRec24_tmp[count+i];
			fYec3_idx_save = count;
			for (int i=0; i<4; i++) fRec25_perm[i]=fRec25_tmp[count+i];
			
			// LOOP 0x101357120
			// pre processing
			for (int i=0; i<4; i++) fRec27_tmp[i]=fRec27_perm[i];
			fYec4_idx = (fYec4_idx+fYec4_idx_save)&4095;
			for (int i=0; i<4; i++) fRec26_tmp[i]=fRec26_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec27[i] = ((fSlow1 * fRec26[i-1]) + (fSlow0 * fRec27[i-1]));
				fYec4[(fYec4_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec27[i]));
				fRec26[i] = fYec4[(fYec4_idx+i-1356)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec26_perm[i]=fRec26_tmp[count+i];
			fYec4_idx_save = count;
			for (int i=0; i<4; i++) fRec27_perm[i]=fRec27_tmp[count+i];
			
			// LOOP 0x101358c70
			// pre processing
			for (int i=0; i<4; i++) fRec29_tmp[i]=fRec29_perm[i];
			fYec5_idx = (fYec5_idx+fYec5_idx_save)&4095;
			for (int i=0; i<4; i++) fRec28_tmp[i]=fRec28_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec29[i] = ((fSlow1 * fRec28[i-1]) + (fSlow0 * fRec29[i-1]));
				fYec5[(fYec5_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec29[i]));
				fRec28[i] = fYec5[(fYec5_idx+i-1277)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec28_perm[i]=fRec28_tmp[count+i];
			fYec5_idx_save = count;
			for (int i=0; i<4; i++) fRec29_perm[i]=fRec29_tmp[count+i];
			
			// LOOP 0x10135a7e0
			// pre processing
			for (int i=0; i<4; i++) fRec31_tmp[i]=fRec31_perm[i];
			fYec6_idx = (fYec6_idx+fYec6_idx_save)&4095;
			for (int i=0; i<4; i++) fRec30_tmp[i]=fRec30_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec31[i] = ((fSlow1 * fRec30[i-1]) + (fSlow0 * fRec31[i-1]));
				fYec6[(fYec6_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec31[i]));
				fRec30[i] = fYec6[(fYec6_idx+i-1188)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec30_perm[i]=fRec30_tmp[count+i];
			fYec6_idx_save = count;
			for (int i=0; i<4; i++) fRec31_perm[i]=fRec31_tmp[count+i];
			
			// LOOP 0x10135c330
			// pre processing
			for (int i=0; i<4; i++) fRec33_tmp[i]=fRec33_perm[i];
			fYec7_idx = (fYec7_idx+fYec7_idx_save)&4095;
			for (int i=0; i<4; i++) fRec32_tmp[i]=fRec32_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec33[i] = ((fSlow1 * fRec32[i-1]) + (fSlow0 * fRec33[i-1]));
				fYec7[(fYec7_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec33[i]));
				fRec32[i] = fYec7[(fYec7_idx+i-1116)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec32_perm[i]=fRec32_tmp[count+i];
			fYec7_idx_save = count;
			for (int i=0; i<4; i++) fRec33_perm[i]=fRec33_tmp[count+i];
			
			// LOOP 0x101364b70
			// pre processing
			for (int i=0; i<4; i++) fRec43_tmp[i]=fRec43_perm[i];
			fYec12_idx = (fYec12_idx+fYec12_idx_save)&4095;
			for (int i=0; i<4; i++) fRec42_tmp[i]=fRec42_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec43[i] = ((fSlow1 * fRec42[i-1]) + (fSlow0 * fRec43[i-1]));
				fYec12[(fYec12_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec43[i]));
				fRec42[i] = fYec12[(fYec12_idx+i-1640)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec42_perm[i]=fRec42_tmp[count+i];
			fYec12_idx_save = count;
			for (int i=0; i<4; i++) fRec43_perm[i]=fRec43_tmp[count+i];
			
			// LOOP 0x1013667b0
			// pre processing
			for (int i=0; i<4; i++) fRec45_tmp[i]=fRec45_perm[i];
			fYec13_idx = (fYec13_idx+fYec13_idx_save)&4095;
			for (int i=0; i<4; i++) fRec44_tmp[i]=fRec44_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec45[i] = ((fSlow1 * fRec44[i-1]) + (fSlow0 * fRec45[i-1]));
				fYec13[(fYec13_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec45[i]));
				fRec44[i] = fYec13[(fYec13_idx+i-1580)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec44_perm[i]=fRec44_tmp[count+i];
			fYec13_idx_save = count;
			for (int i=0; i<4; i++) fRec45_perm[i]=fRec45_tmp[count+i];
			
			// LOOP 0x101368330
			// pre processing
			for (int i=0; i<4; i++) fRec47_tmp[i]=fRec47_perm[i];
			fYec14_idx = (fYec14_idx+fYec14_idx_save)&4095;
			for (int i=0; i<4; i++) fRec46_tmp[i]=fRec46_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec47[i] = ((fSlow1 * fRec46[i-1]) + (fSlow0 * fRec47[i-1]));
				fYec14[(fYec14_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec47[i]));
				fRec46[i] = fYec14[(fYec14_idx+i-1514)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec46_perm[i]=fRec46_tmp[count+i];
			fYec14_idx_save = count;
			for (int i=0; i<4; i++) fRec47_perm[i]=fRec47_tmp[count+i];
			
			// LOOP 0x101369f40
			// pre processing
			for (int i=0; i<4; i++) fRec49_tmp[i]=fRec49_perm[i];
			fYec15_idx = (fYec15_idx+fYec15_idx_save)&4095;
			for (int i=0; i<4; i++) fRec48_tmp[i]=fRec48_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec49[i] = ((fSlow1 * fRec48[i-1]) + (fSlow0 * fRec49[i-1]));
				fYec15[(fYec15_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec49[i]));
				fRec48[i] = fYec15[(fYec15_idx+i-1445)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec48_perm[i]=fRec48_tmp[count+i];
			fYec15_idx_save = count;
			for (int i=0; i<4; i++) fRec49_perm[i]=fRec49_tmp[count+i];
			
			// LOOP 0x10136bae0
			// pre processing
			for (int i=0; i<4; i++) fRec51_tmp[i]=fRec51_perm[i];
			fYec16_idx = (fYec16_idx+fYec16_idx_save)&4095;
			for (int i=0; i<4; i++) fRec50_tmp[i]=fRec50_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec51[i] = ((fSlow1 * fRec50[i-1]) + (fSlow0 * fRec51[i-1]));
				fYec16[(fYec16_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec51[i]));
				fRec50[i] = fYec16[(fYec16_idx+i-1379)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec50_perm[i]=fRec50_tmp[count+i];
			fYec16_idx_save = count;
			for (int i=0; i<4; i++) fRec51_perm[i]=fRec51_tmp[count+i];
			
			// LOOP 0x10136d660
			// pre processing
			for (int i=0; i<4; i++) fRec53_tmp[i]=fRec53_perm[i];
			fYec17_idx = (fYec17_idx+fYec17_idx_save)&4095;
			for (int i=0; i<4; i++) fRec52_tmp[i]=fRec52_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec53[i] = ((fSlow1 * fRec52[i-1]) + (fSlow0 * fRec53[i-1]));
				fYec17[(fYec17_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec53[i]));
				fRec52[i] = fYec17[(fYec17_idx+i-1300)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec52_perm[i]=fRec52_tmp[count+i];
			fYec17_idx_save = count;
			for (int i=0; i<4; i++) fRec53_perm[i]=fRec53_tmp[count+i];
			
			// LOOP 0x10136f1e0
			// pre processing
			for (int i=0; i<4; i++) fRec55_tmp[i]=fRec55_perm[i];
			fYec18_idx = (fYec18_idx+fYec18_idx_save)&4095;
			for (int i=0; i<4; i++) fRec54_tmp[i]=fRec54_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec55[i] = ((fSlow1 * fRec54[i-1]) + (fSlow0 * fRec55[i-1]));
				fYec18[(fYec18_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec55[i]));
				fRec54[i] = fYec18[(fYec18_idx+i-1211)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec54_perm[i]=fRec54_tmp[count+i];
			fYec18_idx_save = count;
			for (int i=0; i<4; i++) fRec55_perm[i]=fRec55_tmp[count+i];
			
			// LOOP 0x101370d60
			// pre processing
			for (int i=0; i<4; i++) fRec57_tmp[i]=fRec57_perm[i];
			fYec19_idx = (fYec19_idx+fYec19_idx_save)&4095;
			for (int i=0; i<4; i++) fRec56_tmp[i]=fRec56_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec57[i] = ((fSlow1 * fRec56[i-1]) + (fSlow0 * fRec57[i-1]));
				fYec19[(fYec19_idx+i)&4095] = (fZec0[i] + (fSlow2 * fRec57[i]));
				fRec56[i] = fYec19[(fYec19_idx+i-1139)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec56_perm[i]=fRec56_tmp[count+i];
			fYec19_idx_save = count;
			for (int i=0; i<4; i++) fRec57_perm[i]=fRec57_tmp[count+i];
			
			// SECTION : 3
			// LOOP 0x10134f870
			// exec code
			for (int i=0; i<count; i++) {
				fZec1[i] = (((((((fRec32[i] + fRec30[i]) + fRec28[i]) + fRec26[i]) + fRec24[i]) + fRec22[i]) + fRec20[i]) + fRec18[i]);
			}
			
			// LOOP 0x101364a70
			// exec code
			for (int i=0; i<count; i++) {
				fZec3[i] = (((((((fRec56[i] + fRec54[i]) + fRec52[i]) + fRec50[i]) + fRec48[i]) + fRec46[i]) + fRec44[i]) + fRec42[i]);
			}
			
			// SECTION : 4
			// LOOP 0x10134f120
			// pre processing
			fYec8_idx = (fYec8_idx+fYec8_idx_save)&2047;
			for (int i=0; i<4; i++) fRec16_tmp[i]=fRec16_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec8[(fYec8_idx+i)&2047] = (fZec1[i] + (0.5f * fRec16[i-1]));
				fRec16[i] = fYec8[(fYec8_idx+i-556)&2047];
				fRec17[i] = (0 - (fZec1[i] - fRec16[i-1]));
			}
			// post processing
			for (int i=0; i<4; i++) fRec16_perm[i]=fRec16_tmp[count+i];
			fYec8_idx_save = count;
			
			// LOOP 0x101364320
			// pre processing
			fYec20_idx = (fYec20_idx+fYec20_idx_save)&2047;
			for (int i=0; i<4; i++) fRec40_tmp[i]=fRec40_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec20[(fYec20_idx+i)&2047] = (fZec3[i] + (0.5f * fRec40[i-1]));
				fRec40[i] = fYec20[(fYec20_idx+i-579)&2047];
				fRec41[i] = (0 - (fZec3[i] - fRec40[i-1]));
			}
			// post processing
			for (int i=0; i<4; i++) fRec40_perm[i]=fRec40_tmp[count+i];
			fYec20_idx_save = count;
			
			// SECTION : 5
			// LOOP 0x10134e9d0
			// pre processing
			fYec9_idx = (fYec9_idx+fYec9_idx_save)&2047;
			for (int i=0; i<4; i++) fRec14_tmp[i]=fRec14_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec9[(fYec9_idx+i)&2047] = (fRec17[i] + (0.5f * fRec14[i-1]));
				fRec14[i] = fYec9[(fYec9_idx+i-441)&2047];
				fRec15[i] = (fRec14[i-1] - fRec17[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec14_perm[i]=fRec14_tmp[count+i];
			fYec9_idx_save = count;
			
			// LOOP 0x101363bd0
			// pre processing
			fYec21_idx = (fYec21_idx+fYec21_idx_save)&2047;
			for (int i=0; i<4; i++) fRec38_tmp[i]=fRec38_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec21[(fYec21_idx+i)&2047] = (fRec41[i] + (0.5f * fRec38[i-1]));
				fRec38[i] = fYec21[(fYec21_idx+i-464)&2047];
				fRec39[i] = (fRec38[i-1] - fRec41[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec38_perm[i]=fRec38_tmp[count+i];
			fYec21_idx_save = count;
			
			// SECTION : 6
			// LOOP 0x10134e2a0
			// pre processing
			fYec10_idx = (fYec10_idx+fYec10_idx_save)&2047;
			for (int i=0; i<4; i++) fRec12_tmp[i]=fRec12_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec10[(fYec10_idx+i)&2047] = (fRec15[i] + (0.5f * fRec12[i-1]));
				fRec12[i] = fYec10[(fYec10_idx+i-341)&2047];
				fRec13[i] = (fRec12[i-1] - fRec15[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec12_perm[i]=fRec12_tmp[count+i];
			fYec10_idx_save = count;
			
			// LOOP 0x1013634a0
			// pre processing
			fYec22_idx = (fYec22_idx+fYec22_idx_save)&2047;
			for (int i=0; i<4; i++) fRec36_tmp[i]=fRec36_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec22[(fYec22_idx+i)&2047] = (fRec39[i] + (0.5f * fRec36[i-1]));
				fRec36[i] = fYec22[(fYec22_idx+i-364)&2047];
				fRec37[i] = (fRec36[i-1] - fRec39[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec36_perm[i]=fRec36_tmp[count+i];
			fYec22_idx_save = count;
			
			// SECTION : 7
			// LOOP 0x10134dba0
			// pre processing
			fYec11_idx = (fYec11_idx+fYec11_idx_save)&2047;
			for (int i=0; i<4; i++) fRec10_tmp[i]=fRec10_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec11[(fYec11_idx+i)&2047] = (fRec13[i] + (0.5f * fRec10[i-1]));
				fRec10[i] = fYec11[(fYec11_idx+i-225)&2047];
				fRec11[i] = (fRec10[i-1] - fRec13[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec10_perm[i]=fRec10_tmp[count+i];
			fYec11_idx_save = count;
			
			// LOOP 0x101362e30
			// pre processing
			fYec23_idx = (fYec23_idx+fYec23_idx_save)&2047;
			for (int i=0; i<4; i++) fRec34_tmp[i]=fRec34_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec23[(fYec23_idx+i)&2047] = (fRec37[i] + (0.5f * fRec34[i-1]));
				fRec34[i] = fYec23[(fYec23_idx+i-248)&2047];
				fRec35[i] = (fRec34[i-1] - fRec37[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec34_perm[i]=fRec34_tmp[count+i];
			fYec23_idx_save = count;
			
			// SECTION : 8
			// LOOP 0x10134daa0
			// exec code
			for (int i=0; i<count; i++) {
				fZec2[i] = ((fSlow4 * (float)input0[i]) + (fSlow3 * fRec11[i]));
			}
			
			// LOOP 0x101362d30
			// exec code
			for (int i=0; i<count; i++) {
				fZec4[i] = ((fSlow4 * (float)input1[i]) + (fSlow3 * fRec35[i]));
			}
			
			// SECTION : 9
			// LOOP 0x10134d9a0
			// exec code
			for (int i=0; i<count; i++) {
				fZec5[i] = (1.500000e-02f * (fZec4[i] + fZec2[i]));
			}
			
			// SECTION : 10
			// LOOP 0x10134b4b0
			// pre processing
			for (int i=0; i<4; i++) fRec9_tmp[i]=fRec9_perm[i];
			fYec24_idx = (fYec24_idx+fYec24_idx_save)&4095;
			for (int i=0; i<4; i++) fRec8_tmp[i]=fRec8_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec9[i] = ((fSlow1 * fRec8[i-1]) + (fSlow0 * fRec9[i-1]));
				fYec24[(fYec24_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec9[i]));
				fRec8[i] = fYec24[(fYec24_idx+i-1617)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec8_perm[i]=fRec8_tmp[count+i];
			fYec24_idx_save = count;
			for (int i=0; i<4; i++) fRec9_perm[i]=fRec9_tmp[count+i];
			
			// LOOP 0x101377dc0
			// pre processing
			for (int i=0; i<4; i++) fRec59_tmp[i]=fRec59_perm[i];
			fYec25_idx = (fYec25_idx+fYec25_idx_save)&4095;
			for (int i=0; i<4; i++) fRec58_tmp[i]=fRec58_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec59[i] = ((fSlow1 * fRec58[i-1]) + (fSlow0 * fRec59[i-1]));
				fYec25[(fYec25_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec59[i]));
				fRec58[i] = fYec25[(fYec25_idx+i-1557)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec58_perm[i]=fRec58_tmp[count+i];
			fYec25_idx_save = count;
			for (int i=0; i<4; i++) fRec59_perm[i]=fRec59_tmp[count+i];
			
			// LOOP 0x101379900
			// pre processing
			for (int i=0; i<4; i++) fRec61_tmp[i]=fRec61_perm[i];
			fYec26_idx = (fYec26_idx+fYec26_idx_save)&4095;
			for (int i=0; i<4; i++) fRec60_tmp[i]=fRec60_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec61[i] = ((fSlow1 * fRec60[i-1]) + (fSlow0 * fRec61[i-1]));
				fYec26[(fYec26_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec61[i]));
				fRec60[i] = fYec26[(fYec26_idx+i-1491)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec60_perm[i]=fRec60_tmp[count+i];
			fYec26_idx_save = count;
			for (int i=0; i<4; i++) fRec61_perm[i]=fRec61_tmp[count+i];
			
			// LOOP 0x10137b480
			// pre processing
			for (int i=0; i<4; i++) fRec63_tmp[i]=fRec63_perm[i];
			fYec27_idx = (fYec27_idx+fYec27_idx_save)&4095;
			for (int i=0; i<4; i++) fRec62_tmp[i]=fRec62_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec63[i] = ((fSlow1 * fRec62[i-1]) + (fSlow0 * fRec63[i-1]));
				fYec27[(fYec27_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec63[i]));
				fRec62[i] = fYec27[(fYec27_idx+i-1422)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec62_perm[i]=fRec62_tmp[count+i];
			fYec27_idx_save = count;
			for (int i=0; i<4; i++) fRec63_perm[i]=fRec63_tmp[count+i];
			
			// LOOP 0x10137d000
			// pre processing
			for (int i=0; i<4; i++) fRec65_tmp[i]=fRec65_perm[i];
			fYec28_idx = (fYec28_idx+fYec28_idx_save)&4095;
			for (int i=0; i<4; i++) fRec64_tmp[i]=fRec64_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec65[i] = ((fSlow1 * fRec64[i-1]) + (fSlow0 * fRec65[i-1]));
				fYec28[(fYec28_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec65[i]));
				fRec64[i] = fYec28[(fYec28_idx+i-1356)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec64_perm[i]=fRec64_tmp[count+i];
			fYec28_idx_save = count;
			for (int i=0; i<4; i++) fRec65_perm[i]=fRec65_tmp[count+i];
			
			// LOOP 0x10137eb80
			// pre processing
			for (int i=0; i<4; i++) fRec67_tmp[i]=fRec67_perm[i];
			fYec29_idx = (fYec29_idx+fYec29_idx_save)&4095;
			for (int i=0; i<4; i++) fRec66_tmp[i]=fRec66_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec67[i] = ((fSlow1 * fRec66[i-1]) + (fSlow0 * fRec67[i-1]));
				fYec29[(fYec29_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec67[i]));
				fRec66[i] = fYec29[(fYec29_idx+i-1277)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec66_perm[i]=fRec66_tmp[count+i];
			fYec29_idx_save = count;
			for (int i=0; i<4; i++) fRec67_perm[i]=fRec67_tmp[count+i];
			
			// LOOP 0x101380700
			// pre processing
			for (int i=0; i<4; i++) fRec69_tmp[i]=fRec69_perm[i];
			fYec30_idx = (fYec30_idx+fYec30_idx_save)&4095;
			for (int i=0; i<4; i++) fRec68_tmp[i]=fRec68_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec69[i] = ((fSlow1 * fRec68[i-1]) + (fSlow0 * fRec69[i-1]));
				fYec30[(fYec30_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec69[i]));
				fRec68[i] = fYec30[(fYec30_idx+i-1188)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec68_perm[i]=fRec68_tmp[count+i];
			fYec30_idx_save = count;
			for (int i=0; i<4; i++) fRec69_perm[i]=fRec69_tmp[count+i];
			
			// LOOP 0x101382280
			// pre processing
			for (int i=0; i<4; i++) fRec71_tmp[i]=fRec71_perm[i];
			fYec31_idx = (fYec31_idx+fYec31_idx_save)&4095;
			for (int i=0; i<4; i++) fRec70_tmp[i]=fRec70_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec71[i] = ((fSlow1 * fRec70[i-1]) + (fSlow0 * fRec71[i-1]));
				fYec31[(fYec31_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec71[i]));
				fRec70[i] = fYec31[(fYec31_idx+i-1116)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec70_perm[i]=fRec70_tmp[count+i];
			fYec31_idx_save = count;
			for (int i=0; i<4; i++) fRec71_perm[i]=fRec71_tmp[count+i];
			
			// LOOP 0x101389fc0
			// pre processing
			for (int i=0; i<4; i++) fRec81_tmp[i]=fRec81_perm[i];
			fYec36_idx = (fYec36_idx+fYec36_idx_save)&4095;
			for (int i=0; i<4; i++) fRec80_tmp[i]=fRec80_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec81[i] = ((fSlow1 * fRec80[i-1]) + (fSlow0 * fRec81[i-1]));
				fYec36[(fYec36_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec81[i]));
				fRec80[i] = fYec36[(fYec36_idx+i-1640)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec80_perm[i]=fRec80_tmp[count+i];
			fYec36_idx_save = count;
			for (int i=0; i<4; i++) fRec81_perm[i]=fRec81_tmp[count+i];
			
			// LOOP 0x10138bc10
			// pre processing
			for (int i=0; i<4; i++) fRec83_tmp[i]=fRec83_perm[i];
			fYec37_idx = (fYec37_idx+fYec37_idx_save)&4095;
			for (int i=0; i<4; i++) fRec82_tmp[i]=fRec82_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec83[i] = ((fSlow1 * fRec82[i-1]) + (fSlow0 * fRec83[i-1]));
				fYec37[(fYec37_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec83[i]));
				fRec82[i] = fYec37[(fYec37_idx+i-1580)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec82_perm[i]=fRec82_tmp[count+i];
			fYec37_idx_save = count;
			for (int i=0; i<4; i++) fRec83_perm[i]=fRec83_tmp[count+i];
			
			// LOOP 0x10138d7b0
			// pre processing
			for (int i=0; i<4; i++) fRec85_tmp[i]=fRec85_perm[i];
			fYec38_idx = (fYec38_idx+fYec38_idx_save)&4095;
			for (int i=0; i<4; i++) fRec84_tmp[i]=fRec84_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec85[i] = ((fSlow1 * fRec84[i-1]) + (fSlow0 * fRec85[i-1]));
				fYec38[(fYec38_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec85[i]));
				fRec84[i] = fYec38[(fYec38_idx+i-1514)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec84_perm[i]=fRec84_tmp[count+i];
			fYec38_idx_save = count;
			for (int i=0; i<4; i++) fRec85_perm[i]=fRec85_tmp[count+i];
			
			// LOOP 0x10138f330
			// pre processing
			for (int i=0; i<4; i++) fRec87_tmp[i]=fRec87_perm[i];
			fYec39_idx = (fYec39_idx+fYec39_idx_save)&4095;
			for (int i=0; i<4; i++) fRec86_tmp[i]=fRec86_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec87[i] = ((fSlow1 * fRec86[i-1]) + (fSlow0 * fRec87[i-1]));
				fYec39[(fYec39_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec87[i]));
				fRec86[i] = fYec39[(fYec39_idx+i-1445)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec86_perm[i]=fRec86_tmp[count+i];
			fYec39_idx_save = count;
			for (int i=0; i<4; i++) fRec87_perm[i]=fRec87_tmp[count+i];
			
			// LOOP 0x101390eb0
			// pre processing
			for (int i=0; i<4; i++) fRec89_tmp[i]=fRec89_perm[i];
			fYec40_idx = (fYec40_idx+fYec40_idx_save)&4095;
			for (int i=0; i<4; i++) fRec88_tmp[i]=fRec88_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec89[i] = ((fSlow1 * fRec88[i-1]) + (fSlow0 * fRec89[i-1]));
				fYec40[(fYec40_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec89[i]));
				fRec88[i] = fYec40[(fYec40_idx+i-1379)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec88_perm[i]=fRec88_tmp[count+i];
			fYec40_idx_save = count;
			for (int i=0; i<4; i++) fRec89_perm[i]=fRec89_tmp[count+i];
			
			// LOOP 0x101392a30
			// pre processing
			for (int i=0; i<4; i++) fRec91_tmp[i]=fRec91_perm[i];
			fYec41_idx = (fYec41_idx+fYec41_idx_save)&4095;
			for (int i=0; i<4; i++) fRec90_tmp[i]=fRec90_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec91[i] = ((fSlow1 * fRec90[i-1]) + (fSlow0 * fRec91[i-1]));
				fYec41[(fYec41_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec91[i]));
				fRec90[i] = fYec41[(fYec41_idx+i-1300)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec90_perm[i]=fRec90_tmp[count+i];
			fYec41_idx_save = count;
			for (int i=0; i<4; i++) fRec91_perm[i]=fRec91_tmp[count+i];
			
			// LOOP 0x1013945b0
			// pre processing
			for (int i=0; i<4; i++) fRec93_tmp[i]=fRec93_perm[i];
			fYec42_idx = (fYec42_idx+fYec42_idx_save)&4095;
			for (int i=0; i<4; i++) fRec92_tmp[i]=fRec92_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec93[i] = ((fSlow1 * fRec92[i-1]) + (fSlow0 * fRec93[i-1]));
				fYec42[(fYec42_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec93[i]));
				fRec92[i] = fYec42[(fYec42_idx+i-1211)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec92_perm[i]=fRec92_tmp[count+i];
			fYec42_idx_save = count;
			for (int i=0; i<4; i++) fRec93_perm[i]=fRec93_tmp[count+i];
			
			// LOOP 0x101396130
			// pre processing
			for (int i=0; i<4; i++) fRec95_tmp[i]=fRec95_perm[i];
			fYec43_idx = (fYec43_idx+fYec43_idx_save)&4095;
			for (int i=0; i<4; i++) fRec94_tmp[i]=fRec94_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fRec95[i] = ((fSlow1 * fRec94[i-1]) + (fSlow0 * fRec95[i-1]));
				fYec43[(fYec43_idx+i)&4095] = (fZec5[i] + (fSlow2 * fRec95[i]));
				fRec94[i] = fYec43[(fYec43_idx+i-1139)&4095];
			}
			// post processing
			for (int i=0; i<4; i++) fRec94_perm[i]=fRec94_tmp[count+i];
			fYec43_idx_save = count;
			for (int i=0; i<4; i++) fRec95_perm[i]=fRec95_tmp[count+i];
			
			// SECTION : 11
			// LOOP 0x10134b3b0
			// exec code
			for (int i=0; i<count; i++) {
				fZec6[i] = (((((((fRec70[i] + fRec68[i]) + fRec66[i]) + fRec64[i]) + fRec62[i]) + fRec60[i]) + fRec58[i]) + fRec8[i]);
			}
			
			// LOOP 0x101389ec0
			// exec code
			for (int i=0; i<count; i++) {
				fZec7[i] = (((((((fRec94[i] + fRec92[i]) + fRec90[i]) + fRec88[i]) + fRec86[i]) + fRec84[i]) + fRec82[i]) + fRec80[i]);
			}
			
			// SECTION : 12
			// LOOP 0x10134acd0
			// pre processing
			fYec32_idx = (fYec32_idx+fYec32_idx_save)&2047;
			for (int i=0; i<4; i++) fRec6_tmp[i]=fRec6_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec32[(fYec32_idx+i)&2047] = (fZec6[i] + (0.5f * fRec6[i-1]));
				fRec6[i] = fYec32[(fYec32_idx+i-556)&2047];
				fRec7[i] = (0 - (fZec6[i] - fRec6[i-1]));
			}
			// post processing
			for (int i=0; i<4; i++) fRec6_perm[i]=fRec6_tmp[count+i];
			fYec32_idx_save = count;
			
			// LOOP 0x1013897e0
			// pre processing
			fYec44_idx = (fYec44_idx+fYec44_idx_save)&2047;
			for (int i=0; i<4; i++) fRec78_tmp[i]=fRec78_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec44[(fYec44_idx+i)&2047] = (fZec7[i] + (0.5f * fRec78[i-1]));
				fRec78[i] = fYec44[(fYec44_idx+i-579)&2047];
				fRec79[i] = (0 - (fZec7[i] - fRec78[i-1]));
			}
			// post processing
			for (int i=0; i<4; i++) fRec78_perm[i]=fRec78_tmp[count+i];
			fYec44_idx_save = count;
			
			// SECTION : 13
			// LOOP 0x10134a5f0
			// pre processing
			fYec33_idx = (fYec33_idx+fYec33_idx_save)&2047;
			for (int i=0; i<4; i++) fRec4_tmp[i]=fRec4_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec33[(fYec33_idx+i)&2047] = (fRec7[i] + (0.5f * fRec4[i-1]));
				fRec4[i] = fYec33[(fYec33_idx+i-441)&2047];
				fRec5[i] = (fRec4[i-1] - fRec7[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec4_perm[i]=fRec4_tmp[count+i];
			fYec33_idx_save = count;
			
			// LOOP 0x101389100
			// pre processing
			fYec45_idx = (fYec45_idx+fYec45_idx_save)&2047;
			for (int i=0; i<4; i++) fRec76_tmp[i]=fRec76_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec45[(fYec45_idx+i)&2047] = (fRec79[i] + (0.5f * fRec76[i-1]));
				fRec76[i] = fYec45[(fYec45_idx+i-464)&2047];
				fRec77[i] = (fRec76[i-1] - fRec79[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec76_perm[i]=fRec76_tmp[count+i];
			fYec45_idx_save = count;
			
			// SECTION : 14
			// LOOP 0x101349f10
			// pre processing
			fYec34_idx = (fYec34_idx+fYec34_idx_save)&2047;
			for (int i=0; i<4; i++) fRec2_tmp[i]=fRec2_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec34[(fYec34_idx+i)&2047] = (fRec5[i] + (0.5f * fRec2[i-1]));
				fRec2[i] = fYec34[(fYec34_idx+i-341)&2047];
				fRec3[i] = (fRec2[i-1] - fRec5[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec2_perm[i]=fRec2_tmp[count+i];
			fYec34_idx_save = count;
			
			// LOOP 0x101388a20
			// pre processing
			fYec46_idx = (fYec46_idx+fYec46_idx_save)&2047;
			for (int i=0; i<4; i++) fRec74_tmp[i]=fRec74_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec46[(fYec46_idx+i)&2047] = (fRec77[i] + (0.5f * fRec74[i-1]));
				fRec74[i] = fYec46[(fYec46_idx+i-364)&2047];
				fRec75[i] = (fRec74[i-1] - fRec77[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec74_perm[i]=fRec74_tmp[count+i];
			fYec46_idx_save = count;
			
			// SECTION : 15
			// LOOP 0x101349700
			// pre processing
			fYec35_idx = (fYec35_idx+fYec35_idx_save)&2047;
			for (int i=0; i<4; i++) fRec0_tmp[i]=fRec0_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec35[(fYec35_idx+i)&2047] = (fRec3[i] + (0.5f * fRec0[i-1]));
				fRec0[i] = fYec35[(fYec35_idx+i-225)&2047];
				fRec1[i] = (fRec0[i-1] - fRec3[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec0_perm[i]=fRec0_tmp[count+i];
			fYec35_idx_save = count;
			
			// LOOP 0x1013883a0
			// pre processing
			fYec47_idx = (fYec47_idx+fYec47_idx_save)&2047;
			for (int i=0; i<4; i++) fRec72_tmp[i]=fRec72_perm[i];
			// exec code
			for (int i=0; i<count; i++) {
				fYec47[(fYec47_idx+i)&2047] = (fRec75[i] + (0.5f * fRec72[i-1]));
				fRec72[i] = fYec47[(fYec47_idx+i-248)&2047];
				fRec73[i] = (fRec72[i-1] - fRec75[i]);
			}
			// post processing
			for (int i=0; i<4; i++) fRec72_perm[i]=fRec72_tmp[count+i];
			fYec47_idx_save = count;
			
			// SECTION : 16
			// LOOP 0x101349600
			// exec code
			for (int i=0; i<count; i++) {
				output0[i] = (FAUSTFLOAT)((fSlow4 * fZec2[i]) + (fSlow3 * fRec1[i]));
			}
			
			// LOOP 0x1013881d0
			// exec code
			for (int i=0; i<count; i++) {
				output1[i] = (FAUSTFLOAT)((fSlow4 * fZec4[i]) + (fSlow3 * fRec73[i]));
			}
			
		}
	}
};
				
mydsp	DSP;

/******************************************************************************
*******************************************************************************

							COREAUDIO INTERFACE

*******************************************************************************
*******************************************************************************/

#define MAX_CHANNELS 256
#define OPEN_ERR -1
#define NO_ERR 0

class TiPhoneCoreAudioRenderer
{

    private:

		AudioUnit fAUHAL;
        
        int	fDevNumInChans;
        int	fDevNumOutChans;
     
        float* fInChannel[MAX_CHANNELS];
        float* fOutChannel[MAX_CHANNELS];
	
		static OSStatus Render(void *inRefCon,
                               AudioUnitRenderActionFlags *ioActionFlags,
                               const AudioTimeStamp *inTimeStamp,
                               UInt32 inBusNumber,
                               UInt32 inNumberFrames,
                               AudioBufferList *ioData);
                               
        static void InterruptionListener(void *inClientData, UInt32 inInterruption);

    public:

        TiPhoneCoreAudioRenderer(int input, int output)
            :fDevNumInChans(input), fDevNumOutChans(output)
        {
            for (int i = 0; i < fDevNumInChans; i++) {
                fInChannel[i] = new float[8192];
            }
    
            for (int i = 0; i < fDevNumOutChans; i++) {
                fOutChannel[i] = new float[8192];
            }
        }
        virtual ~TiPhoneCoreAudioRenderer()
        {
            for (int i = 0; i < fDevNumInChans; i++) {
                delete[] fInChannel[i];
            }
    
            for (int i = 0; i < fDevNumOutChans; i++) {
                delete[] fOutChannel[i]; 
            }
        }

        long Open(long bufferSize, long sampleRate);
        long Close();

        long Start();
        long Stop();

};

typedef TiPhoneCoreAudioRenderer * TiPhoneCoreAudioRendererPtr;

static void PrintStreamDesc(AudioStreamBasicDescription *inDesc)
{
    printf("- - - - - - - - - - - - - - - - - - - -\n");
    printf("  Sample Rate:%f\n", inDesc->mSampleRate);
    printf("  Format ID:%.*s\n", (int) sizeof(inDesc->mFormatID), (char*)&inDesc->mFormatID);
    printf("  Format Flags:%lX\n", inDesc->mFormatFlags);
    printf("  Bytes per Packet:%ld\n", inDesc->mBytesPerPacket);
    printf("  Frames per Packet:%ld\n", inDesc->mFramesPerPacket);
    printf("  Bytes per Frame:%ld\n", inDesc->mBytesPerFrame);
    printf("  Channels per Frame:%ld\n", inDesc->mChannelsPerFrame);
    printf("  Bits per Channel:%ld\n", inDesc->mBitsPerChannel);
    printf("- - - - - - - - - - - - - - - - - - - -\n");
}

static void printError(OSStatus err)
{
    switch (err) {
      	case kAudioConverterErr_FormatNotSupported:
            printf("error code : kAudioConverterErr_FormatNotSupported\n");
            break;
        case kAudioConverterErr_OperationNotSupported:
            printf("error code : kAudioConverterErr_OperationNotSupported\n");
            break;
        case kAudioConverterErr_PropertyNotSupported:
            printf("error code : kAudioConverterErr_PropertyNotSupported\n");
            break;
        case kAudioConverterErr_InvalidInputSize:
            printf("error code : kAudioConverterErr_InvalidInputSize\n");
            break;
        case kAudioConverterErr_InvalidOutputSize:
            printf("error code : kAudioConverterErr_InvalidOutputSize\n");
            break;
        case kAudioConverterErr_UnspecifiedError:
            printf("error code : kAudioConverterErr_UnspecifiedError\n");
            break;
        case kAudioConverterErr_BadPropertySizeError:
            printf("error code : kAudioConverterErr_BadPropertySizeError\n");
            break;
        case kAudioConverterErr_RequiresPacketDescriptionsError:
            printf("error code : kAudioConverterErr_RequiresPacketDescriptionsError\n");
            break;
        case kAudioConverterErr_InputSampleRateOutOfRange:
            printf("error code : kAudioConverterErr_InputSampleRateOutOfRange\n");
            break;
        case kAudioConverterErr_OutputSampleRateOutOfRange:
            printf("error code : kAudioConverterErr_OutputSampleRateOutOfRange\n");
            break;
        default:
            printf("error code : unknown\n");
            break;
    }
}

st::HardwareClock my_clock;

OSStatus TiPhoneCoreAudioRenderer::Render(void *inRefCon,
                                         AudioUnitRenderActionFlags *ioActionFlags,
                                         const AudioTimeStamp *inTimeStamp,
                                         UInt32,
                                         UInt32 inNumberFrames,
                                         AudioBufferList *ioData)
{
    TiPhoneCoreAudioRendererPtr renderer = (TiPhoneCoreAudioRendererPtr)inRefCon;
    my_clock.Update();
    //printf("TiPhoneCoreAudioRenderer::Render 0 %d\n", inNumberFrames);
    
    AudioUnitRender(renderer->fAUHAL, ioActionFlags, inTimeStamp, 1, inNumberFrames, ioData);
    
    float coef = 1.f/32768.f;
    /*
    for (int chan = 0; chan < fDevNumInChans; chan++) {
        for (int frame = 0; frame < inNumberFrames; frame++) {
            fInChannel[chan][frame] = float(((long*)ioData->mBuffers[chan].mData)[frame]) / 32768.f;
            fInChannel[chan][frame] = float(((long*)ioData->mBuffers[chan].mData)[frame]) / 32768.f;
        }
    }
    */
    
    for (int frame = 0; frame < inNumberFrames; frame++) {
        float sample = float(((long*)ioData->mBuffers[0].mData)[frame]) * coef;
        renderer->fInChannel[0][frame] = sample;
        renderer->fInChannel[1][frame] = sample;
    }
    
    //printf("TiPhoneCoreAudioRenderer::Render 1  %d\n", inNumberFrames);
       
    DSP.compute((int)inNumberFrames, renderer->fInChannel, renderer->fOutChannel);
    
    for (int chan = 0; chan < renderer->fDevNumOutChans; chan++) {
        for (int frame = 0; frame < inNumberFrames; frame++) {
           ((long*)ioData->mBuffers[chan].mData)[frame] = long(renderer->fOutChannel[chan][frame] * 32768.f); 
        }
    }
    
    my_clock.Update();
    const float dt = my_clock.GetDeltaTime();
    printf("Normal: %f s\n", dt);
    
    //printf("TiPhoneCoreAudioRenderer::Render 3  %d\n", inNumberFrames);
	return 0;
}

void TiPhoneCoreAudioRenderer::InterruptionListener(void *inClientData, UInt32 inInterruption)
{
	printf("Session interrupted! --- %s ---", inInterruption == kAudioSessionBeginInterruption ? "Begin Interruption" : "End Interruption");
	
	TiPhoneCoreAudioRenderer *obj = (TiPhoneCoreAudioRenderer*)inClientData;
	
	if (inInterruption == kAudioSessionEndInterruption) {
		// make sure we are again the active session
		AudioSessionSetActive(true);
		AudioOutputUnitStart(obj->fAUHAL);
	}
	
	if (inInterruption == kAudioSessionBeginInterruption) {
		AudioOutputUnitStop(obj->fAUHAL);
    }
}

long TiPhoneCoreAudioRenderer::Open(long bufferSize, long samplerate)
{
    OSStatus err1;
    UInt32 outSize;
    UInt32 enableIO;
	Boolean isWritable;
	AudioStreamBasicDescription srcFormat, dstFormat;
    
    printf("Open fDevNumInChans = %ld fDevNumOutChans = %ld bufferSize = %ld samplerate = %ld\n", fDevNumInChans, fDevNumOutChans, bufferSize, samplerate);
	  
    // Initialize and configure the audio session
    err1 = AudioSessionInitialize(NULL, NULL, InterruptionListener, this);
    if (err1 != noErr) {
        printf("Couldn't initialize audio session\n");
        printError(err1);
        return OPEN_ERR;
    }
    
    err1 = AudioSessionSetActive(true);
    if (err1 != noErr) {
        printf("Couldn't set audio session active\n");
        printError(err1);
        return OPEN_ERR;
    }
  			
    UInt32 audioCategory = kAudioSessionCategory_PlayAndRecord;
	err1 = AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(audioCategory), &audioCategory);
    if (err1 != noErr) {
        printf("Couldn't set audio category\n");
        printError(err1);
        return OPEN_ERR;
    }
    
   	//err1 = AudioSessionAddPropertyListener(kAudioSessionProperty_AudioRouteChange, propListener, self), "couldn't set property listener");
    
    Float64 hwSampleRate;
    outSize = sizeof(hwSampleRate);
	err1 = AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareSampleRate, &outSize, &hwSampleRate);
    if (err1 != noErr) {
        printf("Couldn't get hw sample rate\n");
        printError(err1);
        return OPEN_ERR;
    } else {
         printf("Get hw sample rate %f\n", hwSampleRate);
    }
    
    Float32 hwBufferSize;
    outSize = sizeof(hwBufferSize);
	err1 = AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareIOBufferDuration, &outSize, &hwBufferSize);
    if (err1 != noErr) {
        printf("Couldn't get hw buffer duration\n");
        printError(err1);
        return OPEN_ERR;
    } else {
         printf("Get hw buffer duration %f\n", hwBufferSize);
    }

    UInt32 hwInput;
    outSize = sizeof(hwInput);
	err1 = AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareInputNumberChannels, &outSize, &hwInput);
    if (err1 != noErr) {
        printf("Couldn't get hw input channels\n");
        printError(err1);
        return OPEN_ERR;
    } else {
        printf("Get hw input channels %d\n", hwInput);
    }

    UInt32 hwOutput;
    outSize = sizeof(hwOutput);
	err1 = AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareOutputNumberChannels, &outSize, &hwOutput);
    if (err1 != noErr) {
        printf("Couldn't get hw output channels\n");
        printError(err1);
        return OPEN_ERR;
    } else {
        printf("Get hw output channels %d\n", hwOutput);
    }
			
    Float32 preferredBufferSize = float(bufferSize) / float(samplerate);
    printf("preferredBufferSize %f \n", preferredBufferSize);
    
	err1 = AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration, sizeof(preferredBufferSize), &preferredBufferSize);
    if (err1 != noErr) {
        printf("Couldn't set i/o buffer duration\n");
        printError(err1);
        return OPEN_ERR;
    }
  	
    Float64 preferredSamplerate = float(samplerate);
	err1 = AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareSampleRate, sizeof(preferredSamplerate), &preferredSamplerate);
    if (err1 != noErr) {
        printf("Couldn't set i/o sample rate\n");
        printError(err1);
        return OPEN_ERR;
    }
  	
    // AUHAL
    AudioComponentDescription cd = {kAudioUnitType_Output, kAudioUnitSubType_RemoteIO, kAudioUnitManufacturer_Apple, 0, 0};
    AudioComponent HALOutput = AudioComponentFindNext(NULL, &cd);

    err1 = AudioComponentInstanceNew(HALOutput, &fAUHAL);
    if (err1 != noErr) {
		printf("Error calling OpenAComponent\n");
        printError(err1);
        goto error;
	}

    enableIO = 1;
    err1 = AudioUnitSetProperty(fAUHAL, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, 0, &enableIO, sizeof(enableIO));
    if (err1 != noErr) {
        printf("Error calling AudioUnitSetProperty - kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output\n");
        printError(err1);
        goto error;
    }
    
    enableIO = 1;
    err1 = AudioUnitSetProperty(fAUHAL, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, 1, &enableIO, sizeof(enableIO));
    if (err1 != noErr) {
        printf("Error calling AudioUnitSetProperty - kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input\n");
        printError(err1);
        goto error;
    }
    
      
    UInt32 maxFPS;
    outSize = sizeof(maxFPS);
    err1 = AudioUnitGetProperty(fAUHAL, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &maxFPS, &outSize);
    if (err1 != noErr) {
        printf("Couldn't get kAudioUnitProperty_MaximumFramesPerSlice\n");
        printError(err1);
        goto error;
    } else {
        printf("Get kAudioUnitProperty_MaximumFramesPerSlice %d\n", maxFPS);
    }
   
    err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 1, (UInt32*)&bufferSize, sizeof(UInt32));
    if (err1 != noErr) {
        printf("Error calling AudioUnitSetProperty - kAudioUnitProperty_MaximumFramesPerSlice\n");
        printError(err1);
        goto error;
    }

    err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, (UInt32*)&bufferSize, sizeof(UInt32));
    if (err1 != noErr) {
        printf("Error calling AudioUnitSetProperty - kAudioUnitProperty_MaximumFramesPerSlice\n");
        printError(err1);
        goto error;
    }


    err1 = AudioUnitInitialize(fAUHAL);
    if (err1 != noErr) {
		printf("Cannot initialize AUHAL unit\n");
		printError(err1);
        goto error;
	}

    // Setting format
  	
    if (fDevNumInChans > 0) {
        outSize = sizeof(AudioStreamBasicDescription);
        err1 = AudioUnitGetProperty(fAUHAL, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &srcFormat, &outSize);
        if (err1 != noErr) {
            printf("Error calling AudioUnitGetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Output\n");
            printError(err1);
        }
        PrintStreamDesc(&srcFormat);
        
        srcFormat.mFormatID = kAudioFormatLinearPCM;
        srcFormat.mFormatFlags = kAudioFormatFlagsCanonical | kLinearPCMFormatFlagIsNonInterleaved;
        srcFormat.mBytesPerPacket = sizeof(AudioUnitSampleType);
        srcFormat.mFramesPerPacket = 1;
        srcFormat.mBytesPerFrame = sizeof(AudioUnitSampleType);
        srcFormat.mChannelsPerFrame = fDevNumInChans;
        srcFormat.mBitsPerChannel = 32;
        
        PrintStreamDesc(&srcFormat);
        
        err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &srcFormat, sizeof(AudioStreamBasicDescription));
        if (err1 != noErr) {
            printf("Error calling AudioUnitSetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Output\n");
            printError(err1);
        }    
        
    }
	
    if (fDevNumOutChans > 0) {
        outSize = sizeof(AudioStreamBasicDescription);
        err1 = AudioUnitGetProperty(fAUHAL, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &dstFormat, &outSize);
        if (err1 != noErr) {
            printf("Error calling AudioUnitGetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Input\n");
            printError(err1);
        }
        PrintStreamDesc(&dstFormat);
        
        dstFormat.mFormatID = kAudioFormatLinearPCM;
        dstFormat.mFormatFlags = kAudioFormatFlagsCanonical | kLinearPCMFormatFlagIsNonInterleaved;
        dstFormat.mBytesPerPacket = sizeof(AudioUnitSampleType);
        dstFormat.mFramesPerPacket = 1;
        dstFormat.mBytesPerFrame = sizeof(AudioUnitSampleType);
        dstFormat.mChannelsPerFrame = fDevNumOutChans;
        dstFormat.mBitsPerChannel = 32;
        
        PrintStreamDesc(&dstFormat);

        err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &dstFormat, sizeof(AudioStreamBasicDescription));
        if (err1 != noErr) {
            printf("Error calling AudioUnitSetProperty - kAudioUnitProperty_StreamFormat kAudioUnitScope_Input\n");
            printError(err1);
        }
    }

    if (fDevNumInChans > 0 && fDevNumOutChans == 0) {
        AURenderCallbackStruct output;
        output.inputProc = Render;
        output.inputProcRefCon = this;
        err1 = AudioUnitSetProperty(fAUHAL, kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, 0, &output, sizeof(output));
        if (err1 != noErr) {
            printf("Error calling AudioUnitSetProperty - kAudioUnitProperty_SetRenderCallback 1\n");
            printError(err1);
            goto error;
        }
    } else {
        AURenderCallbackStruct output;
        output.inputProc = Render;
        output.inputProcRefCon = this;
        err1 = AudioUnitSetProperty(fAUHAL, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &output, sizeof(output));
        if (err1 != noErr) {
            printf("Error calling AudioUnitSetProperty - kAudioUnitProperty_SetRenderCallback 0\n");
            printError(err1);
            goto error;
        }
    }

    return NO_ERR;

error:
    AudioUnitUninitialize(fAUHAL);
    AudioComponentInstanceDispose(fAUHAL);
    return OPEN_ERR;
}

long TiPhoneCoreAudioRenderer::Close()
{
 	AudioUnitUninitialize(fAUHAL);
    AudioComponentInstanceDispose(fAUHAL);
    return NO_ERR;
}

long TiPhoneCoreAudioRenderer::Start()
{
    AudioSessionSetActive(true);
	OSStatus err = AudioOutputUnitStart(fAUHAL);
  
    if (err != noErr) {
        printf("Error while opening device : device open error \n");
        return OPEN_ERR;
    } else {
        return NO_ERR;
	}
}

long TiPhoneCoreAudioRenderer::Stop()
{
    OSStatus err = AudioOutputUnitStop(fAUHAL);

    if (err != noErr) {
        printf("Error while closing device : device close error \n");
        return OPEN_ERR;
    } else {
        return NO_ERR;
	}
}

/******************************************************************************
*******************************************************************************

								MAIN PLAY THREAD

*******************************************************************************
*******************************************************************************/

long lopt(char *argv[], const char *name, long def) 
{
	int	i;
	for (i = 0; argv[i]; i++) if (!strcmp(argv[i], name)) return atoi(argv[i + 1]);
	return def;
}
	
//-------------------------------------------------------------------------
// 									MAIN
//-------------------------------------------------------------------------

int main(int argc, char *argv[])
{
	UI* interface = new CMDUI(argc, argv);
    TiPhoneCoreAudioRenderer audio_device(DSP.getNumInputs(), DSP.getNumOutputs());
		
    long srate = (long)lopt(argv, "--frequency", 44100);
    int	fpb = lopt(argv, "--buffer", 512);
 		
	DSP.init(long(srate));
	DSP.buildUserInterface(interface);
	
    if (audio_device.Open(fpb, srate) < 0) {
        printf("Cannot open CoreAudio device\n");
        return 0;
    }
    
    if (audio_device.Start() < 0) {
        printf("Cannot start CoreAudio device\n");
        return 0;
    }
    
    printf("inchan = %d, outchan = %d, freq = %ld\n", DSP.getNumInputs(), DSP.getNumOutputs(), srate);
	interface->run();
    
    audio_device.Stop();
    audio_device.Close();
  	return 0;
}

