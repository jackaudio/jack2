/*
	Copyright (C) 2004-2005 Grame

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

#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <exception>

pthread_t fThread;
char fName[256];
int fFifo;

void* ThreadHandler(void* arg)
{
	char c;
	printf("ThreadHandler\n");
	try {
		while (1) {
			read(fFifo, &c, sizeof(c));
			sleep(1);
			//pthread_testcancel();
		}
	} catch (std::exception e) {}
}


int main(int argc, char * const argv[])
{
 	int res;
	void* status;
	struct stat statbuf;
	
	printf("Thread test\n");
	std::set_terminate(__gnu_cxx::__verbose_terminate_handler);

	sprintf(fName, "/tmp/fifo");

	if (stat(fName, &statbuf)) {
		if (errno == ENOENT) {
			if (mkfifo(fName, 0666) < 0) {
				printf("Cannot create inter-client FIFO [%s]\n", fName);
				return 0;
			}
		} else {
			printf("Cannot check on FIFO %s\n", fName);
			return 0;
		}
	} else {
		if (!S_ISFIFO(statbuf.st_mode)) {
			printf("FIFO (%s) already exists, but is not a FIFO!\n", fName);
			return 0;
		}
	}

	if ((fFifo = open(fName, O_RDWR|O_CREAT, 0666)) < 0) {
		printf("Cannot open fifo [%s]", fName);
		return 0;
	} 
	
    if (res = pthread_create(&fThread, 0, ThreadHandler, NULL)) {
		printf("Cannot set create thread %d\n", res);
		return 0;
	}
	
	sleep(3);
	printf("Cancel Thread\n");
	pthread_cancel(fThread);
	pthread_join(fThread, &status);
}
