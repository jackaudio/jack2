/*
 *  transport.c -- JACK transport master example client.
 *
 *  Copyright (C) 2003 Jack O'Quin.
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif
#include <jack/jack.h>
#include <jack/transport.h>

#ifndef HAVE_READLINE
#define whitespace(c) (((c) == ' ') || ((c) == '\t'))
#endif

char *package;				/* program name */
int done = 0;
jack_client_t *client;

/* Time and tempo variables.  These are global to the entire,
 * transport timeline.  There is no attempt to keep a true tempo map.
 * The default time signature is: "march time", 4/4, 120bpm
 */
float time_beats_per_bar = 4.0;
float time_beat_type = 4.0;
double time_ticks_per_beat = 1920.0;
double time_beats_per_minute = 120.0;
volatile int time_reset = 1;		/* true when time values change */

/* JACK timebase callback.
 *
 * Runs in the process thread.  Realtime, must not wait.
 */
static void timebase(jack_transport_state_t state, jack_nframes_t nframes, 
	      jack_position_t *pos, int new_pos, void *arg)
{
	double min;			/* minutes since frame 0 */
	long abs_tick;			/* ticks since frame 0 */
	long abs_beat;			/* beats since frame 0 */

	if (new_pos || time_reset) {

		pos->valid = JackPositionBBT;
		pos->beats_per_bar = time_beats_per_bar;
		pos->beat_type = time_beat_type;
		pos->ticks_per_beat = time_ticks_per_beat;
		pos->beats_per_minute = time_beats_per_minute;

		time_reset = 0;		/* time change complete */

		/* Compute BBT info from frame number.  This is relatively
		 * simple here, but would become complex if we supported tempo
		 * or time signature changes at specific locations in the
		 * transport timeline. */

		min = pos->frame / ((double) pos->frame_rate * 60.0);
		abs_tick = min * pos->beats_per_minute * pos->ticks_per_beat;
		abs_beat = abs_tick / pos->ticks_per_beat;

		pos->bar = abs_beat / pos->beats_per_bar;
		pos->beat = abs_beat - (pos->bar * pos->beats_per_bar) + 1;
		pos->tick = abs_tick - (abs_beat * pos->ticks_per_beat);
		pos->bar_start_tick = pos->bar * pos->beats_per_bar *
			pos->ticks_per_beat;
		pos->bar++;		/* adjust start to bar 1 */

#if 0
		/* some debug code... */
		fprintf(stderr, "\nnew position: %" PRIu32 "\tBBT: %3"
			PRIi32 "|%" PRIi32 "|%04" PRIi32 "\n",
			pos->frame, pos->bar, pos->beat, pos->tick);
#endif

	} else {

		/* Compute BBT info based on previous period. */
		pos->tick +=
			nframes * pos->ticks_per_beat * pos->beats_per_minute
			/ (pos->frame_rate * 60);

		while (pos->tick >= pos->ticks_per_beat) {
			pos->tick -= pos->ticks_per_beat;
			if (++pos->beat > pos->beats_per_bar) {
				pos->beat = 1;
				++pos->bar;
				pos->bar_start_tick +=
					pos->beats_per_bar
					* pos->ticks_per_beat;
			}
		}
	}
}

static void jack_shutdown(void *arg)
{
#if defined(RL_READLINE_VERSION) && RL_READLINE_VERSION >= 0x0400
	rl_cleanup_after_signal();
#endif
	fprintf(stderr, "JACK shut down, exiting ...\n");
	exit(1);
}

static void signal_handler(int sig)
{
	jack_client_close(client);
	fprintf(stderr, "signal received, exiting ...\n");
	exit(0);
}

/* Command functions: see commands[] table following. */

static void com_activate(char *arg)
{
	if (jack_activate(client)) {
		fprintf(stderr, "cannot activate client");
	}
}

static void com_deactivate(char *arg)
{
	if (jack_deactivate(client)) {
		fprintf(stderr, "cannot deactivate client");
	}
}

