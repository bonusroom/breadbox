/* Copyright (c) 1999 Regents of the University of California.
 * rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgment:
 *       This product includes software developed by the Imaging and
 *       Distributed Collaboratories Group at Lawrence Berkeley 
 *       National Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: Device.h,v 1.6 1999/08/10 20:33:47 mperry Exp $
*/


// Device.h:  Base class for devices, which may or may not be RS232 
//            devices.  Derived classes may be non-serial port devices. 

#ifndef Device_h
#define Device_h


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "utils.h"

/*#ifdef sun
#include <termio.h>
#else
#include <termios.h>
#endif  */

#ifdef __FreeBSD__
#include <sys/time.h>
#endif
#ifdef sgi
#include <sys/time.h>
#endif

class Device {
public:
	Device(int=1,short=1,Mode=NORMAL);
	virtual ~Device();
        virtual int clearInput(int);
	virtual int move(char *) = 0;
	virtual int doPower() = 0;
	virtual int shutDown() = 0;
	virtual int startup();
  	virtual int goHome() = 0;
	virtual void getDesc(char *) = 0;
	inline void setDevnum(int n) { devnum = n; }
	virtual void setAvail(int);
        inline pthread_mutex_t *getMutex() { return &devmutex; }
protected:
	int devnum;		  // device (unit) number
	static short debugLevel;  // if > 2, display device communication   
	int power;	          // 0 = 'off'; 1 = 'on'
        // keep track of which devices are responding ("available").
	short initialized;
        int avail;
	char type[2];		// A=absolute, R=relative, F=fractional
	Mode devmode;
	int lockMutex(pthread_mutex_t *);
	int unlockMutex(pthread_mutex_t *);
	/* Each device needs its own mutex to prevent simultaneous access to
           the SAME device (in case multiple threads receive requests to move 
           the same device).  */  
        pthread_mutex_t devmutex;
};

#endif
