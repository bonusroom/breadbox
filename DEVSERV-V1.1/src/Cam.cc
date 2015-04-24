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
 * $Id: Cam.cc,v 1.5 1999/07/23 00:26:37 mperry Exp $
*/


#include "Cam.h"

/* These values are the same for all objects in a class, so they're static 
   to save memory (as when devices are daisy-chained).  The correct values 
   will be stored by the constructor for each object with devnum = 1. */

float Camera::zoom_limit = 0.0;
float Camera::minpan = 0.0;
float Camera::maxpan = 0.0;
float Camera::mintilt = 0.0;
float Camera::maxtilt = 0.0;
float Camera::minzoom = 1.0;
float Camera::maxzoom = 12.0;
float Camera::min_pspeed = 1.0;
float Camera::max_pspeed = 24.0;
float Camera::min_tspeed = 1.0;
float Camera::max_tspeed = 20.0;
float Camera::min_zspeed = 0.0;
float Camera::max_zspeed = 7.0; 


Camera::Camera(int n, short d, short l, Mode m) : Device(n, d, m)
{
        lens = l;
   	pposn = homepan = 0.0;
    	tposn = hometilt = 0.0;
	zposn = homezoom = 1.0;
}


int Camera::setPanSpeed(float amt)
{
    /* If amt is -1.0, the user didn't specify a speed, so use the
       current value. 
       If amt is > 0 and changes the speed, set the new speed and
       return 1; else return 0.  */

    float s;
    if (amt < 0.0)
	s = pspeed;
    else if (amt == 0.0)
        s = min_pspeed;
    else
        s = amt * max_pspeed;
    if (s > max_pspeed)
        s = max_pspeed;
    if (pspeed != s)
    {
        pspeed = s;
	return 1;
    }	
    return 0;	
}


int Camera::setTiltSpeed(float amt)
{
    /* If amt is -1.0, the user didn't specify a speed, so use the
       current value. 
       If amt is > 0 and changes the speed, set the new speed and
       return 1; else return 0.  */

    float s;
    if (amt < 0.0)
	s = tspeed;
    else if (amt == 0.0)
        s = min_tspeed;
    else
        s = amt * max_tspeed;
    if (s > max_tspeed)
        s = max_tspeed;
    if (tspeed != s)
    {	
        tspeed = s;
	return 1;
    }
    return 0;
}


int Camera::setZoomSpeed(float amt)
{
    /* If amt is -1.0, the user didn't specify a speed, so use the
       current value. 
       If amt is > 0 and changes the speed, set the new speed and
       return 1; else return 0.  */

    float s;
    if (amt < 0.0)
	s = zspeed;
    else if (amt == 0.0)
        s = min_zspeed;
    else
        s = amt * max_zspeed;
    if (s > max_zspeed)
        s = max_zspeed;
    if (zspeed != s)
    {	
        zspeed = s;
	return 1;
    }
    return 0;
}

int Camera::parseCmd(char *cmd,struct Cmdline cmds[],int *pt_flag,int *pspd,int *tspd,int *zspd)
{
    int i=0;
    char *token;

    token = strtok(cmd, "@");
    cmds[i].speed = -1.0;
    sscanf(token,"%s%s%s%f",cmds[i].axis,cmds[i].type,cmds[i].value,&cmds[i].speed);
    identifyCmd(&cmds[i], pt_flag, pspd, tspd, zspd);
    i++;
    while ((token = strtok(NULL, "@")) != NULL)
    {
        cmds[i].speed = -1.0;
        sscanf(token,"%s%s%s%f",cmds[i].axis,cmds[i].type,cmds[i].value,&cmds[i
].speed);
        identifyCmd(&cmds[i], pt_flag, pspd, tspd, zspd);
        i++;
    }
    return (i);
}   


void Camera::identifyCmd(struct Cmdline *cmdPtr,int *pt_flag,int *pspd,int *tspd,int *zspd)
{
    if ((cmdPtr->axis[0]=='p')&&(cmdPtr->axis[1]=='a')&&(cmdPtr->axis[2]=='n'))
    {
        *pt_flag = 1;
        *pspd = setPanSpeed(cmdPtr->speed);
        setPanPos(cmdPtr->type[0], atof(cmdPtr->value));
    }
    else if ((cmdPtr->axis[0]=='t')&&(cmdPtr->axis[1]=='i')&&(cmdPtr->axis[2]=='l'))
    {
        *pt_flag = 1;
        *tspd = setTiltSpeed(cmdPtr->speed);
        setTiltPos(cmdPtr->type[0], atof(cmdPtr->value));
    }
    else if ((cmdPtr->axis[0]=='z')&&(cmdPtr->axis[1]=='o')&&(cmdPtr->axis[2]=='o'))
    {
	*zspd = setZoomSpeed(cmdPtr->speed);
	setZoomPos(cmdPtr->type[0], atof(cmdPtr->value));
    }
}


void Camera::setPanPos(char c, float amt)
{
    if (c == 'A')
       pposn = amt;
    else if (c == 'R')
       pposn += amt;
    else if (c == 'F')
       pposn += setFractionalPan(amt);
    if (pposn < minpan)
       pposn = minpan;
    else if (pposn > maxpan)
       pposn = maxpan;	
}


void Camera::setTiltPos(char c, float amt)
{
    if (c == 'A')
       tposn = amt;
    else if (c == 'R')
       tposn += amt;
    else if (c == 'F')
       tposn += setFractionalTilt(amt);
    if (tposn < mintilt)
       tposn = mintilt;
    else if (tposn > maxtilt)
       tposn = maxtilt;	
}


void Camera::setZoomPos(char c, float amt)
{
    if (c == 'A')
       zposn = amt;
    else if (c == 'R')
       zposn += amt;
    else if (c == 'F')
       zposn = setFractionalZoom(amt);
    if (zposn < minzoom)
       zposn = minzoom;
    else if (zposn > maxzoom)
       zposn = maxzoom;	
}


float Camera::setFractionalPan(float amt)
{
    return (pcoeff * exp(zposn * pexp) * amt);	
}


float Camera::setFractionalTilt(float amt)
{
    return (tcoeff * exp(zposn * texp) * amt);	
}


float Camera::setFractionalZoom(float amt)
{
    /* amt is fraction given by user. Compute current width being viewed 
       (y) as: y = zcoeffw * exp(zposn * zexpw).
       Compute new width to be viewed (w) as a fraction of the current width:
    	     w = amt * y.
       Compute the new zoom factor needed to view width w as:
           zoomw = ln (w / zcoeffw) / zexpw.
       Since we zoom out to view a larger area, if amt > 1, convert
       zoomw to a negative value.	
       Repeat the computation to calculate the zoom factor needed to view
       the new height and return the average of the zoom factors for
       width and height.   */

    double x = zexpw * (double)zposn;
    double y = (double)amt * zcoeffw * exp(x);
    double zoomw = log(y / zcoeffw) / zexpw;
    x = zexph * (double)zposn;
    y = (double)amt * zcoeffh * exp(x);
    double zoomh = log(y /zcoeffh) / zexph;
    return ((float)(zoomw + zoomh) / 2.0); 
}

