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
 * $Id: canonvcc1.cc,v 1.5 1999/08/05 01:39:50 mperry Exp $
*/


#include "canonvcc1.h"

CanonVCC1::CanonVCC1(Serialport *port,const char *p,int n,short d,short l, Mode m) 
	: Camera(n, d, l, m)	
{
	int x;

	if (debugLevel > 0)
	   fprintf(stderr,"INITIALIZING THE CANON VC-C1\n");
	initialized = 0;
	avail = 0;
	setPort(port);
	if (devmode == SIMULATED)
	{
	    setDefaults();
	    return;
        } 
	if (devnum == 1)
	{
	    portp->openPort(p);
	    int cflags = B9600 | CS8 | CSIZE | CSTOPB;  	
	    x = portp->setupPort(cflags, 0, IGNBRK); 
	    if (x < 0)
	    {
		if (debugLevel > 0)
		   fprintf(stderr,"CAN'T SET UP A PORT FOR THE CANON VC-C1\n");
		return;
	    }	
	}
	int fd = portp->getFileDesc(); 
	if (fd > 0)
	    x = init();
	if ((fd < 0) || !avail)
	{
	    if (debugLevel > 0)
	    {
	       fprintf(stderr,"INITIALIZATION FAILED FOR THE CANON VC-C1\n");
	       fprintf(stderr,"CHECK THE CONNECTION/POWER\n");
	    }
	    return;
	}	
	setDefaults();
}

void CanonVCC1::setDefaults() 
{
	initialized = 1;
	avail = 1;
	if (devnum == 1)
	{
	    // Re-initialize all static variables (values apply to every
	    // canon vcc1 camera). 
	    minpan = -50.0;
	    maxpan = 50.0;
	    mintilt = -20.0;
	    maxtilt = 20.0; 
	    min_pspeed = 100.0;
	    max_pspeed = 500.0;
	    min_tspeed = 100.0;
	    max_tspeed = 500.0;
	    min_zspeed = 100.0;
	    max_zspeed = 500.0;
	    zoom_limit = 128.0;
	    minzoom = 1.0;
	    maxzoom = 12.0;
	}
	if (!lens)
	{	
	    pcoeff = 64.586;
	    pexp = -0.198;
	    tcoeff = 41.43;
	    texp = -0.182;
	    zcoeffw = 12.244;
	    zexpw = -0.223; 
	    zcoeffh = 8.891;
	    zexph = -0.224;	
	}
	else
	{
	    pcoeff = 65.3;
	    pexp = -0.154;
	    tcoeff = 40.447;
	    texp = -0.118;
	    zcoeffw = 13.423;
	    zexpw = -0.204; 
	    zcoeffh = 13.985;
	    zexph = -0.243;	
	}
	pspeed = max_pspeed;
	tspeed = max_tspeed;
	zspeed = max_zspeed;
	pspeed = max_pspeed;
	tspeed = max_tspeed;
}

void CanonVCC1::setPort(Serialport *p)
{
	portp = p;
	assert(portp != NULL);
} 

int CanonVCC1::init()
{
	int x;
	unsigned char buf[15];

	x = doPower();
	if (x) 
	{
	    // Send the Pedestal Initialize 1 cmd.
	    buf[0] = 0x00;
	    buf[1] = 0x58;
	    buf[2] = 0x30; 
	    x = transmit(buf, 3);
	}
	return x;
}

int CanonVCC1::getOperationalStatus(unsigned char reply[])
{
	unsigned char buf[20];
	int x;

	buf[0] = 0x00;
	buf[1] = 0x86;
	x = transmit(buf, 2);
	if (debugLevel > 2)
	{
	   printf("CanonVCC1::getOperationalStatus--transmit returned %d\n",x);
	   printf("getOperationalStatus: transmit returned reply: ");
	   displayString("getOpStatus reply", buf, x); 	
	}
	for (int i=0; i<x; i++)
	    reply[i] = buf[i]; 	
	return x;
}


