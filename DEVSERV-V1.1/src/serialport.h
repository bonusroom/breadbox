/*
 *  Copyright (c) 1999 Regents of the University of California.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. All advertising materials mentioning features or use of this software
 *     must display the following acknowledgment:
 *       This product includes software developed by the Imaging and
 *       Distributed Collaboration Group at Lawrence Berkeley 
 *       National Laboratory.
 *  4. Neither the name of the University nor of the Laboratory may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 *
 *  $Id: serialport.h,v 1.6 1999/08/06 19:21:29 mperry Exp $
 */

// serialport.h:  contains member functions to open and set up
//                a serial port (e.g., for RS232 devices).


#ifndef serialport_h
#define serialport_h


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#ifdef sun
#include <termio.h>
#include <sys/termios.h>
#endif

#ifdef __FreeBSD__
#include <termios.h>
#include <sys/ioctl.h>
#endif

#ifdef sgi
#include <sys/time.h>
#include <sys/termios.h>
#endif

#ifdef __linux__
#include <sys/time.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif


class Serialport {
public:
	Serialport(int=0,int=1);
	~Serialport();
	void openPort(const char *);
	int setupPort(int cflags, int oflags, int iflags);
	void closePort();
	inline int getFileDesc() const { return fd; }
	inline int getID() const { return portid; }
        inline pthread_mutex_t *getPortMutex() { return &port_mutex; }
private:
	int fd;		// file descriptor 
	int debugLevel; // if > 0, display all error messages.
	int portid;	// identifier 
	pthread_mutex_t port_mutex;  // to lock a serial port 
	struct termios oldmode;
};

#endif

