/* Copyright (c) 1999 Regents of the University of California.
 * All rights reserved.
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
 * $Id: sonyevid30.cc,v 1.5 1999/08/05 01:39:51 mperry Exp $
*/

#include <string.h>
#include <math.h>
#include "sonyevid30.h"

short SonyEVID30::cmdcount[MAX_PORTS][MAX_SONY];

SonyEVID30::SonyEVID30(Serialport *port,const char *p,int num,short d,short l,Mode m)
	: Camera(num, d, l, m)	
{
	int x;

	if (debugLevel > 0)
	  fprintf(stderr,"INITIALIZING SONY EVI-D30 (address: %d)\n",devnum);
	avail = 0;
	setPort(port);
	int id = portp->getID();
	cmdcount[id][devnum-1] = 0;  
	if (devmode == SIMULATED)
	{
	  setDefaults();
	  return;
        }		
	if (devnum == 1)
	{
	   portp->openPort(p);
	   int cflags = B9600 | CS8 | CSIZE;  
	   x = portp->setupPort(cflags, 0, IGNBRK); 
	   if (x < 0)
	   {
	     // Don't exit in case other devices are functional. 
	     if (debugLevel > 0)
	       fprintf(stderr,"CAN'T SET UP PORT FOR THE SONY EVI-D30 (address: %d)\n",devnum);
	     initialized = 0;
	     return;
	   }
	   int fd = portp->getFileDesc();	
	   if (fd < 0)
	   {
	     if (debugLevel > 0)
	       fprintf(stderr,"CAN'T OPEN A PORT FOR THE SONY EVI-D30 (address: %d)\n",devnum);
	     initialized = 0;
	     return;
	   }	
	   
	   x = init();
	   if (x <= 0)
           {
	       initialized = 0;
	       if (debugLevel > 0)
	          fprintf(stderr,"INITIALIZATION FAILED FOR SONY EVI-D30 (address: %d)\n",devnum);
	       return;
  	   }
	}
	setDefaults();
}


void SonyEVID30::setDefaults()
{
	initialized = 1;
	avail = 1;
	if (devnum == 1)
	{
	    minpan = -100.0;
	    maxpan = 100.0;
	    mintilt = -25.0;
	    maxtilt = 25.0;	
	    min_pspeed = 1.0;
	    max_pspeed = 24.0;
	    min_tspeed = 1.0;
	    max_tspeed = 20.0; 
	    min_zspeed = 34.0;
            max_zspeed = 55.0;
	    minzoom = 1.0;
	    maxzoom = 12.0;
	}
        pcoeff = 57.4;
	pexp = -0.182;
	tcoeff = 40.44;
	texp = -0.12;
	zcoeffw = 9.65;
	zcoeffh = 7.885;
	zexpw = -0.213;
	zexph = -0.196;	
	if (lens && (debugLevel > 0))	
	{
	    fprintf(stderr, "WARNING: You specified a wide-angle lens ");
	    fprintf(stderr, "for the Sony EVI-D30, and it's calibrated ");
	    fprintf(stderr, "for a regular lens only."); 
	}	
	pspeed = max_pspeed;
	tspeed = max_tspeed;
	zspeed = max_zspeed;
	pposn = homepan;
	tposn = hometilt;
	zposn = homezoom;
	strcpy(panspeed, "24");
	strcpy(tiltspeed, "20");
}
	
void SonyEVID30::setPort(Serialport *p)
{
    portp = p;
    assert(portp != NULL); 	
}

int SonyEVID30::init()
{
	int n;
	char cmd1[] = { "88010001FF" };
	char cmd2[] = { "883001FF" }; 
	n = transmit((unsigned char *)cmd1, strlen(cmd1));
	n = transmit((unsigned char *)cmd2, strlen(cmd2));
	clearInput(portp->getFileDesc());
	return n;
}

void SonyEVID30::setPtSpeedfield(char c)
{
    if (c=='p')
       intToString((int)pspeed,2,panspeed);
    else if (c=='t')
       intToString((int)tspeed,2,tiltspeed);
}


