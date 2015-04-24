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
 *       Distributed Collaboratories Group at Lawrence Berkeley 
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
 *  $Id: serialport.cc,v 1.10 1999/08/06 19:28:49 mperry Exp $
 */

#include "serialport.h"

Serialport::Serialport(int i, int d)
{
	debugLevel = d;
	portid = i;
	fd = -1;
        pthread_mutex_init(&port_mutex, NULL);
}

Serialport::~Serialport()
{
	if (fd > 0)
	   closePort();
}

void Serialport::openPort(const char *p)
{
    fd = open(p, O_RDWR | O_NDELAY);
    if ((fd < 0) && (debugLevel > 0))
    {
        fprintf(stderr,"Devserv: open() failed for %s; errno: %d",p,errno);
	fprintf(stderr," ...terminating\n");
        exit(1);
    }
}


int Serialport::setupPort(int cflags, int oflags, int iflags)
{
    struct termios newmode;

    /* save current port settings */
    if (tcgetattr(fd, &oldmode) < 0)
    {	
	if (debugLevel > 0)
	{
           perror("Devserv: tcgetattr");
	   fprintf(stderr, " errno: %d\n", errno);
	}
	closePort();
        return(-1);
    }

    /* clear out new termios struct and set the new port settings  */
    memset(&newmode, 0, sizeof(newmode));
    newmode.c_iflag = iflags;	
    newmode.c_oflag = oflags;
    newmode.c_cflag = cflags | CLOCAL | CREAD; 
    newmode.c_lflag = 0;
    newmode.c_cc[VMIN] = newmode.c_cc[VTIME] = 0;

    tcflush(fd, TCIFLUSH);
    if (tcsetattr(fd, TCSANOW, &newmode) < 0)
    {	
	if (debugLevel > 0)
	{
           perror("Devserv: tcsetattr for new port settings");
	   fprintf(stderr, " errno: %d\n", errno);
	}
	closePort();
        return(-1);
    }

    if (fcntl(fd, F_SETFL, FNDELAY) < 0) 
    {
	if (debugLevel > 0)
	{
           perror("Devserv: fcntl");
	   fprintf(stderr, " errno: %d\n", errno);
	}
	closePort();
        return(-1);
    }
    return(0);
}


void Serialport::closePort()
{
    /* Restore the state of the serial port and then close the port. */
    if (tcsetattr(fd, TCSANOW, &oldmode) < 0)
    {
	if (debugLevel > 0)
	{
           perror("Devserv: Can't reset port");
	   fprintf(stderr, " errno: %d\n", errno);
	}
    }
    close(fd);
}

