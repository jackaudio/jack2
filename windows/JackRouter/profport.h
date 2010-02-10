
/******************************************************************************
 PORTABLE ROUTINES FOR WRITING PRIVATE PROFILE STRINGS --  by Joseph J. Graf
 Header file containing prototypes and compile-time configuration.
 
 [09/05/02] D. Fober - Windows definitions added
******************************************************************************/

#ifndef __profport__
#define __profport__

#define MAX_LINE_LENGTH    1024

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#include "Windows.h"
#define get_private_profile_int        GetPrivateProfileInt
#define get_private_profile_string     GetPrivateProfileString
#define write_private_profile_string   WritePrivateProfileString
#define write_private_profile_int      WritePrivateProfileInt
#else
int get_private_profile_int      (char * section, char * entry, int def, char * file_name);
int get_private_profile_string   (char * section, char * entry, char * def, char * buffer, int buffer_len, char * file_name);
int write_private_profile_string (char * section, char * entry, char * buffer, char * file_name);
int write_private_profile_int    (char * section, char * entry, int val, char * file_name);
#endif

float get_private_profile_float	(char * section, char * entry, float def, char * file_name);

#ifdef __cplusplus
}
#endif

#endif //__profport__
