
/** @file mp_thread_client.c
 *
 * @brief This simple client demonstrates the use of "jack_thread_wait" function in a multi-threaded context.
 
  A set of threads (the jack process thread + n helper threads) are used to work on a global queue of tasks.
  The last finishing thread gives control back to libjack using the "jack_thread_wait" function. Other threads suspend
  on a condition variable and are resumed next cycle by the libjack suspended thread.
*/
 
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
 
#define __SMP__ 1

#include <jack/jack.h>
#include <jack/thread.h>

//#include "jack.h"
//#include "thread.h"
#include <math.h>
 
#include "JackAtomic.h"
 
jack_port_t *input_port;
jack_port_t *output_port;
jack_client_t *client;
int buffer_size;
 
#define WORK_AT_EACH_CYCLE   1000
#define WORK_AT_EACH_LOOP  15
static SInt32 cycle_work_count = 0;
 
pthread_cond_t	cond;
pthread_mutex_t	mutex;
 
jack_nframes_t last_time = 0;
static int print_count = 50;
int result = 0;

typedef struct thread_context 
{
	pthread_t thread;
	int num;
};
 
// Simulate workload
static int fib(int n)
{
	if (n < 2)
		return n;
	else 
		return fib(n - 2) + fib(n - 1);
}

static void do_some_work(void *arg)
{
	result = fib(WORK_AT_EACH_LOOP);
}
 
static void resume_all_threads(void *arg) 
{
	thread_context* context = (thread_context*)arg;
	
	jack_nframes_t cur_time = jack_frame_time(client);
	if (--print_count == 0) {
		printf("resume_all_threads from thread = %ld jack_frame_time = %u jack_cpu_load = %f\n", context->num, (cur_time - last_time), jack_cpu_load(client));
		print_count = 50;
	}
	pthread_mutex_lock(&mutex);		// Hum...
	pthread_cond_broadcast(&cond); 
	pthread_mutex_unlock(&mutex);	// Hum...
	cycle_work_count = WORK_AT_EACH_CYCLE;
	last_time = cur_time;
}
 
static void suspend_jack_thread(void *arg) 
{
	jack_thread_wait(client, 0);
}
 
static void suspend_worker_thread(void *arg) 
{
	pthread_mutex_lock(&mutex);		// Hum...
	pthread_cond_wait(&cond, &mutex); 
	pthread_mutex_unlock(&mutex);	// Hum...
}
 
static void * worker_aux_thread(void *arg)
{
	while (1) {
 
		int val = DEC_ATOMIC(&cycle_work_count);
 
		if (val == 1) {  // Last thread
			suspend_jack_thread(arg);
			resume_all_threads(arg);
		} else if (val < 1) {	
			suspend_worker_thread(arg);
		} else {
			do_some_work(arg);
		}
	}
 
	return 0;
}
 
static void * worker_thread(void *arg)
{
	suspend_worker_thread(arg);		// Start in "suspended" state
	worker_aux_thread(arg);
	return 0;
}
 
// Example of audio process
int process(jack_nframes_t nframes, void *arg)
{
	resume_all_threads(arg);
	worker_aux_thread(arg);
	return 0;
}
 
/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void jack_shutdown (void *arg)
{
	exit(1);
}
 
int main (int argc, char *argv[])
{
	thread_context* worker_threads;
	int n, nthreads = 0;
	
	if (argc == 2) 
		nthreads = atoi(argv[1]);
	
	worker_threads = (thread_context *) malloc (sizeof (thread_context) * nthreads);
	
	/* open a client connection to the JACK server */
	if ((client = jack_client_open("mp_thread_test", JackNoStartServer, NULL)) == NULL) {
		fprintf(stderr, "Cannot open client\n");
		exit(1);
	}
 
	buffer_size = jack_get_buffer_size(client);
 
	/* tell the JACK server to call the 'callback' function
	*/
	worker_threads[0].num = 0;
	jack_set_process_callback(client, process, &worker_threads[0]);
 
	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/
	jack_on_shutdown(client, jack_shutdown, 0);
 
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
 
	input_port = jack_port_register(client, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	output_port = jack_port_register(client, "output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
 
	if ((input_port == NULL) || (output_port == NULL)) {
		fprintf(stderr, "no more JACK ports available\n");
		exit(1);
	}
 	
	fprintf(stderr, "Creating %d threads\n", nthreads);
 
	for (n = 1; n <= nthreads; ++n) {
		worker_threads[n].num = n;
		if (jack_client_create_thread(client, &worker_threads[n].thread, 90, 1, worker_thread, &worker_threads[n]) < 0) 
			exit(1);
		jack_acquire_real_time_scheduling (worker_threads[n].thread, 90);
	}
 
	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */
 
	if (jack_activate(client)) {
		fprintf(stderr, "cannot activate client");
		exit(1);
	}
 
	while (1) {
	#ifdef WIN32 
		Sleep(1000);
	#else
		sleep(1);
	#endif
	}
 
	jack_client_close(client);
 
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
 
	exit(0);
}
