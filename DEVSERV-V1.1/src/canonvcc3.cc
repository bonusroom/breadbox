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
 * $Id: canonvcc3.cc,v 1.5 1999/08/05 01:39:50 mperry Exp $
*/

#include "canonvcc3.h"

CanonVCC3::CanonVCC3(Serialport *port, const char *p, int n, short d,short l,Mode m) 
	: Camera(n, d, l, m)	
{
	int x;

	if (debugLevel > 0)
	   fprintf(stderr,"INITIALIZING THE CANON VC-C3\n");
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
		   fprintf(stderr,"CAN'T SET UP PORT FOR THE CANON VC-C3\n");
		return;
	    }	
	}
	if (portp->getFileDesc() < 0)
	{
	    if (debugLevel > 0)
	       fprintf(stderr,"CAN'T OPEN PORT FOR THE CANON VC-C3\n");
	    return;
	}	
	x = setCntlMode("00");
	if (debugLevel > 2)
	   printf("Canon::Canon--setCntlMode ret'd %d; avail = %d\n",x,avail);
	if (!avail)
	{
	    if (debugLevel > 0)
	       fprintf(stderr,"INITIALIZATION FAILED FOR CANON VC-C3\n");	
	    return;
	}
	setDefaults();
}

void CanonVCC3::setDefaults()
{
	initialized = 1;
	if (devnum == 1)
	{
	    minpan = -90.0;
	    maxpan = 90.0;
	    mintilt = -30.0;
	    maxtilt = 25.0; 
	    min_pspeed = 1.0;
	    max_pspeed = 76.0;
	    min_tspeed = 1.0;
	    max_tspeed = 70.0;
	    min_zspeed = 0.0;
	    max_zspeed = 7.0;
	    minzoom = 1.0;
	    maxzoom = 12.0;
	    setZoomLimit();
	}
        pcoeff = 72.529;
	pexp = -0.095;
	tcoeff = 55.423;
	texp = -0.144;
	zcoeffw = 13.21;
	zcoeffh = 10.177;
	zexpw = -0.18; 
	zexph = -0.211;	
	if (!lens && (debugLevel > 0))
	{
	    fprintf(stderr, "WARNING: You did not specify a wide-angle ");
	    fprintf(stderr, "lens for the Canon VC-C3, and it's only ");
	    fprintf(stderr, "calibrated for a wide-angle lens."); 
	}		
	pspeed = max_pspeed;
	tspeed = max_tspeed;
	zspeed = min_zspeed;
	doZoomSpeedCmd();
}

void CanonVCC3::setPort(Serialport *p)
{
	portp = p;
	assert(portp != NULL);
} 

int CanonVCC3::readZoomSpeed()
{
	char req[20]; 
	strcpy(req, "0501120401");
        return transmit((unsigned char *)req, 10);
}

int CanonVCC3::setCntlMode(const char *mode)
{
	char req[20]; 
	strcpy(req, "05081701");
	strcat(req, mode);
	return transmit((unsigned char *)req, strlen(req));
}

