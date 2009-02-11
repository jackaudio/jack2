/*
Copyright (C) 2008 Grame & RTL

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software 
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include "JackEngineProfiling.h"
#include "JackGraphManager.h"
#include "JackClientControl.h"
#include "JackClientInterface.h"
#include "JackTime.h"

namespace Jack
{

JackEngineProfiling::JackEngineProfiling():fAudioCycle(0)
{
    jack_info("Engine profiling activated, beware %ld MBytes are needed to record profiling points...", sizeof(fProfileTable) / (1024 * 1024));
    
    // Force memory page in
    memset(fProfileTable, 0, sizeof(fProfileTable));
}

JackEngineProfiling::~JackEngineProfiling()
{
    // Window monitoring
    int max_client = 0;
    char buffer[1024];
    char* nameTable[CLIENT_NUM];
    FILE* file = fopen("JackEngineProfiling.log", "w");
    
    jack_info("Write server and clients timing data...");

    if (file == NULL) {
        jack_error("JackEngineProfiling::Save cannot open JackEngineProfiling.log file");
    } else {
  
        for (int i = 2; i < TIME_POINTS; i++) {
            bool header = true;
            bool printed = false;
            int count = 0;
            for (int j = REAL_REFNUM; j < CLIENT_NUM; j++) {
                if (fProfileTable[i].fClientTable[j].fRefNum > 0) {
                    long d1 = long(fProfileTable[i - 1].fCurCycleBegin - fProfileTable[i - 2].fCurCycleBegin);
                    long d2 = long(fProfileTable[i].fPrevCycleEnd - fProfileTable[i - 1].fCurCycleBegin);
                    if (d1 > 0 && fProfileTable[i].fClientTable[j].fStatus != NotTriggered) {  // Valid cycle
                        count++;
                        nameTable[count] = fNameTable[fProfileTable[i].fClientTable[j].fRefNum];
                        
                        // driver delta and end cycle
                        if (header) {
                            fprintf(file, "%ld \t %ld \t", d1, d2);
                            header = false;
                        }
                        long d5 = long(fProfileTable[i].fClientTable[j].fSignaledAt - fProfileTable[i - 1].fCurCycleBegin);
                        long d6 = long(fProfileTable[i].fClientTable[j].fAwakeAt - fProfileTable[i - 1].fCurCycleBegin);
                        long d7 = long(fProfileTable[i].fClientTable[j].fFinishedAt - fProfileTable[i - 1].fCurCycleBegin);
                        
                        // ref, signal, start, end, scheduling, duration, status
                        fprintf(file, "%d \t %ld \t %ld \t  %ld \t %ld \t  %ld \t %d \t", 
                                fProfileTable[i].fClientTable[j].fRefNum,
                                ((d5 > 0) ? d5 : 0),
                                ((d6 > 0) ? d6 : 0),
                                ((d7 > 0) ? d7 : 0),
                                ((d6 > 0 && d5 > 0) ? (d6 - d5) : 0),
                                ((d7 > 0 && d6 > 0) ? (d7 - d6) : 0),
                                fProfileTable[i].fClientTable[j].fStatus);
                        printed = true;
                    }
                }
                max_client = (count > max_client) ? count : max_client;
            }
            if (printed) {
                fprintf(file, "\n");
            } else if (fProfileTable[i].fAudioCycle > 0) {  // Driver timing only
                long d1 = long(fProfileTable[i].fCurCycleBegin - fProfileTable[i - 1].fCurCycleBegin);
                long d2 = long(fProfileTable[i].fPrevCycleEnd - fProfileTable[i - 1].fCurCycleBegin);
                if (d1 > 0) {  // Valid cycle
                    fprintf(file, "%ld \t %ld \n", d1, d2);
                }
            }
        }
        fclose(file);
    }
  
    // Driver period
    file = fopen("Timing1.plot", "w");

    if (file == NULL) {
        jack_error("JackEngineProfiling::Save cannot open Timing1.log file");
    } else {
        
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Audio driver timing\"\n");
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"usec\"\n");
        fprintf(file, "plot \"JackEngineProfiling.log\" using 1 title \"Audio period\" with lines \n");
    
        fprintf(file, "set output 'Timing1.pdf\n");
        fprintf(file, "set terminal pdf\n");
    
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Audio driver timing\"\n");
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"usec\"\n");
        fprintf(file, "plot \"JackEngineProfiling.log\" using 1 title \"Audio period\" with lines \n");
    
        fclose(file);
    }
    
    // Driver end date
    file = fopen("Timing2.plot", "w");

    if (file == NULL) {
        jack_error("JackEngineProfiling::Save cannot open Timing2.log file");
    } else {
   
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Driver end date\"\n");
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"usec\"\n");
        fprintf(file, "plot  \"JackEngineProfiling.log\" using 2 title \"Driver end date\" with lines \n");
    
        fprintf(file, "set output 'Timing2.pdf\n");
        fprintf(file, "set terminal pdf\n");
    
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Driver end date\"\n");
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"usec\"\n");
        fprintf(file, "plot  \"JackEngineProfiling.log\" using 2 title \"Driver end date\" with lines \n");
    
        fclose(file);
    }
   
    // Clients end date
    if (max_client > 0) {
        file = fopen("Timing3.plot", "w");
        if (file == NULL) {
            jack_error("JackEngineProfiling::Save cannot open Timing3.log file");
        } else {
        
            fprintf(file, "set multiplot\n");
            fprintf(file, "set grid\n");
            fprintf(file, "set title \"Clients end date\"\n");
            fprintf(file, "set xlabel \"audio cycles\"\n");
            fprintf(file, "set ylabel \"usec\"\n");
        
            fprintf(file, "plot ");
            for (int i = 0; i < max_client; i++) {
                if (i == 0) {
                    if ((i + 1) == max_client) {
                        sprintf(buffer, "\"JackEngineProfiling.log\" using 1 title \"Audio period\" with lines,\"JackEngineProfiling.log\" using %d title \"%s\" with lines", 
                        ((i + 1) * 7) - 1 , nameTable[(i + 1)]);
                    } else {
                        sprintf(buffer, "\"JackEngineProfiling.log\" using 1 title \"Audio period\" with lines,\"JackEngineProfiling.log\" using %d title \"%s\" with lines,", 
                        ((i + 1) * 7) - 1 , nameTable[(i + 1)]);
                    }
                } else if ((i + 1) == max_client) { // Last client
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines", ((i + 1) * 7) - 1 , nameTable[(i + 1)]);
                } else {
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines,", ((i + 1) * 7) - 1, nameTable[(i + 1)]);
                }
                fprintf(file, buffer);
            }
        
            fprintf(file, "\n unset multiplot\n");  
            fprintf(file, "set output 'Timing3.pdf\n");
            fprintf(file, "set terminal pdf\n");
        
            fprintf(file, "set multiplot\n");
            fprintf(file, "set grid\n");
            fprintf(file, "set title \"Clients end date\"\n");
            fprintf(file, "set xlabel \"audio cycles\"\n");
            fprintf(file, "set ylabel \"usec\"\n");
            fprintf(file, "plot ");
            for (int i = 0; i < max_client; i++) {
                if (i == 0) {
                    if ((i + 1) == max_client) {
                        sprintf(buffer, "\"JackEngineProfiling.log\" using 1 title \"Audio period\" with lines,\"JackEngineProfiling.log\" using %d title \"%s\" with lines", 
                        ((i + 1) * 7) - 1 , nameTable[(i + 1)]);
                    } else {
                        sprintf(buffer, "\"JackEngineProfiling.log\" using 1 title \"Audio period\" with lines,\"JackEngineProfiling.log\" using %d title \"%s\" with lines,", 
                        ((i + 1) * 7) - 1 , nameTable[(i + 1)]);
                    }
                } else if ((i + 1) == max_client) { // Last client
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines", ((i + 1) * 7) - 1 , nameTable[(i + 1)]);
                } else {
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines,", ((i + 1) * 7) - 1, nameTable[(i + 1)]);
                }
                fprintf(file, buffer);
            }
            fclose(file);
        }
    }
    
    // Clients scheduling
    if (max_client > 0) {
        file = fopen("Timing4.plot", "w");

        if (file == NULL) {
            jack_error("JackEngineProfiling::Save cannot open Timing4.log file");
        } else {
        
            fprintf(file, "set multiplot\n");
            fprintf(file, "set grid\n");
            fprintf(file, "set title \"Clients scheduling latency\"\n");
            fprintf(file, "set xlabel \"audio cycles\"\n");
            fprintf(file, "set ylabel \"usec\"\n");
            fprintf(file, "plot ");
            for (int i = 0; i < max_client; i++) {
                if ((i + 1) == max_client) // Last client
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines", ((i + 1) * 7), nameTable[(i + 1)]);
                else
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines,", ((i + 1) * 7), nameTable[(i + 1)]);
                fprintf(file, buffer);
            }
            
            fprintf(file, "\n unset multiplot\n");  
            fprintf(file, "set output 'Timing4.pdf\n");
            fprintf(file, "set terminal pdf\n");
            
            fprintf(file, "set multiplot\n");
            fprintf(file, "set grid\n");
            fprintf(file, "set title \"Clients scheduling latency\"\n");
            fprintf(file, "set xlabel \"audio cycles\"\n");
            fprintf(file, "set ylabel \"usec\"\n");
            fprintf(file, "plot ");
            for (int i = 0; i < max_client; i++) {
                if ((i + 1) == max_client) // Last client
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines", ((i + 1) * 7), nameTable[(i + 1)]);
                else
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines,", ((i + 1) * 7), nameTable[(i + 1)]);
                fprintf(file, buffer);
            }
            fclose(file);
        }
    }
    
     // Clients duration
    if (max_client > 0) {
        file = fopen("Timing5.plot", "w");

        if (file == NULL) {
            jack_error("JackEngineProfiling::Save cannot open Timing5.log file");
        } else {
        
            fprintf(file, "set multiplot\n");
            fprintf(file, "set grid\n");
            fprintf(file, "set title \"Clients duration\"\n");
            fprintf(file, "set xlabel \"audio cycles\"\n");
            fprintf(file, "set ylabel \"usec\"\n");
            fprintf(file, "plot ");
            for (int i = 0; i < max_client; i++) {
                if ((i + 1) == max_client) // Last client
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines", ((i + 1) * 7) + 1, nameTable[(i + 1)]);
                else
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines,", ((i + 1) * 7) + 1, nameTable[(i + 1)]);
                fprintf(file, buffer);
            }
            
            fprintf(file, "\n unset multiplot\n");  
            fprintf(file, "set output 'Timing5.pdf\n");
            fprintf(file, "set terminal pdf\n");
            
            fprintf(file, "set multiplot\n");
            fprintf(file, "set grid\n");
            fprintf(file, "set title \"Clients duration\"\n");
            fprintf(file, "set xlabel \"audio cycles\"\n");
            fprintf(file, "set ylabel \"usec\"\n");
            fprintf(file, "plot ");
            for (int i = 0; i < max_client; i++) {
                if ((i + 1) == max_client) // Last client
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines", ((i + 1) * 7) + 1, nameTable[(i + 1)]);
                else
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines,", ((i + 1) * 7) + 1, nameTable[(i + 1)]);
                fprintf(file, buffer);
            }
            fclose(file);
        }
    }
}

void JackEngineProfiling::Profile(JackClientInterface** table, 
                                   JackGraphManager* manager, 
                                   jack_time_t period_usecs,
                                   jack_time_t cur_cycle_begin, 
                                   jack_time_t prev_cycle_end)
{
    fAudioCycle = (fAudioCycle + 1) % TIME_POINTS;
  
    // Keeps cycle data
    fProfileTable[fAudioCycle].fPeriodUsecs = period_usecs;
    fProfileTable[fAudioCycle].fCurCycleBegin = cur_cycle_begin;
    fProfileTable[fAudioCycle].fPrevCycleEnd = prev_cycle_end;
    fProfileTable[fAudioCycle].fAudioCycle = fAudioCycle;

    for (int i = REAL_REFNUM; i < CLIENT_NUM; i++) {
        JackClientInterface* client = table[i];
        JackClientTiming* timing = manager->GetClientTiming(i);
        if (client && client->GetClientControl()->fActive) {
            strcpy(fNameTable[i], client->GetClientControl()->fName);
            fProfileTable[fAudioCycle].fClientTable[i].fRefNum = i;
            fProfileTable[fAudioCycle].fClientTable[i].fSignaledAt = timing->fSignaledAt;
            fProfileTable[fAudioCycle].fClientTable[i].fAwakeAt = timing->fAwakeAt;
            fProfileTable[fAudioCycle].fClientTable[i].fFinishedAt = timing->fFinishedAt;
            fProfileTable[fAudioCycle].fClientTable[i].fStatus = timing->fStatus;
        }
    }
}

JackTimingMeasure* JackEngineProfiling::GetCurMeasure()
{
    return &fProfileTable[fAudioCycle];
}
    
} // end of namespace