int SonyEVID30::doZoomSpeed(float amt)
{
    char buf[15];
    sprintf(buf, "8%d", devnum);
    strcat(buf, "010407");	
    intToString((int)amt,2,&buf[strlen(buf)]);
    strcat(buf, "FF");	
    return (transmit((unsigned char *)buf, strlen(buf)));  	
}

int SonyEVID30::move(char *cmd)
{
    struct Cmdline cmds[6];  // store multiple commands
    int i, j, n=0, pt=0, pspd=0, tspd=0, zspd=0;
    int id;

    if (!initialized || !portp->getFileDesc())
	return -1;
    id = portp->getID();
    if (debugLevel > 2)
    {
       printf("Sony (address %d)--move: cmd: %s; cmdcount[%d][%d]: %d\n",
	  devnum,cmd,id,(devnum-1),cmdcount[id][devnum-1]); 	
    }
   
    i = parseCmd(cmd, cmds, &pt, &pspd, &tspd, &zspd); 
    if (pspd)
	setPtSpeedfield('p');
    if (tspd)
	setPtSpeedfield('t');	
    if (devmode == SIMULATED)
	return 1;
    for (j=0; j<i; j++)
    {
	if (!strcmp(cmds[j].axis, "zoom"))
	    n = doZoom();
	else if (!strcmp(cmds[j].axis,"power")||!strcmp(cmds[j].axis,"pow"))
	{
	    power = stringToInt(cmds[j].value);
	    n = doPower();
	}
	else if (!strcmp(cmds[j].axis, "shutdown") && avail)
	    n = shutDown();
	else if (!strcmp(cmds[j].axis, "home"))
	    n = goHome();
	else if (pt&&(!strcmp(cmds[j].axis,"pan")||!strcmp(cmds[j].axis,"tilt")))
	{
	    n = doPanAndTilt();
	    pt = 0;
	}
    }	
    if ((n < 0) && (debugLevel>0)) 
	fprintf(stderr, "SONY EVI-D30 (address: %d) NOT RESPONDING...CHECK CONNECTION/POWER\n",devnum);
    return n;	
}

int SonyEVID30::transmit(unsigned char *buf, int len)
{
	int l,x,n=0;
	unsigned char reply[20]; 
        fd_set f;
        struct timeval t;
	int fd = portp->getFileDesc();
	int id = portp->getID();
	pthread_mutex_t *port_mutex = portp->getPortMutex();
 
	x = lockMutex(port_mutex);
        t.tv_sec = 0;
        t.tv_usec = 100000;	

        l = asciiToPackedHex(buf,len);
	if (debugLevel > 2)
	   displayString("SONY EVI-D30 CMD", buf, l);
   	if (debugLevel > 2)
        {
	   printf("Sony (address: %d)--transmit: cmdcount[%d][%d]: %d\n",
		devnum,id,(devnum-1),cmdcount[id][devnum-1]); 
        }

        /* We can write up to 2 commands to each of the Sony EVI-D30's
	   buffers, but if more than 2 commands are sent, the Sony sends
	   a "buffer-full" error message.  So increase the count each
	   time a command is written and decrease the count each time a
	   completion reply is received from the Sony.  Any Sony in
	   a daisy-chain can read the replies to any other Sony in that
	   chain.  So all Sonys on one port have a row of the 2-dimensional
	   array assigned, based on "id".  Each column in the row corresponds
	   to the devnum (internal address).  Any device can decrement the
	   command count.  The device with the specified address looks at
	   at its own command count (if > 1, reads before transmitting)    
	   and increments its own count when it transmits a request.
        */

        while ((cmdcount[id][devnum-1] > 1) && (n<3)) 
	{
	    // readFromSony returns: -1: read() failed, 0: nothing read,
	    //		             >0: number of bytes read
	    x = readFromSony(reply);	
	    if (!strcmp((char *)buf,(char *)reply))
		x = -1;
	    ++n;
	}
        x = writen(fd, buf, l);
	if ((char)buf[0] != '\x88')
	{
	    // Don't increment cmdcount for IF_CLEAR & BROADCAST_ADDRESS cmds.
            ++cmdcount[id][devnum-1];
	}
	if (debugLevel > 2)
        {
	   printf("Sony (%d): wrote cmd--cmdcount[%d][%d]: %d\n",
		devnum,id,(devnum-1),cmdcount[id][devnum-1]); 
        }
	n = 0;
	x = readFromSony(reply);	
	if (!strcmp((char *)buf,(char *)reply))
	    x = -1;
        while ((n<3) && (x==0)) 	
        {
   	    FD_ZERO(&f);
   	    FD_SET(fd,&f);	
   	    select(0, NULL, NULL, NULL, &t);
	    x = readFromSony(reply);	
	    if (!strcmp((char *)buf,(char *)reply))
		x = -1;
	    ++n;
	}
	setAvail(x);
	if (debugLevel > 2)
	{
	   printf("Sony (address %d): transmit returning %d; avail = %d\n",
		devnum,x,avail);
	}
	n = unlockMutex(port_mutex);
	return x;
}