int CanonVCC1::move(char *cmd)
{
    Cmdline cmds[6];  // store multiple commands
    int i, n=0, j, pt=0, pspd=0, tspd=0, zspd=0;

    if (!initialized)
	return -1;
    i = parseCmd(cmd, cmds, &pt, &pspd, &tspd, &zspd); 	
    if (devmode == SIMULATED)
	return 1;
    if (pspd)
        n = doPtSpeedCmd('p');
    if (tspd)
        n = doPtSpeedCmd('t');

    // Walk array and carry out requested commands in order of occurrence.
    for (j=0; j<i; j++)
    {
        if (!strcmp(cmds[j].axis,"pan"))
	    n = doPan(); 
        else if((cmds[j].axis[0]=='t')&&(cmds[j].axis[1]=='i')&&(cmds[j].axis[2]=='l'))
	    n = doTilt(); 
	else if ((cmds[j].axis[0]=='p')&&(cmds[j].axis[1]=='o')&&(cmds[j].axis[2]=='w'))
	{
	    n = stringToInt(cmds[j].value);
	    if (power != n)
	    {
		power = n;
		n = doPower();
	    }	
	}
        else if (!strcmp(cmds[j].axis, "shutdown") && avail)
            n = shutDown();
        else if (!strcmp(cmds[j].axis, "home"))
            n = goHome();
	else if (!strcmp(cmds[j].axis, "zoom"))
            n = doZoom();
    } 
    if ((n < 0) && (debugLevel > 0))
    {	
      fprintf(stderr,"CANON VC-C1 %d NOT RESPONDING...CHECK CONNECTION/POWER\n",
	   devnum); 
    }	
    return n;
}

void CanonVCC1::waitNotBusy()
{
    /* If the Canon VC-C1 is moving (panning, tilting, zooming, or focusing, it will 
       ignore commands.  So send the operational // status request and check values 
       of status bytes (per page 12 of the Canon VC-C1 API.  Wait until not busy or 
       3 seconds have elapsed.  */

    int x, n=0;
    unsigned char reply[15];

    x = getOperationalStatus(reply); 
    while ((n<3000)&&((reply[5]==0x32)||(reply[5]==0x33)||(reply[5]>0x35)||(reply[6]==0x38)||(reply[7]==0x31)))
    {
	usleep(1000);
	x = getOperationalStatus(reply);
	n++;
    }
}


int CanonVCC1::doPtSpeedCmd(char c)
{
        int x;
        unsigned char cmd[20];
	cmd[0] = 0x00;
	if (c == 'p')
	{
	    cmd[1] = 0x50;
       	    x = (int)pspeed;
	}
	else if (c== 't')
	{
	    cmd[1] = 0x51;
            x = (int)tspeed;
	}
        intToString(x,3,(char *)&cmd[2]);
        return transmit(cmd, 5);
}

int CanonVCC1::doPan()
{
        unsigned char req[20];
	req[0] = '\x00';
	req[1] = '\x54';
        intToString(convert("pan",maxpan), 3, (char *)&req[2]);
	waitNotBusy();
        return transmit(req, 5);
}


int CanonVCC1::doTilt()
{
        unsigned char req[20];
	req[0] = '\x00';
	req[1] = '\x55';
        intToString(convert("tilt",maxtilt), 3, (char *)&req[2]);
	waitNotBusy();
        return transmit(req, 5);
}

int CanonVCC1::doZoom()
{
        unsigned char req[20];
	req[0] = 0x00;
	req[1] = 0xA3;
	intToString(convert("zoom",zoom_limit), 2, (char *)&req[2]);
	waitNotBusy();
	if (debugLevel > 3)
	   displayString("CanonVCC1--transmitting ZOOM CMD", req, 4); 
	return transmit(req, 4);
}

int CanonVCC1::goHome()
{
	int x;
	unsigned char buf[20];

	if (debugLevel > 2)
	    printf("CanonVCC1::goHome--sending P/T HOME cmd\n");
	// Move the Pan/Tilter to home position.
	buf[0] = 0x00;
	buf[1] = 0x57;
	waitNotBusy();
	x = transmit(buf, 2);
	// Zoom to home zoom position.
	zposn = homezoom;
	if (debugLevel > 2)
	    printf("CanonVCC1::goHome--calling doZoom\n");
	waitNotBusy();
	return doZoom();
}


int CanonVCC1::shutDown()
{
	int x = 0;
	power = 0;
	if (avail)
	   x = doPower(); 
	portp->closePort();
	return x;			
}