static void com_exit(char *arg)
{
	done = 1;
}

static void com_help(char *);			/* forward declaration */

static void com_locate(char *arg)
{
	jack_nframes_t frame = 0;

	if (*arg != '\0')
		frame = atoi(arg);

	jack_transport_locate(client, frame);
}

static void com_master(char *arg)
{
	int cond = (*arg != '\0');
	if (jack_set_timebase_callback(client, cond, timebase, NULL) != 0)
		fprintf(stderr, "Unable to take over timebase.\n");
}

static void com_play(char *arg)
{
	jack_transport_start(client);
}

static void com_release(char *arg)
{
	jack_release_timebase(client);
}

static void com_stop(char *arg)
{
	jack_transport_stop(client);
}

/* Change the tempo for the entire timeline, not just from the current
 * location. */
static void com_tempo(char *arg)
{
	float tempo = 120.0;

	if (*arg != '\0')
		tempo = atof(arg);

	time_beats_per_minute = tempo;
	time_reset = 1;
}

/* Set sync timeout in seconds. */
static void com_timeout(char *arg)
{
	double timeout = 2.0;

	if (*arg != '\0')
		timeout = atof(arg);

	jack_set_sync_timeout(client, (jack_time_t) (timeout*1000000));
}


/* Command parsing based on GNU readline info examples. */

typedef void cmd_function_t(char *);	/* command function type */

/* Transport command table. */
typedef struct {
	char *name;			/* user printable name */
	cmd_function_t *func;		/* function to call */
	char *doc;			/* documentation  */
} command_t;

/* command table must be in alphabetical order */
command_t commands[] = {
	{"activate",	com_activate,	"Call jack_activate()"},
	{"exit",	com_exit,	"Exit transport program"},
	{"deactivate",	com_deactivate,	"Call jack_deactivate()"},
	{"help",	com_help,	"Display help text [<command>]"},
	{"locate",	com_locate,	"Locate to frame <position>"},
	{"master",	com_master,	"Become timebase master "
					"[<conditionally>]"},
	{"play",	com_play,	"Start transport rolling"},
	{"quit",	com_exit,	"Synonym for `exit'"},
	{"release",	com_release,	"Release timebase"},
	{"stop",	com_stop,	"Stop transport"},
	{"tempo",	com_tempo,	"Set beat tempo <beats_per_min>"},
	{"timeout",	com_timeout,	"Set sync timeout in <seconds>"},
	{"?",		com_help,	"Synonym for `help'" },
	{(char *)NULL, (cmd_function_t *)NULL, (char *)NULL }
};
     
static command_t *find_command(char *name)
{
	register int i;
	size_t namelen;

	if ((name == NULL) || (*name == '\0'))
		return ((command_t *)NULL);

	namelen = strlen(name);
	for (i = 0; commands[i].name; i++)
		if (strncmp(name, commands[i].name, namelen) == 0) {

			/* make sure the match is unique */
			if ((commands[i+1].name) &&
			    (strncmp(name, commands[i+1].name, namelen) == 0))
				return ((command_t *)NULL);
			else
				return (&commands[i]);
		}
     
	return ((command_t *)NULL);
}

static void com_help(char *arg)
{
	register int i;
	command_t *cmd;

	if (!*arg) {
		/* print help for all commands */
		for (i = 0; commands[i].name; i++) {
			printf("%s\t\t%s.\n", commands[i].name,
			       commands[i].doc);
		}

	} else if ((cmd = find_command(arg))) {
		printf("%s\t\t%s.\n", cmd->name, cmd->doc);

	} else {
		int printed = 0;

		printf("No `%s' command.  Valid command names are:\n", arg);

		for (i = 0; commands[i].name; i++) {
			/* Print in six columns. */
			if (printed == 6) {
				printed = 0;
				printf ("\n");
			}

			printf ("%s\t", commands[i].name);
			printed++;
		}

		printf("\n\nTry `help [command]\' for more information.\n");
	}
}