int SonyEVID30::readFromSony(unsigned char *buf)
{
   unsigned char c = '\0'; 
   int n,i=0,cam,s;		
   fd_set f;
   struct timeval t;
   int fd = portp->getFileDesc();
   int id = portp->getID();	
 
   t.tv_sec = 0;
   t.tv_usec = 100000;	
   FD_ZERO(&f);
   FD_SET(fd, &f); 
   s = select(fd+1, &f, NULL, NULL, &t);
   if ((s < 0) && (debugLevel > 0))
   {	
	perror("select failed");
	fprintf(stderr," errno: %d\n", errno);
   }	
   if (s > 0)
   {
       // Read until we get the terminating byte, \xff.
       while ((char)c != '\xff')
       {       
  	  n = read(fd, &c, 1);
	  if (n == -1)
	      return n; 
	  if (n==1)
	      buf[i++] = c;
       }	
       if ((debugLevel == 2) || (debugLevel == 4))
       {
	  printf("SONY EVI-D30 (%d) REPLY",devnum);
          displayString("", buf, i);
       }
       if (((char)buf[1]=='\x51')||((char)buf[1]=='\x52'))
       {
	  cam = (buf[0] >> 4) - 8;
	  --cmdcount[id][cam-1];	
       }
       if (debugLevel > 2)
       { 
         if (((char)buf[2]=='\x60')||((char)buf[1]=='\x61')||((char)buf[1]=='\x62'))
  	     printf("SONY EVI-D30 BUFFERS FULL\n");
       }
       return i; 
   }
   return s;
}


int SonyEVID30::convert(const char *dof, float limit)
{
    float x;

    if (!strcmp(dof,"pan"))
    {
        x = pposn / limit * 880.0;
        if (x > 880.0)
	    x = 880.0;
 	else if (x < -880.0)
	    x = -880.0;
    }
    else if (!strcmp(dof,"tilt"))
    {
        x = tposn / limit * 300.0;
	if (x > 300.0)
	   x = 300.0;
	else if (x < -300.0)
	   x = -300.0;
    }
    else if (!strcmp(dof,"zoom")) 
    { 
        x = (zposn - 1.0) / 11.0 * 1023.0; 
	if (x < 0.0)
	    x = 0.0;
	else if (x > 1023.0)
	    x = 1023.0;
    }
    return doubleToInt((double)x);
}


int SonyEVID30::doPanAndTilt()
{
    char buf[40];
    sprintf(buf, "8%d010602%s%s",devnum,panspeed,tiltspeed);
    /*sprintf(buf, "8%d", devnum);	
    strcat(buf, "010602");
    strcat(buf, panspeed);
    strcat(buf, tiltspeed); */	
    make0XString(convert("pan",maxpan), &buf[strlen(buf)] ,4);
    make0XString(convert("tilt",maxtilt), &buf[strlen(buf)], 4);
    strcat(buf, "FF");
    return transmit((unsigned char *)buf, strlen(buf));
}


