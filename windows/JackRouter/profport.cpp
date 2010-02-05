/*
History : 
01-28-02 : Change the location of temporary files created in write_private_profile_string
		   now done in TmpDirectory.
01-29-02 : Correct bug when the '=' character is not present.
06-18-02 : Return default value if file does not exist, new write_private_profile_int function.
*/

/***** Routines to read profile strings --  by Joseph J. Graf ******/
/***** corrections and improvements -- by D. Fober - Grame ******/
/*
	corrections: buffer sizes control
	improvements: behavior more similar to windows
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <string>

#include <ctype.h> 
#include "profport.h"   /* function prototypes in here */

#ifndef WIN32

static int read_line (FILE *fp, char *bp, int size);
static int read_section(FILE *fp, char *section);
static int read_entry (FILE *fp, char *entry, char *buff, int size);
static char * read_value (char *buff);
static int read_int_value (char *buff, int def);
static char * read_file (char *file);
static char * str_search (char * buff, char * str, int stopCond);

/*****************************************************************
* Function:     read_line()
* Arguments:    <FILE *> fp - a pointer to the file to be read from
*               <char *> bp - a pointer to the copy buffer
*               <int>  size - size of the copy buffer
* Returns:      the line length if successful -1 otherwise
******************************************************************/
static int read_line(FILE *fp, char *bp, int size)
{  
	char c = '\0';
	int i = 0, limit = size-2;

	/* Read one line from the source file */
	while (((c = getc(fp)) != '\n') && (i < limit)) {
		if (c == EOF) {
			if (!i) return -1;
			else break;
		}
		bp[i++] = c;
	}
	bp[i] = '\0';
	return i;
}

static int read_section (FILE *fp, char *section)
{  
	char buff[MAX_LINE_LENGTH];
	char t_section[MAX_LINE_LENGTH];
	int n, slen;

	sprintf(t_section,"[%s]", section); /* Format the section name */
	slen = strlen (t_section);
	/*  Move through file 1 line at a time until a section is matched or EOF */
	do {
		n = read_line(fp, buff, MAX_LINE_LENGTH); 
		if (n == -1)   
			return 0;
	} while (strncmp (buff,t_section, slen));
	return 1;
 }

static int read_entry (FILE *fp, char *entry, char *buff, int size)
{  
	int n, elen = strlen (entry);

	do {
		n = read_line(fp, buff, size); 
		if (n == -1) 
			return 0;
		else if (*buff == '[')
			return 0;	
	} while (strncmp (buff, entry, elen));
	return 1;
 }

#define isBlank(c)	((c == ' ') || (c == '\t'))
static char * read_value (char *buff)
{  
    char * eq = strrchr (buff,'=');    /* Parse out the equal sign */
    if (eq) {
    	eq++;
    	while (*eq && isBlank(*eq))
    		eq++;
//    	return *eq ? eq : 0;
    	return eq;
    }
    return eq;
 }

#define isSignedDigit(c)	(isdigit(c) || (c == '+') || (c == '-'))
static int read_int_value (char *buff, int def)
{  
    char * val = read_value (buff);
    char value[20]; int i;
    
    if (!*val) return def;
    
	for (i = 0; isSignedDigit(*val) && (i <= 10); i++ )
		value[i] = *val++;
	value[i] = '\0';
    return value[0] ? atoi(value) : def;
}
 
static char * read_file (char *file)
{
	FILE *fd = fopen (file,"r");
	int size; char * buff = 0;
	
	if (!fd) return 0;
	if (fseek (fd, 0, SEEK_END) == -1) goto err;
	size = ftell (fd);
	if (size < 0) goto err;
	if (fseek (fd, 0, SEEK_SET) == -1) goto err;
	buff = (char *) malloc (size+1);
	if (buff) {
		*buff = 0;
		fread (buff, 1, size, fd);
		buff[size] = 0;
	}
err:
	fclose (fd);
	return buff;
}
 
static char * str_search (char * buff, char * str, int stopCond)
{
	char *ptr = buff;
	int len = strlen (str);
	while (*ptr && strncmp (ptr, str, len)) {
		while (*ptr && (*ptr++ != '\n')) 
			;
		if (*ptr == stopCond)
			return 0;
	}
	return *ptr ? ptr : 0;
}

/**************************************************************************
* Function:     get_private_profile_int()
* Arguments:    <char *> section - the name of the section to search for
*               <char *> entry - the name of the entry to find the value of
*               <int> def - the default value in the event of a failed read
*               <char *> file_name - the name of the .ini file to read from
* Returns:      the value located at entry
***************************************************************************/
int get_private_profile_int(char *section,
    char *entry, int def, char *file_name)
{   
    FILE *fp = fopen(file_name,"r");
    char buff[MAX_LINE_LENGTH];
    
    if( !fp ) return def; /* Return default value if file does not exist */
    if (!read_section (fp, section)) goto err;
    if (!read_entry (fp, entry, buff, MAX_LINE_LENGTH)) goto err;
	def = read_int_value (buff, def);
err:
	fclose (fp);
	return def;
}

