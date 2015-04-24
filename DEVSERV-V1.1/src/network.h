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
 * $Id: network.h,v 1.5 1999/08/10 19:54:49 mperry Exp $
*/


// network.h: Base class for network -- multi-threaded version. 

#ifndef Network_h
#define Network_h

//#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#ifndef WIN32 
#include <sys/socket.h>
#include <pthread.h>

#ifdef sun
#include <sys/systeminfo.h> 
#endif

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sysexits.h>
#include <errno.h>
#include <unistd.h>	// for close() with CC compiler
#include <sys/time.h>
#endif


class Network {
public:
	Network(int=0,int=0,short=0);
        virtual ~Network();
	inline void setDebugLevel(int n) { debugLevel = n; }
	void setPort(short);
	inline short getPort() const { return port; }
	void getLocalname(char *);
	void getRemoteHost(char *);
	virtual int openSocket() = 0;
	virtual int doSend(const char *, long);
	virtual int doRecv(char *, long);
	virtual int doBlockedRecv(char *, long, char *);
	inline int getTimeout() { return seconds; } 
protected:
	int sock, seconds, debugLevel;
	short port;
	struct sockaddr_in peer;
	// mutexes for locking receive/send buffers
	static pthread_mutex_t rmutex, smutex;
};

#endif