static void execute_command(char *line)
{
	register int i;
	command_t *command;
	char *word;
     
	/* Isolate the command word. */
	i = 0;
	while (line[i] && whitespace(line[i]))
		i++;
	word = line + i;
     
	while (line[i] && !whitespace(line[i]))
		i++;
     
	if (line[i])
		line[i++] = '\0';
     
	command = find_command(word);
     
	if (!command) {
		fprintf(stderr, "%s: No such command.  There is `help\'.\n",
			word);
		return;
	}
     
	/* Get argument to command, if any. */
	while (whitespace(line[i]))
		i++;
     
	word = line + i;
     
	/* invoke the command function. */
	(*command->func)(word);
}


/* Strip whitespace from the start and end of string. */
static char *stripwhite(char *string)
{
	register char *s, *t;

	s = string;
	while (whitespace(*s))
		s++;

	if (*s == '\0')
		return s;
     
	t = s + strlen (s) - 1;
	while (t > s && whitespace(*t))
		t--;
	*++t = '\0';
     
	return s;
}
     
static char *dupstr(char *s)
{
	char *r = malloc(strlen(s) + 1);
	strcpy(r, s);
	return r;
}
     
/* Readline generator function for command completion. */
static char *command_generator (const char *text, int state)
{
	static int list_index, len;
	char *name;
     
	/* If this is a new word to complete, initialize now.  This
	   includes saving the length of TEXT for efficiency, and
	   initializing the index variable to 0. */
	if (!state) {
		list_index = 0;
		len = strlen (text);
	}
     
	/* Return the next name which partially matches from the
	   command list. */
	while ((name = commands[list_index].name)) {
		list_index++;
     
		if (strncmp(name, text, len) == 0)
			return dupstr(name);
	}
     
	return (char *) NULL;		/* No names matched. */
}

static void command_loop()
{
#ifdef HAVE_READLINE
	char *line, *cmd;
	char prompt[32];

	snprintf(prompt, sizeof(prompt), "%s> ", package);

	/* Allow conditional parsing of the ~/.inputrc file. */
	rl_readline_name = package;
     
	/* Define a custom completion function. */
	rl_completion_entry_function = command_generator;
#else
	char line[64] = {0,};
	char *cmd = NULL;
#endif

	/* Read and execute commands until the user quits. */
	while (!done) {

#ifdef HAVE_READLINE
		line = readline(prompt);
     
		if (line == NULL) {	/* EOF? */
			printf("\n");	/* close out prompt */
			done = 1;
			break;
		}
#else
		printf("%s> ", package);
		fgets(line, sizeof(line), stdin);
		line[strlen(line)-1] = '\0';
#endif
     
		/* Remove leading and trailing whitespace from the line. */
		cmd = stripwhite(line);

		/* If anything left, add to history and execute it. */
		if (*cmd)
		{
#ifdef HAVE_READLINE
			add_history(cmd);
#endif
			execute_command(cmd);
		}
     
#ifdef HAVE_READLINE
		free(line);		/* realine() called malloc() */
#endif
	}
}

int main(int argc, char *argv[])
{
	jack_status_t status;

	/* basename $0 */
	package = strrchr(argv[0], '/');
	if (package == 0)
		package = argv[0];
	else
		package++;

	/* open a connection to the JACK server */
	client = jack_client_open (package, JackNullOption, &status);
	if (client == NULL) {
		fprintf (stderr, "jack_client_open() failed, "
			 "status = 0x%2.0x\n", status);
		return 1;
	}

	signal(SIGQUIT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);

	jack_on_shutdown(client, jack_shutdown, 0);

	if (jack_activate(client)) {
		fprintf(stderr, "cannot activate client");
		return 1;
	}

	/* execute commands until done */
	command_loop();

	jack_client_close(client);
	exit(0);
}
