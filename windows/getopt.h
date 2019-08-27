#ifndef _GETOPT_H_
#define _GETOPT_H_

#include "JackCompilerDeps.h"

struct option /* specification for a long form option... */
{
	const char *name; /* option name, without leading hyphens */
	int         has_arg; /* does it take an argument? */
	int        *flag; /* where to save its status, or NULL */
	int         val; /* its associated status value */
};

enum /* permitted values for its `has_arg' field... */
{
	no_argument = 0,      /* option never takes an argument */
	required_argument, /* option always requires an argument */
	optional_argument /* option may take an argument */
};

#ifdef __cplusplus
extern "C" {
#endif

extern char *optarg; /* argument associated with option */
SERVER_EXPORT extern int optind; /* index into parent argv vector */
SERVER_EXPORT extern int optopt;
SERVER_EXPORT extern int opterr;

SERVER_EXPORT extern int getopt(int nargc, char * const *nargv, const char *options);

SERVER_EXPORT extern int getopt_long_only(int nargc, char * const *nargv, const char *options,
				const struct option *long_options, int *idx);

SERVER_EXPORT extern int getopt_long(int nargc, char * const *nargv, const char *options,
				const struct option *long_options, int *idx);

SERVER_EXPORT extern int getopt_long_only(int nargc, char * const *nargv, const char *options,
				const struct option *long_options, int *idx);

#ifdef __cplusplus
}
#endif

#endif