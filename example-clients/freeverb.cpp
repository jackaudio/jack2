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
#include <jack/jack.h>

#include <windows.h>

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
                //sleep(1);
                Sleep(1000);
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
        float fslider0;
        float R0_0;
        int iota0;
        float dline0[225];
        float R1_0;
        int iota1;
        float dline1[341];
        float R2_0;
        int iota2;
        float dline2[441];
        float R3_0;
        int iota3;
        float dline3[556];
        float R4_0;
        int iota4;
        float dline4[1617];
        float fslider1;
        float R5_0;
        float fslider2;
        float R6_0;
        int iota5;
        float dline5[1557];
        float R7_0;
        float R8_0;
        int iota6;
        float dline6[1491];
        float R9_0;
        float R10_0;
        int iota7;
        float dline7[1422];
        float R11_0;
        float R12_0;
        int iota8;
        float dline8[1277];
        float R13_0;
        float R14_0;
        int iota9;
        float dline9[1116];
        float R15_0;
        float R16_0;
        int iota10;
        float dline10[1188];
        float R17_0;
        float R18_0;
        int iota11;
        float dline11[1356];
        float R19_0;
        float R3_1;
        float R2_1;
        float R1_1;
        float R0_1;
        float R20_0;
        int iota12;
        float dline12[248];
        float R21_0;
        int iota13;
        float dline13[364];
        float R22_0;
        int iota14;
        float dline14[464];
        float R23_0;
        int iota15;
        float dline15[579];
        float R24_0;
        int iota16;
        float dline16[1640];
        float R25_0;
        float R26_0;
        int iota17;
        float dline17[1580];
        float R27_0;
        float R28_0;
        int iota18;
        float dline18[1514];
        float R29_0;
        float R30_0;
        int iota19;
        float dline19[1445];
        float R31_0;
        float R32_0;
        int iota20;
        float dline20[1300];
        float R33_0;
        float R34_0;
        int iota21;
        float dline21[1139];
        float R35_0;
        float R36_0;
        int iota22;
        float dline22[1211];
        float R37_0;
        float R38_0;
        int iota23;
        float dline23[1379];
        float R39_0;
        float R23_1;
        float R22_1;
        float R21_1;
        float R20_1;
    public:
        virtual int getNumInputs()
        {
            return 2;
        }
        virtual int getNumOutputs()
        {
            return 2;
        }
        virtual void init(int samplingFreq)
        {
            int i;
            fSamplingFreq = samplingFreq;
            fslider0 = 0.333300f;
            R0_0 = 0.0;
            iota0 = 0;
            for (i = 0; i < 225; i++)
                dline0[i] = 0.0;
            R1_0 = 0.0;
            iota1 = 0;
            for (i = 0; i < 341; i++)
                dline1[i] = 0.0;
            R2_0 = 0.0;
            iota2 = 0;
            for (i = 0; i < 441; i++)
                dline2[i] = 0.0;
            R3_0 = 0.0;
            iota3 = 0;
            for (i = 0; i < 556; i++)
                dline3[i] = 0.0;
            R4_0 = 0.0;
            iota4 = 0;
            for (i = 0; i < 1617; i++)
                dline4[i] = 0.0;
            fslider1 = 0.9500000f;
            R5_0 = 0.0;
            fslider2 = 0.9500000f;
            R6_0 = 0.0;
            iota5 = 0;
            for (i = 0; i < 1557; i++)
                dline5[i] = 0.0;
            R7_0 = 0.0;
            R8_0 = 0.0;
            iota6 = 0;
            for (i = 0; i < 1491; i++)
                dline6[i] = 0.0;
            R9_0 = 0.0;
            R10_0 = 0.0;
            iota7 = 0;
            for (i = 0; i < 1422; i++)
                dline7[i] = 0.0;
            R11_0 = 0.0;
            R12_0 = 0.0;
            iota8 = 0;
            for (i = 0; i < 1277; i++)
                dline8[i] = 0.0;
            R13_0 = 0.0;
            R14_0 = 0.0;
            iota9 = 0;
            for (i = 0; i < 1116; i++)
                dline9[i] = 0.0;
            R15_0 = 0.0;
            R16_0 = 0.0;
            iota10 = 0;
            for (i = 0; i < 1188; i++)
                dline10[i] = 0.0;
            R17_0 = 0.0;
            R18_0 = 0.0;
            iota11 = 0;
            for (i = 0; i < 1356; i++)
                dline11[i] = 0.0;
            R19_0 = 0.0;
            R3_1 = 0.0;
            R2_1 = 0.0;
            R1_1 = 0.0;
            R0_1 = 0.0;
            R20_0 = 0.0;
            iota12 = 0;
            for (i = 0; i < 248; i++)
                dline12[i] = 0.0;
            R21_0 = 0.0;
            iota13 = 0;
            for (i = 0; i < 364; i++)
                dline13[i] = 0.0;
            R22_0 = 0.0;
            iota14 = 0;
            for (i = 0; i < 464; i++)
                dline14[i] = 0.0;
            R23_0 = 0.0;
            iota15 = 0;
            for (i = 0; i < 579; i++)
                dline15[i] = 0.0;
            R24_0 = 0.0;
            iota16 = 0;
            for (i = 0; i < 1640; i++)
                dline16[i] = 0.0;
            R25_0 = 0.0;
            R26_0 = 0.0;
            iota17 = 0;
            for (i = 0; i < 1580; i++)
                dline17[i] = 0.0;
            R27_0 = 0.0;
            R28_0 = 0.0;
            iota18 = 0;
            for (i = 0; i < 1514; i++)
                dline18[i] = 0.0;
            R29_0 = 0.0;
            R30_0 = 0.0;
            iota19 = 0;
            for (i = 0; i < 1445; i++)
                dline19[i] = 0.0;
            R31_0 = 0.0;
            R32_0 = 0.0;
            iota20 = 0;
            for (i = 0; i < 1300; i++)
                dline20[i] = 0.0;
            R33_0 = 0.0;
            R34_0 = 0.0;
            iota21 = 0;
            for (i = 0; i < 1139; i++)
                dline21[i] = 0.0;
            R35_0 = 0.0;
            R36_0 = 0.0;
            iota22 = 0;
            for (i = 0; i < 1211; i++)
                dline22[i] = 0.0;
            R37_0 = 0.0;
            R38_0 = 0.0;
            iota23 = 0;
            for (i = 0; i < 1379; i++)
                dline23[i] = 0.0;
            R39_0 = 0.0;
            R23_1 = 0.0;
            R22_1 = 0.0;
            R21_1 = 0.0;
            R20_1 = 0.0;
        }

        virtual void buildUserInterface(UI* inter)
        {
            inter->openVerticalBox("Freeverb");
            inter->addHorizontalSlider("Damp", &fslider2, 0.500000f, 0.000000f, 1.000000f, 2.500000e-02f);
            inter->addHorizontalSlider("RoomSize", &fslider1, 0.500000f, 0.000000f, 1.000000f, 2.500000e-02f);
            inter->addHorizontalSlider("Wet", &fslider0, 0.333300f, 0.000000f, 1.000000f, 2.500000e-02f);
            inter->closeBox();
        }
        virtual void compute (int count, float** input, float** output)
        {
            float* input0;
            input0 = input[0];
            float* input1;
            input1 = input[1];
            float* output0;
            output0 = output[0];
            float* output1;
            output1 = output[1];
            float ftemp0 = fslider0;
            float ftemp1 = (1 - ftemp0);
            float ftemp5 = (0.700000f + (0.280000f * fslider1));
            float ftemp6 = (0.400000f * fslider2);
            float ftemp7 = (1 - ftemp6);
            for (int i = 0; i < count; i++) {
                float ftemp2 = input0[i];
                if (++iota0 == 225)
                    iota0 = 0;
                float T0 = dline0[iota0];
                if (++iota1 == 341)
                    iota1 = 0;
                float T1 = dline1[iota1];
                if (++iota2 == 441)
                    iota2 = 0;
                float T2 = dline2[iota2];
                if (++iota3 == 556)
                    iota3 = 0;
                float T3 = dline3[iota3];
                if (++iota4 == 1617)
                    iota4 = 0;
                float T4 = dline4[iota4];
                float ftemp3 = input1[i];
                float ftemp4 = (1.500000e-02f * (ftemp2 + ftemp3));
                R5_0 = ((ftemp7 * R4_0) + (ftemp6 * R5_0));
                dline4[iota4] = (ftemp4 + (ftemp5 * R5_0));
                R4_0 = T4;
                if (++iota5 == 1557)
                    iota5 = 0;
                float T5 = dline5[iota5];
                R7_0 = ((ftemp7 * R6_0) + (ftemp6 * R7_0));
                dline5[iota5] = (ftemp4 + (ftemp5 * R7_0));
                R6_0 = T5;
                if (++iota6 == 1491)
                    iota6 = 0;
                float T6 = dline6[iota6];
                R9_0 = ((ftemp7 * R8_0) + (ftemp6 * R9_0));
                dline6[iota6] = (ftemp4 + (ftemp5 * R9_0));
                R8_0 = T6;
                if (++iota7 == 1422)
                    iota7 = 0;
                float T7 = dline7[iota7];
                R11_0 = ((ftemp7 * R10_0) + (ftemp6 * R11_0));
                dline7[iota7] = (ftemp4 + (ftemp5 * R11_0));
                R10_0 = T7;
                if (++iota8 == 1277)
                    iota8 = 0;
                float T8 = dline8[iota8];
                R13_0 = ((ftemp7 * R12_0) + (ftemp6 * R13_0));
                dline8[iota8] = (ftemp4 + (ftemp5 * R13_0));
                R12_0 = T8;
                if (++iota9 == 1116)
                    iota9 = 0;
                float T9 = dline9[iota9];
                R15_0 = ((ftemp7 * R14_0) + (ftemp6 * R15_0));
                dline9[iota9] = (ftemp4 + (ftemp5 * R15_0));
                R14_0 = T9;
                if (++iota10 == 1188)
                    iota10 = 0;
                float T10 = dline10[iota10];
                R17_0 = ((ftemp7 * R16_0) + (ftemp6 * R17_0));
                dline10[iota10] = (ftemp4 + (ftemp5 * R17_0));
                R16_0 = T10;
                if (++iota11 == 1356)
                    iota11 = 0;
                float T11 = dline11[iota11];
                R19_0 = ((ftemp7 * R18_0) + (ftemp6 * R19_0));
                dline11[iota11] = (ftemp4 + (ftemp5 * R19_0));
                R18_0 = T11;
                float ftemp8 = (R16_0 + R18_0);
                dline3[iota3] = ((((0.500000f * R3_0) + R4_0) + (R6_0 + R8_0)) + ((R10_0 + R12_0) + (R14_0 + ftemp8)));
                float R3temp0 = T3;
                float R3temp1 = (R3_0 - (((R4_0 + R6_0) + (R8_0 + R10_0)) + ((R12_0 + R14_0) + ftemp8)));
                R3_0 = R3temp0;
                R3_1 = R3temp1;
                dline2[iota2] = ((0.500000f * R2_0) + R3_1);
                float R2temp0 = T2;
                float R2temp1 = (R2_0 - R3_1);
                R2_0 = R2temp0;
                R2_1 = R2temp1;
                dline1[iota1] = ((0.500000f * R1_0) + R2_1);
                float R1temp0 = T1;
                float R1temp1 = (R1_0 - R2_1);
                R1_0 = R1temp0;
                R1_1 = R1temp1;
                dline0[iota0] = ((0.500000f * R0_0) + R1_1);
                float R0temp0 = T0;
                float R0temp1 = (R0_0 - R1_1);
                R0_0 = R0temp0;
                R0_1 = R0temp1;
                output0[i] = ((ftemp1 * ftemp2) + (ftemp0 * R0_1));
                if (++iota12 == 248)
                    iota12 = 0;
                float T12 = dline12[iota12];
                if (++iota13 == 364)
                    iota13 = 0;
                float T13 = dline13[iota13];
                if (++iota14 == 464)
                    iota14 = 0;
                float T14 = dline14[iota14];
                if (++iota15 == 579)
                    iota15 = 0;
                float T15 = dline15[iota15];
                if (++iota16 == 1640)
                    iota16 = 0;
                float T16 = dline16[iota16];
                R25_0 = ((ftemp7 * R24_0) + (ftemp6 * R25_0));
                dline16[iota16] = (ftemp4 + (ftemp5 * R25_0));
                R24_0 = T16;
                if (++iota17 == 1580)
                    iota17 = 0;
                float T17 = dline17[iota17];
                R27_0 = ((ftemp7 * R26_0) + (ftemp6 * R27_0));
                dline17[iota17] = (ftemp4 + (ftemp5 * R27_0));
                R26_0 = T17;
                if (++iota18 == 1514)
                    iota18 = 0;
                float T18 = dline18[iota18];
                R29_0 = ((ftemp7 * R28_0) + (ftemp6 * R29_0));
                dline18[iota18] = (ftemp4 + (ftemp5 * R29_0));
                R28_0 = T18;
                if (++iota19 == 1445)
                    iota19 = 0;
                float T19 = dline19[iota19];
                R31_0 = ((ftemp7 * R30_0) + (ftemp6 * R31_0));
                dline19[iota19] = (ftemp4 + (ftemp5 * R31_0));
                R30_0 = T19;
                if (++iota20 == 1300)
                    iota20 = 0;
                float T20 = dline20[iota20];
                R33_0 = ((ftemp7 * R32_0) + (ftemp6 * R33_0));
                dline20[iota20] = (ftemp4 + (ftemp5 * R33_0));
                R32_0 = T20;
                if (++iota21 == 1139)
                    iota21 = 0;
                float T21 = dline21[iota21];
                R35_0 = ((ftemp7 * R34_0) + (ftemp6 * R35_0));
                dline21[iota21] = (ftemp4 + (ftemp5 * R35_0));
                R34_0 = T21;
                if (++iota22 == 1211)
                    iota22 = 0;
                float T22 = dline22[iota22];
                R37_0 = ((ftemp7 * R36_0) + (ftemp6 * R37_0));
                dline22[iota22] = (ftemp4 + (ftemp5 * R37_0));
                R36_0 = T22;
                if (++iota23 == 1379)
                    iota23 = 0;
                float T23 = dline23[iota23];
                R39_0 = ((ftemp7 * R38_0) + (ftemp6 * R39_0));
                dline23[iota23] = (ftemp4 + (ftemp5 * R39_0));
                R38_0 = T23;
                float ftemp9 = (R36_0 + R38_0);
                dline15[iota15] = ((((0.500000f * R23_0) + R24_0) + (R26_0 + R28_0)) + ((R30_0 + R32_0) + (R34_0 + ftemp9)));
                float R23temp0 = T15;
                float R23temp1 = (R23_0 - (((R24_0 + R26_0) + (R28_0 + R30_0)) + ((R32_0 + R34_0) + ftemp9)));
                R23_0 = R23temp0;
                R23_1 = R23temp1;
                dline14[iota14] = ((0.500000f * R22_0) + R23_1);
                float R22temp0 = T14;
                float R22temp1 = (R22_0 - R23_1);
                R22_0 = R22temp0;
                R22_1 = R22temp1;
                dline13[iota13] = ((0.500000f * R21_0) + R22_1);
                float R21temp0 = T13;
                float R21temp1 = (R21_0 - R22_1);
                R21_0 = R21temp0;
                R21_1 = R21temp1;
                dline12[iota12] = ((0.500000f * R20_0) + R21_1);
                float R20temp0 = T12;
                float R20temp1 = (R20_0 - R21_1);
                R20_0 = R20temp0;
                R20_1 = R20temp1;
                output1[i] = ((ftemp1 * ftemp3) + (ftemp0 * R20_1));
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
    int i;
    for (i = 0; i < gNumInChans; i++) {
        gInChannel[i] = (float *)jack_port_get_buffer(input_ports[i], nframes);
    }
    for (i = 0; i < gNumOutChans; i++) {
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
    int i;

    CMDUI* inter = new CMDUI(argc, argv);
    DSP.buildUserInterface(inter);

    //_snprintf(jackname, 255, "%s", basename(argv[0]));
    _snprintf(jackname, 255, "%s", "freeverb");

    if ((client = jack_client_new(jackname)) == 0) {
        fprintf(stderr, "jack server not running?\n");
        return 1;
    }

    jack_set_process_callback(client, process, 0);

    jack_set_sample_rate_callback(client, srate, 0);

    jack_on_shutdown(client, jack_shutdown, 0);

    gNumInChans = DSP.getNumInputs();
    gNumOutChans = DSP.getNumOutputs();

    for (i = 0; i < gNumInChans; i++) {
        char buf[256];
        _snprintf(buf, 256, "in_%d", i);
        input_ports[i] = jack_port_register(client, buf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    }
    for (i = 0; i < gNumOutChans; i++) {
        char buf[256];
        _snprintf(buf, 256, "out_%d", i);
        output_ports[i] = jack_port_register(client, buf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    }

    DSP.init(jack_get_sample_rate(client));
    DSP.buildUserInterface(inter);

    inter->process_command();

    physicalInPorts = (char **)jack_get_ports(client, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
    physicalOutPorts = (char **)jack_get_ports(client, NULL, NULL, JackPortIsPhysical | JackPortIsOutput);

    if (jack_activate(client)) {
        fprintf(stderr, "cannot activate client");
        return 1;
    }

    /*
    if (physicalOutPorts != NULL) {
    	for (int i = 0; i < gNumInChans && physicalOutPorts[i]; i++) {
    		jack_connect(client, physicalOutPorts[i], jack_port_name(input_ports[i]));
    	}
    }
    */

    if (physicalInPorts != NULL) {
        for (int i = 0; i < gNumOutChans && physicalInPorts[i]; i++) {
            jack_connect(client, jack_port_name(output_ports[i]), physicalInPorts[i]);
        }
    }

    /*
    jack_connect(client, "AudioPlayer:out1", jack_port_name(input_ports[0]));
    jack_connect(client, "AudioPlayer:out1", jack_port_name(input_ports[1]));

    jack_connect(client, "AudioPlayer:out2", jack_port_name(input_ports[0]));
    jack_connect(client, "AudioPlayer:out2", jack_port_name(input_ports[1]));
    */

    jack_connect(client, "JackRouter:out1", jack_port_name(input_ports[0]));
    jack_connect(client, "JackRouter:out2", jack_port_name(input_ports[1]));


    inter->run();

    jack_deactivate(client);

    for (i = 0; i < gNumInChans; i++) {
        jack_port_unregister(client, input_ports[i]);
    }
    for (i = 0; i < gNumOutChans; i++) {
        jack_port_unregister(client, output_ports[i]);
    }

    jack_client_close(client);

    return 0;
}