int CanonVCC1::doPower()
{
	unsigned char buf[20];
	buf[0] = '\x00';
	buf[1] = '\xA0';
	if (power)
	    buf[2] = '\x31';
	else
	    buf[2] = '\x30'; 	
	return transmit(buf, 3);
}


int CanonVCC1::transmit(unsigned char *buf, int len)
{
    int i,n;
    unsigned char cmd[20]; 
    pthread_mutex_t *mut = portp->getPortMutex();

    cmd[0] = 0xFF;
    cmd[1] = 0x30;
    cmd[2] = 0x30;
    for (n=0,i=3; n<len; n++,i++)
	cmd[i] = buf[n];	
    cmd[i++] = 0xEF;  
    if (debugLevel > 2) 
        displayString("CanonVCC1: CMD", cmd, i);
    // Lock the port.
    n = lockMutex(mut);
    if (n < 0)
    {
        avail = 0; 
	return n;
    }
    writen(portp->getFileDesc(), cmd, i);	
    n = readFromCanon(buf);
    // Unlock the port.
    unlockMutex(mut);
    if ((n>0) && (debugLevel > 2))
        displayString("CanonVCC1: REPLY", buf, n);
    if (debugLevel > 2)
    {
        printf("CanonVCC1::transmit--readFromCanon ret'd %d: avail = %d\n",
	   n,avail);
    }
    if ((n < 0) && (debugLevel > 2))
	printf("CanonVCC1--ERROR sent by device\n");
    setAvail(n);
    return n;
}

 
int CanonVCC1::readFromCanon(unsigned char *buf)
{
	unsigned char c='\0';
	fd_set f;
	struct timeval t;
	int n,i=0; 
	int fd = portp->getFileDesc();

	t.tv_sec = 5;
	t.tv_usec = 0;
	FD_ZERO(&f);
	FD_SET(fd, &f);
	n = select(fd+1, &f, NULL, NULL, &t);
	if (debugLevel > 2)
	   printf("readFromCanon: select() returned %d\n", n);
	if ((n < 0) && (debugLevel>0))
	{
	    perror("select failed");
	    printf("errno: %d\n", errno);
	}

	if (n > 0)
	{
	    // Read until we get the header byte, FEh; then read until
	    // we get the terminator byte, EFh.

	    n = read(fd, &c, 1);
	    if (n)
		avail = 1;
	    while (n && (c != 0xFE))
	    {	
	        n = read(fd, &c, 1);
	   	if (n < 0)
		    return n;		
	    }
	    if (c == 0xFE)
	    {
		buf[i++] = c; 	
	        while (c != 0xEF)
	        {
		    n = read(fd, &c, 1);
		    if (n < 0)
			return n;
		    if (n==1)
			buf[i++] = c;
	        }
	    }
	    return i;
	} 
	return n;
}


int CanonVCC1::convert(const char *dof, float limit)
{
    float x;
    if (!strcmp(dof, "pan"))
    {
        x = pposn / limit * 650.0;
        x += 650.0;
        if (x > 1300.0)
            x = 1300.0;
        else if (x < 0.0)
            x = 0.0;
    }  
    else if (!strcmp(dof, "tilt"))
    {
        x = tposn / limit * 289.0;
        x += 289.0;
        if (x > 578.0)
            x = 578.0;
        else if (x < 0.0)
            x = 0.0;
    }
    else if (!strcmp(dof, "zoom"))
    {
        x = (zposn - 1.0) / 11.0 * limit;
        if (x < 0.0)
           x = 0.0;
        else if (x > limit)
           x = limit;
    }
    return doubleToInt((double)x);
}

void CanonVCC1::getDesc(char *buf)
{
    strcpy(buf, "\"Canon VC-C1\" ");
    if (!avail)
    {
	strcat(buf, "NOT_RESPONDING");
	return;
    }
    strcat(buf, "pan ");
    sprintf(&buf[strlen(buf)],"%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f",
	pposn,minpan,maxpan,pspeed,min_pspeed,max_pspeed);
    strcat(buf, "@tilt ");
    sprintf(&buf[strlen(buf)],"%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f",
	tposn,mintilt,maxtilt,tspeed,min_tspeed,max_tspeed);
    strcat(buf, "@zoom ");
    sprintf(&buf[strlen(buf)],"%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f",
	zposn,minzoom,maxzoom,zspeed,min_zspeed,max_zspeed);
}
 
