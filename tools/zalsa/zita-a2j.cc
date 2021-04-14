// ----------------------------------------------------------------------------
//
//  Copyright (C) 2012 Fons Adriaensen <fons@linuxaudio.org>
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include "alsathread.h"
#include "jackclient.h"
#include "lfqueue.h"

static const char *clopt = "hvLSj:d:r:p:n:c:Q:I:";

static void help (void)
{
    fprintf (stderr, "\n%s-%s\n", APPNAME, VERSION);
    fprintf (stderr, "(C) 2012-2018 Fons Adriaensen  <fons@linuxaudio.org>\n");
    fprintf (stderr, "Use ALSA capture device as a Jack client.\n\n");
    fprintf (stderr, "Usage: %s <options>\n", APPNAME);
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "  -h                 Display this text\n");
    fprintf (stderr, "  -j <jackname>      Name as Jack client [%s]\n", APPNAME);
    fprintf (stderr, "  -d <device>        ALSA capture device [none]\n");  
    fprintf (stderr, "  -r <rate>          Sample rate [48000]\n");
    fprintf (stderr, "  -p <period>        Period size [256]\n");   
    fprintf (stderr, "  -n <nfrags>        Number of fragments [2]\n");   
    fprintf (stderr, "  -c <nchannels>     Number of channels [2]\n");
    fprintf (stderr, "  -S                 Word clock sync, no resampling\n");
    fprintf (stderr, "  -Q <quality>       Resampling quality, 16..96 [auto]\n");
    fprintf (stderr, "  -I <samples>       Latency adjustment [0]\n");
    fprintf (stderr, "  -L                 Force 16-bit and 2 channels [off]\n");
    fprintf (stderr, "  -v                 Print tracing information [off]\n");
}

class zita_a2j
{
	Lfq_int32 *commq;
	Lfq_adata *alsaq;
	Lfq_jdata *infoq;
	Lfq_audio *audioq;
	bool stop;
	bool v_opt;
	bool L_opt;
	bool S_opt;
	char *jname;
	char *device;
	int fsamp;
	int bsize;
	int nfrag;
	int nchan;
	int rqual;
	int ltcor;

public:

    zita_a2j()
    {
        commq = new Lfq_int32(16);
        alsaq = new Lfq_adata(256);
        infoq = new Lfq_jdata(256);
        audioq = 0;
        stop = false;
        v_opt = false;
        L_opt = false;
        S_opt = false;
        jname = strdup(APPNAME);
        device = 0;
        fsamp = 48000;
        bsize = 128;
        nfrag = 2;
        nchan = 2;
        rqual = 0;
        ltcor = 0;
        A = 0;
        C = 0;
        J = 0;
    }

private:

    int procoptions (int ac, const char *av [])
    {
        int k;
        
        optind = 1;
        opterr = 0;
        while ((k = getopt (ac, (char **) av, (char *) clopt)) != -1)
        {
            if (optarg && (*optarg == '-'))
            {
                fprintf (stderr, "  Missing argument for '-%c' option.\n", k); 
                fprintf (stderr, "  Use '-h' to see all options.\n");
                return 1;
            }
            switch (k)
            {
            case 'h' : help (); return 1;
            case 'v' : v_opt = true; break;
            case 'L' : L_opt = true; break;
            case 'S' : S_opt = true; break;
            case 'j' : jname = optarg; break;
            case 'd' : device = optarg; break;
            case 'r' : fsamp = atoi (optarg); break;    
            case 'p' : bsize = atoi (optarg); break;    
            case 'n' : nfrag = atoi (optarg); break;    
            case 'c' : nchan = atoi (optarg); break;    
            case 'Q' : rqual = atoi (optarg); break;    
            case 'I' : ltcor = atoi (optarg); break;    
            case '?':
                if (optopt != ':' && strchr (clopt, optopt))
                {
                    fprintf (stderr, "  Missing argument for '-%c' option.\n", optopt); 
                }
                else if (isprint (optopt))
                {
                    fprintf (stderr, "  Unknown option '-%c'.\n", optopt);
                }
                else
                {
                    fprintf (stderr, "  Unknown option character '0x%02x'.\n", optopt & 255);
                }
                fprintf (stderr, "  Use '-h' to see all options.\n");
                return 1;
            default:
                return 1;
            }
        }
        return 0;
    }

