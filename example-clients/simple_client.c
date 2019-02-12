/** @file simple_client.c
 *
 * @brief This simple client demonstrates the basic features of JACK
 * as they would be used by many applications.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <jack/jack.h>


#include <pthread.h>
#include <time.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/errno.h>

#define Q_NAME "/tsq"
#define Q_MSG_SIZE 10
#define TABLE_SIZE   (200)


jack_port_t *output_port1, *output_port2;
jack_client_t *client;
pthread_t writerThread;
mqd_t tsq;
char msg_send[Q_MSG_SIZE];

#ifndef M_PI
#define M_PI  (3.14159265)
#endif


typedef struct
{
	float sine[TABLE_SIZE];
	int left_phase;
	int right_phase;
}
paTestData;



static void signal_handler(int sig)
{
	jack_client_close(client);
	fprintf(stderr, "signal received, exiting ...\n");

//	pthread_kill(writerThread, -9);

	if( mq_close(tsq) < 0) {
	}

	exit(0);
}

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client follows a simple rule: when the JACK transport is
 * running, copy the input port to the output.  When it stops, exit.
 */

void *worker_thread_listener_fileWriter()
{
	struct timespec tim;
    FILE* filepointer;

	tim.tv_sec = 0;
	tim.tv_nsec = 300000;

	if( ! (filepointer = fopen("client_ts.log", "w")) ){
		printf("Error Opening file %d\n", errno);
		pthread_exit((void*)-1);
	}


    fprintf(filepointer, "Started Filewriter Thread\n");fflush(filepointer);

	mqd_t tsq2 = mq_open(Q_NAME, O_RDWR | O_NONBLOCK);
    char msg_recv[Q_MSG_SIZE];


    while(1){

        if ( mq_receive(tsq2, msg_recv, Q_MSG_SIZE, NULL) > 0) {
    		fprintf(filepointer, "%s\n",msg_recv);fflush(filepointer);
        } else {
            if(errno != EAGAIN){
                fprintf(filepointer, "recv error %d %s %s\n", errno, strerror(errno), msg_recv);fflush(filepointer);
            }
        }
        nanosleep(&tim , NULL);
    }
    fclose(filepointer);
}


int
process (jack_nframes_t nframes, void *arg)
{

	jack_default_audio_sample_t *out1, *out2;
	paTestData *data = (paTestData*)arg;
	int i;


    struct timespec sys_time;
	memset((void*)&sys_time, 0, sizeof(struct timespec));

    if (clock_gettime(CLOCK_REALTIME, &sys_time)) {
//        fprintf(filepointer, " Clockrealtime Error\n");fflush(filepointer);
    }


	out1 = (jack_default_audio_sample_t*)jack_port_get_buffer (output_port1, nframes);
	out2 = (jack_default_audio_sample_t*)jack_port_get_buffer (output_port2, nframes);

	for( i=0; i<nframes; i++ )
	{
		out1[i] = data->sine[data->left_phase];  /* left */
		out2[i] = data->sine[data->right_phase];  /* right */
		data->left_phase += 1;
		if( data->left_phase >= TABLE_SIZE ) data->left_phase -= TABLE_SIZE;
		data->right_phase += 3; /* higher pitch so we can distinguish left and right. */
		if( data->right_phase >= TABLE_SIZE ) data->right_phase -= TABLE_SIZE;
	}

	memset(msg_send, 0, Q_MSG_SIZE);
	sprintf (msg_send, "%lld", (sys_time.tv_sec*1000000000ULL + sys_time.tv_nsec));

	if (mq_send(tsq, msg_send, Q_MSG_SIZE, 0) < 0) {
//		fprintf(filepointer, "send error %d %s %s\n", errno, strerror(errno), msg_send);fflush(filepointer);
	}

	return 0;
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void
jack_shutdown (void *arg)
{
	exit (1);
}

int
main (int argc, char *argv[])
{
	const char **ports;
	const char *client_name;
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;
	paTestData data;
	int i;

	if (argc >= 2) {		/* client name specified? */
		client_name = argv[1];
		if (argc >= 3) {	/* server name specified? */
			server_name = argv[2];
			int my_option = JackNullOption | JackServerName;
			options = (jack_options_t)my_option;
		}
	} else {			/* use basename of argv[0] */
		client_name = strrchr(argv[0], '/');
		if (client_name == 0) {
			client_name = argv[0];
		} else {
			client_name++;
		}
	}

	struct mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = 1000;
	attr.mq_msgsize = Q_MSG_SIZE;
	attr.mq_curmsgs = 0;


    if( mq_unlink(Q_NAME) < 0) {
        printf("unlink %s error %d %s\n", Q_NAME, errno, strerror(errno));fflush(stdout);
    } else {
         printf("unlink %s success\n", Q_NAME );fflush(stdout);
    }

	if ((tsq = mq_open(Q_NAME, O_RDWR | O_CREAT | O_NONBLOCK | O_EXCL, 0666, &attr)) == -1)  {
		printf("create error %s %d %s\n", Q_NAME, errno, strerror(errno));fflush(stdout);
	} else {
        printf("create success %s\n", Q_NAME);fflush(stdout);
	}

//    if( pthread_create( &writerThread, NULL, (&worker_thread_listener_fileWriter), NULL) != 0 ) {
//        printf("Error creating thread\n");fflush(stdout);
//    }




	for( i=0; i<TABLE_SIZE; i++ )
	{
		data.sine[i] = 0.2 * (float) sin( ((double)i/(double)TABLE_SIZE) * M_PI * 2. );
	}
	data.left_phase = data.right_phase = 0;


	/* open a client connection to the JACK server */

	client = jack_client_open (client_name, options, &status, server_name);
	if (client == NULL) {
		fprintf (stderr, "jack_client_open() failed, "
			 "status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
		exit (1);
	}
	if (status & JackServerStarted) {
		fprintf (stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name(client);
		fprintf (stderr, "unique name `%s' assigned\n", client_name);
	}

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/

	jack_set_process_callback (client, process, &data);

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/

	jack_on_shutdown (client, jack_shutdown, 0);

	/* create two ports */

	output_port1 = jack_port_register (client, "output1",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput, 0);

	output_port2 = jack_port_register (client, "output2",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput, 0);

	if ((output_port1 == NULL) || (output_port2 == NULL)) {
		fprintf(stderr, "no more JACK ports available\n");
		exit (1);
	}


	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		exit (1);
	}

	/* Connect the ports.  You can't do this before the client is
	 * activated, because we can't make connections to clients
	 * that aren't running.  Note the confusing (but necessary)
	 * orientation of the driver backend ports: playback ports are
	 * "input" to the backend, and capture ports are "output" from
	 * it.
	 */

	 printf("%s\n",jack_port_name (output_port1));
	 printf("%s\n",jack_port_name (output_port2));
	if (jack_connect (client, jack_port_name (output_port1), "meter:in")) {
		fprintf (stderr, "cannot connect output ports\n");
	}

	if (jack_connect (client, jack_port_name (output_port2), "meter:in")) {
		fprintf (stderr, "cannot connect output ports\n");
	}

	signal(SIGQUIT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);

	/* keep running until the Ctrl+C */
	while (1) {
		sleep (1);
	}
	jack_client_close (client);
	exit (0);
}
