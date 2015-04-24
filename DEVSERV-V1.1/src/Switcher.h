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
 *  $Id: Switcher.h,v 1.2 1999/07/19 21:10:07 mperry Exp $
 */

// Switcher.h: Base class for a video mixer/switcher

#ifndef Switcher_h
#define Switcher_h

#include "Device.h"

#define MAX_SRC 4	

class Switcher : public Device {
public:
	Switcher(int=1, int=1, Mode=NORMAL);
	virtual int move(char *) = 0;
	virtual int doPower() = 0;
	virtual int shutDown() = 0;
	virtual int goHome() = 0;
	virtual void getDesc(char *) = 0;
	inline void setHomeSrc(int s1,int s2) { home_srcA=s1; home_srcB=s2; }  
protected:
	int min_video_unit, max_video_unit;  
	int home_srcA, home_srcB; //default (home) input sources to bus A and B
	int busA, busB;  // store current input source allocated to bus A and B
	int pnpstate;    // 0: pnp is off; 1: pnp is on 
	float xcoord, ycoord, width, height;  // for picture-in-picture
};

#endif
