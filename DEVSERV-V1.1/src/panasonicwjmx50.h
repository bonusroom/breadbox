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
 * $Id: panasonicwjmx50.h,v 1.3 1999/08/04 23:06:13 mperry Exp $
*/

// panasonicwjmx50.h: Interface for the Panasonic WJ-MX50 video mixer/switcher 

#ifndef panasonicwjmx50_h
#define panasonicwjmx50_h

#include <assert.h>
#include "Switcher.h"
#include "serialport.h"

class PanasonicWJMX50 : public Switcher {
public:
	PanasonicWJMX50(Serialport *, const char *,int=1,int=1, Mode=NORMAL);
	void setPort(Serialport *);
	virtual int move(char *);
	virtual void getDesc(char *);
	virtual int doPower();
	virtual int shutDown();
	virtual int goHome();
	void setCoord(float, float);
	void setSize(float, float);
	int getTransCoord(float);
	int doCrosspoint(char);
	int doPnp();
	int doMixWipeLever();
	int doPositioner();
	int getSize();
private:
	Serialport *portp;
	int transmit(unsigned char *, int);
	int readFromSwitcher(unsigned char *);
	int setWipeMode(char *, char *, char); 
	int setMixWipeEffect();
	int setMixWipeLever(char *, char *, char *);
	int setPositioner(char *, char *);
	int doQuery(int);
	int sendPowerCmd();
};

#endif