    int parse_options (const char* load_init)
    {
        int argsz;
        int argc = 0;
        const char** argv;
        char* args = strdup (load_init);
        char* token;
        char* ptr = args;
        char* savep;

	    if (!load_init) {
            return 0;
        }

        argsz = 8; /* random guess at "maxargs" */
        argv = (const char **) malloc (sizeof (char *) * argsz);

        argv[argc++] = APPNAME;

        while (1) {

            if ((token = strtok_r (ptr, " ", &savep)) == NULL) {
                break;
            }

            if (argc == argsz) {
                argsz *= 2;
                argv = (const char **) realloc (argv, sizeof (char *) * argsz);
            }

            argv[argc++] = token;
            ptr = NULL;
        }

        return procoptions (argc, argv);
    }

    void printinfo (void)
    {
        int     n, k;
        double  e, r;
        Jdata   *J;

        n = 0;
        k = 99999;
        e = r = 0;
        while (infoq->rd_avail ())
        {
            J = infoq->rd_datap ();
            if (J->_state == Jackclient::TERM)
            {
                printf ("Fatal error condition, terminating.\n");
                stop = true;
                return;
            }
            else if (J->_state == Jackclient::WAIT)
            {
                printf ("Detected excessive timing errors, waiting 10 seconds.\n");
                n = 0;
            }
            else if (J->_state == Jackclient::SYNC0)
            {
                printf ("Starting synchronisation.\n");
            }
            else if (v_opt)
            {
                n++;
                e += J->_error;
                r += J->_ratio;
                if (J->_bstat < k) k = J->_bstat;
            }
            infoq->rd_commit ();
        }
        if (n) printf ("%8.3lf %10.6lf %5d\n", e / n, r / n, k);
    }


    Alsa_pcmi      *A;
    Alsathread     *C;
    Jackclient     *J;

public:

    int
    jack_initialize (jack_client_t* client, const char* load_init)
    {
        int            k, k_del, opts;
        double         t_jack;
        double         t_alsa;
        double         t_del;
        
        if (parse_options (load_init)) {
                fprintf (stderr, "parse options failed\n");
                return 1;
        }

        if (device == 0)
        {
            help ();
            return 1;
        }
        if (rqual < 16) rqual = 16;
        if (rqual > 96) rqual = 96;
        if ((fsamp < 8000) || (bsize < 16) || (nfrag < 2) || (nchan < 1))
        {
            fprintf (stderr, "Illegal parameter value(s).\n");
            return 1;
        }

        opts = 0;
        if (v_opt) opts |= Alsa_pcmi::DEBUG_ALL;
        if (L_opt) opts |= Alsa_pcmi::FORCE_16B | Alsa_pcmi::FORCE_2CH;
        A = new Alsa_pcmi (0, device, 0, fsamp, bsize, nfrag, opts);
        if (A->state ())
        {
            fprintf (stderr, "Can't open ALSA capture device '%s'.\n", device);
            return 1;
        }
        if (v_opt) A->printinfo ();
        if (nchan > A->ncapt ())
        {
            nchan = A->ncapt ();
            fprintf (stderr, "Warning: only %d channels are available.\n", nchan);
        }
        C = new Alsathread (A, Alsathread::CAPT);
        J = new Jackclient (client, 0, Jackclient::CAPT, nchan, S_opt, this);
        usleep (100000);

        t_alsa = (double) bsize / fsamp;
        if (t_alsa < 1e-3) t_alsa = 1e-3;
        t_jack = (double) J->bsize () / J->fsamp (); 
        t_del = t_alsa + t_jack;
        k_del = (int)(t_del * fsamp);
        for (k = 256; k < 2 * k_del; k *= 2);
        audioq = new Lfq_audio (k, nchan);

        if (rqual == 0)
        {
            k = (fsamp < J->fsamp ()) ? fsamp : J->fsamp ();
            if (k < 44100) k = 44100;
            rqual = (int)((6.7 * k) / (k - 38000));
        }
        if (rqual < 16) rqual = 16;
        if (rqual > 96) rqual = 96;

        C->start (audioq, commq, alsaq, J->rprio () + 10);
        J->start (audioq, commq, alsaq, infoq, J->fsamp () / (double) fsamp, k_del, ltcor, rqual);

        return 0;
    }

    void jack_finish (void* arg)
    {
        commq->wr_int32 (Alsathread::TERM);
        usleep (100000);
        delete C;
        delete A;
        delete J;
        delete audioq;
    }
};

extern "C" {

int
jack_initialize (jack_client_t* client, const char* load_init)
{
	zita_a2j *c = new zita_a2j();
	c->jack_initialize(client, load_init);
	return 0;
}

void jack_finish (void* arg)
{
	if (!arg) return;
	Jackclient *J = (Jackclient *)arg;
	zita_a2j *c = (zita_a2j *)J->getarg();
	c->jack_finish(arg);
	delete c;
}

} /* extern "C" */