/**************************************************************************
* Function:     get_private_profile_string()
* Arguments:    <char *> section - the name of the section to search for
*               <char *> entry - the name of the entry to find the value of
*               <char *> def - default string in the event of a failed read
*               <char *> buffer - a pointer to the buffer to copy into
*               <int> buffer_len - the max number of characters to copy
*               <char *> file_name - the name of the .ini file to read from
* Returns:      the number of characters copied into the supplied buffer
***************************************************************************/

int get_private_profile_string(char *section, char *entry, char *def,
    char *buffer, int buffer_len, char *file_name)
{   
    FILE *fp = fopen (file_name,"r");
    char buff[MAX_LINE_LENGTH];
    char *val;
    
    if( !fp ) goto err; /* Return default value if file does not exist */
    if (!read_section (fp, section)) goto err;
    if (!read_entry (fp, entry, buff, MAX_LINE_LENGTH)) goto err;
	val = read_value (buff);
    if(val) def = val;

err:
	if (fp) fclose (fp);
	if (def) {
		strncpy (buffer, def, buffer_len - 1);
		buffer[buffer_len] = '\0';
	}
	else buffer[buffer_len] = '\0';
	return strlen (buffer);
}


/***************************************************************************
 * Function:    write_private_profile_string()
 * Arguments:   <char *> section - the name of the section to search for
 *              <char *> entry - the name of the entry to find the value of
 *              <char *> buffer - pointer to the buffer that holds the string
 *              <char *> file_name - the name of the .ini file to read from
 * Returns:     TRUE if successful, otherwise FALSE
 ***************************************************************************/
int write_private_profile_string(char *section,
    char *entry, char *buffer, char *file_name)

{
	char * content = read_file(file_name);
	FILE * fd = fopen(file_name,"w");
    char t_section[MAX_LINE_LENGTH], *ptr;
    int ret = 0;
	
	if (!fd) goto end;
	if (!content) {
    	fprintf (fd, "[%s]\n%s = %s\n", section, entry, buffer);
    	ret = 1;
    	goto end;
	}
    sprintf(t_section,"[%s]",section);         /* Format the section name */
    ptr = str_search (content, t_section, 0);  /* look for the section start */
    if (!ptr) {
    	/* no such section: add the new section at end of file */
    	fprintf (fd, "%s\n[%s]\n%s = %s\n", content, section, entry, buffer);
    }
    else {
    	char * eptr;
    	eptr = str_search (ptr, entry, '[');
    	if (!eptr) {
    		/* no such entry: looks for next section */
    		eptr = str_search (++ptr, "[", 0);
	    	if (!eptr) {
    			/* section is the last one */
	    		fprintf (fd, "%s\n%s = %s\n", content, entry, buffer);
	    	}
	    	else {
	    		while (*ptr && (*ptr != '\n')) ptr++;
	    		*ptr = 0;
	    		fprintf (fd, "%s\n%s = %s", content, entry, buffer);
	    		*ptr = '\n';
	    		fprintf (fd, "%s", ptr);
	    	}
	    }
	    else {
	    	*eptr++ = 0;
	    	fprintf (fd, "%s%s = %s", content, entry, buffer);
	    	while (*eptr && (*eptr != '\n')) eptr++;
	    	if (eptr) fprintf (fd, "%s", eptr);
	    }
    }
    ret = 1;

end:
	if (content) free(content);
	if (fd) fclose(fd);
	return 0;
}

/***************************************************************************
 * Function:    write_private_profile_int()
 * Arguments:   <char *> section - the name of the section to search for
 *              <char *> entry - the name of the entry to find the value of
 *              <int> buffer - the value to be written
 *              <char *> file_name - the name of the .ini file to read from
 * Returns:     TRUE if successful, otherwise FALSE
 ***************************************************************************/
int write_private_profile_int(char *section,
    char *entry, int val, char *file_name)
{
    char buffer [64];
    sprintf(buffer, "%d", val);
    return write_private_profile_string (section,entry, buffer, file_name);
}

#endif // #ifndef WIN32


/**************************************************************************
* Function:     get_private_profile_float()
* Arguments:    <char *> section - the name of the section to search for
*               <char *> entry - the name of the entry to find the value of
*               <float> def - the default value in the event of a failed read
*               <char *> file_name - the name of the .ini file to read from
* Returns:      the value located at entry
* Warning:		The float value to be read must not contain more than 100 digits.
* Author:		CD, 15/11/2006.
***************************************************************************/
#define maxFloatLen	100
float get_private_profile_float	(char * section, char * entry, float def, char * file_name)
{
	float result = def;
	char buffer[ maxFloatLen ], *endptr;

	if ( get_private_profile_string(section, entry, "", buffer, maxFloatLen, file_name) > 0 )
	{
		result = (float)strtod(buffer, &endptr);
		if ((result==0) && (endptr==buffer))
			result = def;
	}
	return result;
}
