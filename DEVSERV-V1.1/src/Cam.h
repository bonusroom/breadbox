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
 * $Id: Cam.h,v 1.4 1999/07/23 00:26:37 mperry Exp $
*/

// Interface for the Camera class 

#ifndef Cam_h
#define Cam_h

#include "Device.h"
#include <string.h>
#include <math.h>
 

typedef struct Cmdline {
        char axis[10];
        char type[2];
        char value[20];
        float speed;
};


class Camera : public Device {
public:
	Camera(int=1, short=1, short=0, Mode=NORMAL);
	virtual int move(char *) = 0;
	virtual int doPower() = 0;
	virtual int shutDown() = 0;
	virtual int goHome() = 0;
	virtual void getDesc(char *) = 0;  // get description of device	
	virtual void setPanPos(char, float);     // set current pan position
	virtual void setTiltPos(char, float);    // set current tilt position
	virtual void setZoomPos(char, float);	 // set current zoom position 
	virtual float setFractionalPan(float);
	virtual float setFractionalTilt(float);
	virtual float setFractionalZoom(float);
	// Set pan, tilt, and zoom speed values
	virtual int setPanSpeed(float);
	virtual int setTiltSpeed(float);
	virtual int setZoomSpeed(float);
	inline void setHomePan(float p) { homepan = p; }
	inline void setHomeTilt(float t) { hometilt = t; }
	int parseCmd(char *, struct Cmdline cmds[], int *, int *, int *, int *);
	virtual void identifyCmd(struct Cmdline *,int *,int *,int *,int *);
protected:
        // type of lens: 0=regular; 1=wide-angle
	short lens;
	// absolute zoom limit
	static float zoom_limit;
        // minimum/maximum pan/tilt/zoom values
	static float minpan, maxpan, mintilt, maxtilt, minzoom, maxzoom; 
        // minimum/maximum pan and tilt speeds 
	static float min_pspeed, max_pspeed, min_tspeed, max_tspeed; 
        // minimum/maximum zoom speeds 
	static float min_zspeed, max_zspeed; 
	// current pan, tilt, and zoom speeds
	float pspeed, tspeed, zspeed;
        // current pan, tilt, and zoom positions
	float pposn, tposn, zposn; 
	// home positions for pan, tilt, and zoom
	float homepan, hometilt, homezoom; 
	// home pan and tilt speed
	float homespeed; 
	// coefficient and exponent for the fractional pan, tilt, and
	// zoom equations; altho. they are the same for a type of
	// camera, they vary with the type of lens (e.g., values are 
	// different for wide-angle and regular lenses
	float pcoeff, pexp, tcoeff, texp, zcoeffw, zcoeffh, zexpw, zexph;
};

#endif