int SonyEVID30::doZoom()
{
    char buf[40];
    sprintf(buf, "8%d", devnum);
    strcat(buf, "010447");	
    make0XString(convert("zoom",maxzoom), &buf[strlen(buf)], 4);
    strcat(buf, "FF");
    return transmit((unsigned char *)buf, strlen(buf));
}


int SonyEVID30::panTiltPosInq()
{
    char buf[12];
    sprintf(buf, "8%d", devnum);
    strcat(buf, "090612FF");
    return transmit((unsigned char *)buf, strlen(buf));
}

int SonyEVID30::zoomPosInq()
{
    char buf[12];
    sprintf(buf, "8%d", devnum);
    strcat(buf, "090447FF");
    return transmit((unsigned char *)buf, strlen(buf));
}

void SonyEVID30::checkPower()
{
    if (powerInq() != 1)
    {
	power = 1;
	doPower();
    }
}
 

int SonyEVID30::powerInq()
{
    char buf[12];
    int x=2,y=1;

    sprintf(buf, "8%d", devnum);
    strcat(buf, "090400FF");
    transmit((unsigned char *)buf, strlen(buf));
    while ((x > 1) && (y<4))	
    {
    	x = getPowerInqReply();	
   	y++;
    } 
    return x;	
}


int SonyEVID30::getPowerInqReply()
{
    unsigned char buf[40];

    int x = readFromSony(buf);
    if (!verifyDeviceNumber(buf[0]))
	return 2;
    if (x>0)
    {
	if ((x==4)&&(buf[1]=='\x50')&&(buf[2]=='\x2'))
	    return 1;           // power is on 
	else if ((x==4)&&(buf[1]=='\x50')&&(buf[2]=='\x3'))
	    return 0;           // power is off 
	else if ((buf[1]=='\x61') || (buf[1]=='\x62'))
	    return -1;		// got error message 		
	else
	    return 2;	        // got inapplicable message
    }
    return x;
}

int SonyEVID30::verifyDeviceNumber(unsigned char c)
{
    unsigned char buf[3];
    buf[0] = c;
    buf[0] >>=4;
    if ((int)buf[0] == (devnum+8))
	return 1;
    else
	return 0;
} 	

	

int SonyEVID30::doPower()
{
    char buf[13];
    int x;	

    sprintf(buf, "8%d", devnum);
    if (power)	
 	strcat(buf, "01040002FF");
    else
	strcat(buf, "01040003FF");
    x = transmit((unsigned char *)buf, strlen(buf));
    sleep(3);
    return x;	
}

int SonyEVID30::shutDown()
{
    int x = 0;
    if (avail)
    {
    	power = 0;
    	x = doPower();
    }	
    if (devnum == 1)
	portp->closePort();
    return x;
}


int SonyEVID30::goHome()
{
    int x;
    pposn = homepan;
    tposn = hometilt;
    x = doPanAndTilt();
    if (x < 0)
	return x;
    zposn = homezoom;
    x = doZoomSpeed(max_zspeed);
    x = doZoom();	
    return x;
}	 


void SonyEVID30::getDesc(char *buf)
{
    strcpy(buf, "\"Sony EVI-D30\" ");
    if (!avail)
    {	
	strcat(buf, "NOT_RESPONDING");
 	return;
    }
    strcat(buf, "power ");
    if (power)
	strcat(buf, "on ");
    else
	strcat(buf, "off ");
    strcat(buf, "off on@pan ");
    sprintf(&buf[strlen(buf)],"%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f",
	pposn,minpan,maxpan,pspeed,min_pspeed,max_pspeed);
    strcat(buf, "@tilt ");
    sprintf(&buf[strlen(buf)],"%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f",
	tposn,mintilt,maxtilt,tspeed,min_tspeed,max_tspeed);
    strcat(buf, "@zoom ");
    sprintf(&buf[strlen(buf)],"%1.2f %1.2f %1.2f",zposn,minzoom,maxzoom);   
}