int CanonVCC3::move(char *cmd)
{
    struct Cmdline cmds[6];  // store multiple commands
    int i, n=0, j, pt=0, pspd = 0, tspd = 0, zspd=0;

    if (!initialized)
	return -1;
    i = parseCmd(cmd, cmds, &pt, &pspd, &tspd, &zspd);	
    if (devmode == SIMULATED)
	return 1;
    if (pspd || tspd)
        n = doPtSpeedCmd();
    if (zspd)
	n = doZoomSpeedCmd();	

    // Walk array and carry out requested commands in order of occurrence.
    for (j=0; j<i; j++)
    {
        if(pt&&(!strcmp(cmds[j].axis,"pan")||!strcmp(cmds[j].axis,"tilt")))
	{
	    n = doPanAndTilt(); 
	    pt = 0;
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
      fprintf(stderr,"CANON VC-C3 (devnum: %d) NOT RESPONDING...CHECK CONNECTION/POWER\n",devnum); 
    }	
    return n;
}


void CanonVCC3::setZoomLimit()
{
    char req[12];
    int x,y;
    strcpy(req, "0501120203");
    x = transmit((unsigned char *)req, 10); 
    x = req[5];
    y = req[6];	
    x <<= 8; 	
    zoom_limit = x | y; 	
}


int CanonVCC3::doPtSpeedCmd()
{
        char req[40];
        strcpy(req, "0705120302");
        intToString((int)pspeed, 2, &req[strlen(req)]);
        intToString((int)tspeed, 2, &req[strlen(req)]);
        return transmit((unsigned char *)req, strlen(req));
}

int CanonVCC3::doZoomSpeedCmd()
{ 
        char req[40];
        strcpy(req, "0601120402");
        intToString((int)zspeed,2,&req[strlen(req)]);
        return transmit((unsigned char *)req, strlen(req));
}
	
int CanonVCC3::doPanAndTilt()
{
        char req[20];
        strcpy(req, "0905120502");
        intToString(convert("pan",maxpan), 4, &req[strlen(req)]);
        intToString(convert("tilt",maxtilt), 4, &req[strlen(req)]);
        return transmit((unsigned char *)req, strlen(req));
}

int CanonVCC3::doZoom()
{
        char req[20];
        strcpy(req, "0701120202");
        intToString(convert("zoom",maxzoom), 4, &req[strlen(req)]);
	return transmit((unsigned char *)req, strlen(req));
}

int CanonVCC3::goHome()
{
	int x;
	char req[40]; 

	// Move the Pan/Tilter to home position.
	strcpy(req, "030511");
	x = transmit((unsigned char*)req, 6);
	// Zoom to home zoom position.
	zposn = homezoom;
	return doZoom();
}


int CanonVCC3::shutDown()
{
	/* There is no explicit powerOff command. (There is the "SoftwareReset" 	   cmd, but this turns off the power and then turns it on again and 
	   resets the control mode. So just close the port.  
       */ 
	
	portp->closePort();	
	return 0;
}


int CanonVCC3::doPower()
{
	return (doSoftwareReset());
}

int CanonVCC3::doSoftwareReset()
{
	char req[40]; 
	strcpy(req, "030801");
	return transmit((unsigned char *)req, 6);
}

int CanonVCC3::transmit(unsigned char *req, int len)
{
	int x,n;
        pthread_mutex_t *mut = portp->getPortMutex();

	// Calculate the checksum, write request and read reply until 
	// we get a response.

        x = calculateChecksum(req, len); 

	if (debugLevel > 2) 
	   displayString("Canon CMD", req, x);
	// Lock the port mutex.
	n = lockMutex(mut);
	if (n < 0)
	{
	    avail = 0;
	    return n;
        }
	writen(portp->getFileDesc(), req, x);	
	n = getReply(req);
	if (debugLevel > 2)
	{
	   printf("Canon::transmit--first getReply ret'd %d: avail = %d\n",
		n,avail);
	}
	if ((n < 0) && (debugLevel > 2))
	   printf("Canon VC-C3 received NACK/ERROR\n");
	setAvail(n);
	while (avail && (n!=2))
	   n = getReply(req);
	// Unlock the port.
	unlockMutex(mut);
	return n;
}

int CanonVCC3::getReply(unsigned char *buf)
{
	int x=0, n=0;

        /* Get ACK/NACK or response from the camera and return a
           code indicating type of reply:
               -1 => NACK/error; 0 => nothing read; 1 => ACK; 2 => response
	        3 => notification
           ACK/NACKs are always 4 bytes (byte0 = frame length = 3), and
           bit 7 of frame id (byte1) = 1.
           In an ACK/NACK frame, byte2 indicates whether ok or error--
           if byte2 == 0 => ACK (ok), else, NACK (error).

           Responses/notifications are of variable length; bit 7 of frame id
           (byte1) = 0.  Frame id is used to distinguish between
           ACKS/NACKS and responses/notifications.  To differentiate
           between responses and notifications, look at CID (cmd id)--
           bit5 = 0 for response; bit5=1 for notification.

           Try to read up to a maximum of 3 times.  */

	while ((x==0) && (n<3))
	{
	    x = readFromCanon(buf);
	    if (debugLevel > 2)
	       printf("Canon::getReply--readFromCanon ret'd x: %d\n",x);
	    ++n;
	}	

	// If we haven't read anything, set device to unavailable.
	setAvail(x);
	if (debugLevel > 2)
	{ 
	    printf("Canon::getReply--after readFromCanon--x: %d; avail: %d\n",
		x,avail);
	}

	/* If we read something, set device to available, and
	   return a code indicating the type of reply we received
	   (1 = ACK; -1 = NACK; 2 = RESPONSE; 3 = NOTIFICATION). 
	   Since we are returning in buf the actual reply, the
	   caller can detect/handle errors.  

	   Send an ACK if we receive a response or notification.  */

	if (x > 0)
	{
	    if ((x==4) && (buf[1] > 127))
	    {
		if (buf[2] == 0)
		    x = 1;
	        else 
		    x = -1;
	    }
	    else if ((x>3) && (x<16) && ((buf[1] & 128) == 0))
	    {
		unsigned char ack[4];
	        formatACK(buf,ack);
		writen(portp->getFileDesc(), ack, 4);
		if ((buf[2] & 32) == 0)
		     x = 2;
		else if ((buf[2] & 32) == 32)
		     x = 3;	
	    }		 
	    else
		return -1;	// got garbage or error
        }	
	return x;
}

int CanonVCC3::readFromCanon(unsigned char *buf)
{
	unsigned char c;
	fd_set f;
	struct timeval t;
	int n,i, len;
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

	// If select() returned value > 0, read 1 byte giving the
	// frame length and then read "frame length" number of bytes.

	if (n > 0)
	{
	  n = read(fd, &c, 1);
	  len = (int)c;
	  buf[0] = c;
	  i = 1;
	  if (len > 0)
	  {
	    n = readn(fd, &buf[i], len);
	    if (debugLevel > 2)
	       displayString("Canon reply",buf,len);
	    return (n+1);
	  } 
	} 
	return n;
}


int CanonVCC3::convert(const char *dof, float limit)
{
    float x;
    if (!strcmp(dof, "pan"))
    {
        x = pposn / limit * 782.0;
	x *= -1;
        if (x > 782.0)
            x = 782.0;
        else if (x < -782.0)
            x = -782.0;
        x += 32768.0;
    }  
    else if (!strcmp(dof, "tilt"))
    {
        if (tposn < 0)
            x = tposn / 30.0 * 266.0;
        else
            x = tposn / 25.0 * 217.0;
        if (x > 217.0)
            x = 217.0;
        else if (x < -266.0)
            x = -266.0;
        x += 32768.0;
    }
    else if (!strcmp(dof, "zoom"))
    {
        x = (zposn - 1.0) / 11.0 * zoom_limit;
        if (x < 0.0)
           x = 0.0;
        else if (x > zoom_limit)
           x = zoom_limit;
    }
    return doubleToInt((double)x);
}

int CanonVCC3::calculateChecksum(unsigned char *buf, int len)
{
    int i,x,sum = 0;

    x = asciiToPackedHex(buf, len);
    for (i=0; i<x; i++)
	sum += buf[i];	
    for (i=1; (256*i < sum); i++);
    buf[x] = 256 * i - sum;	
    return (x+1);	
}


void CanonVCC3::formatACK(unsigned char *response, unsigned char *ack)
{
    ack[0] = 3;
    ack[1] = 128 + response[1];
    ack[2] = 0;
    ack[3] = 256 - (ack[0] + ack[1] + ack[2]);
}

void CanonVCC3::getDesc(char *buf)
{
    strcpy(buf, "\"Canon VC-C3\" ");
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
