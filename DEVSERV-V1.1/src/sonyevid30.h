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
 * $Id: sonyevid30.h,v 1.2 1999/07/19 21:10:08 mperry Exp $
*/


// sonyevid30.h:  Interface for the Sony EVI-D30 camera.
//		  One object should represent one physical camera.  

#ifndef sonyevid30_h
#define sonyevid30_h

#include <assert.h>
#include "Cam.h"
#include "serialport.h"

#define MAX_PORTS 15
#define MAX_SONY   7

class SonyEVID30 : public Camera {
public:
	SonyEVID30(Serialport *, const char *,int=1,short=1,short=0,Mode=NORMAL);
	void setPort(Serialport *);
	void setDevnum(int);
	virtual int move(char *); 
	virtual void getDesc(char *);	
	virtual int doPower();
	virtual int shutDown();
	virtual int goHome();
	inline void setHomeZoom(float z) { homezoom = z; } 
	int doZoomSpeed(float);
private:
	/* Array used by all Sony EVI-D30s that are daisy-chained to a port to
	   count number of commands sent to the devices (to avoid RS232 buffer 
           overflow).  */
	static short cmdcount[MAX_PORTS][MAX_SONY];
	
	Serialport *portp;
	char panspeed[3];
	char tiltspeed[3];
	int init();
	void setDefaults();
	void setPtSpeedfield(char);
	int transmit(unsigned char *, int);
	int readFromSony(unsigned char *);
	int convert(const char *, float);
	int doPanAndTilt();
	int doZoom();
	int panTiltPosInq();
	int zoomPosInq();
	void checkPower();
	int powerInq();
	int getPowerInqReply();
	int verifyDeviceNumber(unsigned char);
};

#endif
