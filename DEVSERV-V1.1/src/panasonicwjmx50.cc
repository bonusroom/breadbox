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
 * $Id: panasonicwjmx50.cc,v 1.6 1999/08/10 19:54:50 mperry Exp $
*/


// panasonicwjmx50.cc: Implementation for the PanasonicWJMX50 WJ-MX50
//                     video mixer/switcher	

#include <math.h>
#include "panasonicwjmx50.h"

PanasonicWJMX50::PanasonicWJMX50(Serialport *port,const char *p,int num,int d,Mode m) 
	: Switcher(num, d, m)
{
	int x;
	avail = 1;
	if (debugLevel > 0)
	    fprintf(stderr,"INITIALIZING PANASONIC WJ-MX50\n");
	setPort(port);
	if (devmode == SIMULATED)
	    return;
	if (devnum == 1)
	{
	    portp->openPort(p);
	    int cflags = B9600 | CS8 | CSIZE;
	    x = portp->setupPort(cflags, 0, IGNBRK);
	    if (x < 0)
	    {
		if (debugLevel > 0)
		   fprintf(stderr,"PANASONIC INITIALIZATION FAILED\n");
		return;
	    }
	}
	if (portp->getFileDesc() < 0)
	{
	    if (debugLevel > 0)
	      fprintf(stderr,"INITIALIZATION FAILED FOR PANASONIC WJ-MX50\n");
            avail = 0;
	    return;
	}    	
}

void PanasonicWJMX50::setPort(Serialport *p)
{
	portp = p;
	assert(portp != NULL);
}

void PanasonicWJMX50::getDesc(char *buf)
{
     sprintf(buf, "\"Panasonic WJ-MX50\" ");
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
     strcat(buf, "off on@camera ");
     sprintf(&buf[strlen(buf)],"%d %d %d",busA,min_video_unit,max_video_unit);
     strcat(buf, "@pnp cam ");
     sprintf(&buf[strlen(buf)], "%d %d %d ",busB,min_video_unit,max_video_unit);
     strcat(buf, "@pnp sta ");
     if (pnpstate)
	   strcat(buf, "on ");
     else
	   strcat(buf, "off "); 
     strcat(buf, "on off@pnp tra ");
     sprintf(&buf[strlen(buf)], "%1.2f %1.2f ", xcoord, ycoord);
     strcat(buf, "0.00 1.00@pnp siz ");
     sprintf(&buf[strlen(buf)], "%1.2f %1.2f ", width, height);
     strcat(buf, "0.00 1.00"); 
}


int PanasonicWJMX50::move(char *cmd)
{
    struct cmdline {
	char dof[10];
	char field2[12];
        char field3[5];
	float value1;
	float value2;
    };
 	
    struct cmdline cmds[6];	// store multiple commands
    char *token;
    int i=0,x,n,cam=0,sta=0,siz=0,tra=0;

    // Algorithm:  1) separate commands by "@" and fill out fields of struct(s);
    //             2) since we can only send one cmd at a time, walk the
    //	              array of commands and send request to device.	
   
    token = strtok(cmd, "@");
    if (debugLevel > 2)	
       printf("PanasonicWJMX50::move: CMD: %s;  AVAIL: %d\n", cmd, avail); 	
    n=sscanf(token, "%s%s%s%f%f",cmds[i].dof,cmds[i].field2,cmds[i].field3,&cmds[i].value1,&cmds[i].value2);
    i++;
    while ((token = strtok(NULL, "@")) !=NULL)
    {
        n=sscanf(token, "%s%s%s%f%f",cmds[i].dof,cmds[i].field2,cmds[i].field3,&cmds[i].value1,&cmds[i].value2);
	i++;
    }
    for (int j=0; j<i; j++)
    {
      if (!strcmp(cmds[j].dof,"power")||!strcmp(cmds[j].dof,"pow"))
      {
	power = stringToInt(cmds[j].field3);
	if (devmode != SIMULATED)
	   x = doPower(); 
      } 
      else if (!strcmp(cmds[j].dof, "home"))
      { 	
	if (devmode != SIMULATED)
	   x = goHome();	
      }
      else if ((cmds[j].dof[0]=='c')&&(cmds[j].dof[1]=='a')&&(cmds[j].dof[2]=='m'))
      {	
	busA = atoi(cmds[j].field3);
	if (devmode != SIMULATED)
	   x = doCrosspoint('A');	
      }	
      else if (!strcmp(cmds[j].dof, "pnp"))
      {
	if ((cmds[j].field2[0]=='c')&&(cmds[j].field2[1]=='a')&&(cmds[j].field2[2]=='m'))
	{
	    if (busB != atoi(cmds[j].field3))
	    {
	       cam = 1;	
	       busB = atoi(cmds[j].field3);
	    }
	}
	else if (!strcmp(cmds[j].field2,"sta")||!strcmp(cmds[j].field2,"state"))
	{
	    x = stringToInt(cmds[j].field3);
	    if (((x==0)||(x==1)) && (pnpstate!=x))
	    {
	       sta = 1;
	       pnpstate = x;	
	    }
	}
	else if ((n==5)&&!strcmp(cmds[j].field2,"tra")||!strcmp(cmds[j].field2,"translate"))
	{
	    tra = 1;
	    strcpy(type, cmds[j].field3);
	    setCoord(cmds[j].value1, cmds[j].value2);	
	}
	else if ((n==5)&&!strcmp(cmds[j].field2,"siz")||!strcmp(cmds[j].field2,"size"))
	{
	    siz = 1;
	    strcpy(type, cmds[j].field3);
	    setSize(cmds[j].value1, cmds[j].value2);
	}  
      }	
      else if (!strcmp(cmds[j].dof, "shutdown") && avail && (devmode!=SIMULATED))
	x = shutDown();
    }
    if (devmode == SIMULATED)
	return 1;
    if (cam)
	doCrosspoint('B');
    if (sta) 
    {
	if (pnpstate)
 	    x=doPnp();
	else
	    setMixWipeLever("00","10","10");
    }	
    if (!sta && siz && pnpstate)
	x = doMixWipeLever();
    if (!sta && tra && pnpstate)	
	   x = doPositioner();
    return x;
}

int PanasonicWJMX50::sendPowerCmd()
{
	char cmd[6];
	cmd[0] = '\x02';
	cmd[1] = 'P';
	cmd[2] = 'O';
	cmd[3] = 'F';
	cmd[4] = '\x03';
        return transmit((unsigned char *)cmd,5);
}

int PanasonicWJMX50::doPower()
{
	int n;

	// Send doQuery() to turn on the device.  Then either leave it on
	// or send POF to turn if off.
	n = doQuery(0);
	if (!power)
	   n = sendPowerCmd();
	return n;			
}


int PanasonicWJMX50::shutDown()
{
	power = 0;	
	int x = 0;
	if (avail)
	   x = doPower();
	portp->closePort();
	return x;
}

int PanasonicWJMX50::doCrosspoint(char bus)
{
        char cmd[9]; 
	cmd[0] = '\x02';
	cmd[1] = 'V';
	cmd[2] = 'C';
	cmd[3] = 'P';
	cmd[4] = ':';
  	cmd[5] = bus;
	if (bus == 'A')
	    cmd[6] = busA + 48;
	else
	    cmd[6] = busB + 48;
	cmd[7] = '\x03'; 
	if (debugLevel > 2)
	    displayString("Panasonic crosspoint cmd",(unsigned char *)cmd,8); 
	return transmit((unsigned char *)cmd, 8);
}


int PanasonicWJMX50::goHome(void)
{
	setCoord(0.5, 0.5);
	setSize(0.5, 0.5);
	pnpstate = 0;
	setMixWipeLever("00","10","10");
	if (!avail)
	{	
	  if (debugLevel > 0) 
	    fprintf(stderr,"PANASONIC NOT RESPONDING...CHECK CONNECTION/POWER\n");
  	  return -1; 	
        }
	busA = home_srcA;
	busB = home_srcB;
        doCrosspoint('A');
        return doCrosspoint('B');
}

void PanasonicWJMX50::setCoord(float x, float y)
{
	if (type[0] == 'A')
	{
	     xcoord = x;
	     ycoord = y;
	}
	else if (type[0] == 'R')
	{
	     xcoord += x;
	     ycoord += y;
	}
	if (xcoord < 0.0)
	     xcoord = 0.0;
	else if (xcoord > 1.0)
	     xcoord = 1.0;
	if (ycoord < 0.0)
	     ycoord = 0.0;
	else if (ycoord > 1.0)
	     ycoord = 1.0;
}


void PanasonicWJMX50::setSize(float w, float h)
{
	if (type[0] == 'A')
	{
	     width = w;
	     height = h;
	}
	else if (type[0] == 'R')
	{
	     width += w;
	     height += h;
	}
	if (width < 0.0)
	     width = 0.0;
	else if (width > 1.0)
	     width = 1.0;
	if (height < 0.0)
	     height = 0.0;
	else if (height > 1.0)
	     height = 1.0;
}


int PanasonicWJMX50::doPnp()
{
	setWipeMode("12", "ZM", '1');
	doMixWipeLever();
	return doPositioner();
}


int PanasonicWJMX50::doMixWipeLever()
{
	char buf1[4], buf2[4], buf3[4];
	int n = getSize();
	intToString(n,2,buf1);
	n = getTransCoord(ycoord);
	intToString(n,2,buf2);
	n = getTransCoord(xcoord);
	intToString(n,2,buf3);
	return setMixWipeLever(buf1, buf2, buf3);
}

int PanasonicWJMX50::doPositioner()
{
	char buf1[4], buf2[4];
	int n = getTransCoord(ycoord);
	intToString(n,2,buf1);
	n = getTransCoord(xcoord);
	intToString(n,2,buf2);
	return setPositioner(buf1, buf2);
}
	

int PanasonicWJMX50::setWipeMode(char *wipeno, char *pattern, char mode)
{
	char cmd[12];
	
	cmd[0] = '\x2';
	cmd[1] = 'V';
	cmd[2] = 'W';
	cmd[3] = 'P';
	cmd[4] = ':';
	/* P. 28-29 of Users' Manual shows wipe patterns 12-15 for Square Wipe
	   Button; p. 37 of Users' Manual says to press the square wipe button. */
	cmd[5] = wipeno[0];
	cmd[6] = wipeno[1];
	cmd[7] = pattern[0];
	cmd[8] = pattern[1];
	cmd[9] = mode;
	cmd[10] = '\x3';
	return transmit((unsigned char *)cmd, 11);
}

int PanasonicWJMX50::setMixWipeEffect()
{
	char cmd[10];
	cmd[0] = '\x2';
	cmd[1] = 'V';
	cmd[2] = 'M';
	cmd[3] = 'X';
	cmd[4] = ':';
	cmd[5] = 'D';
	cmd[6] = 'R';
	cmd[7] = '\x3';
	return transmit((unsigned char *)cmd, 8);
}


int PanasonicWJMX50::setMixWipeLever(char *size, char *y, char *x)
{
    char cmd[13];
    cmd[0] = '\x2';
    cmd[1] = 'V';
    cmd[2] = 'M';
    cmd[3] = 'P';
    cmd[4] = ':';
    cmd[5] = size[0];
    cmd[6] = size[1];
    cmd[7] = y[0];
    cmd[8] = y[1];
    cmd[9] = x[0];
    cmd[10] = x[1];
    cmd[11] = '\x3';
    if (debugLevel > 2)
	displayString("Panasonic setMixWipeLevel cmd",(unsigned char *)cmd,12);
    return transmit((unsigned char *)cmd, 12);
}

int PanasonicWJMX50::setPositioner(char *y, char *x)
{
    char cmd[12]; 
    cmd[0] = '\x2';
    cmd[1] = 'V';
    cmd[2] = 'P';
    cmd[3] = 'S';
    cmd[4] = ':';
    cmd[5] = 'N';
    cmd[6] = y[0];
    cmd[7] = y[1];
    cmd[8] = x[0];
    cmd[9] = x[1];
    cmd[10] = '\x3';
    if (debugLevel > 2)
      displayString("Panasonic setPositioner cmd",(unsigned char *)cmd,11);
    return transmit((unsigned char *)cmd, 11);
}


int PanasonicWJMX50::getSize()
{
    float size = (width + height) * 127.5;
    return doubleToInt((double)size);
}

int PanasonicWJMX50::getTransCoord(float coord)
{
    float size = (width + height) / 2.0;
    float x = coord / (1.0 - size) * 255.0;
    int n = doubleToInt((double)x);
    if (n > 255)
        n = 255;
    return n;
}

int PanasonicWJMX50::transmit(unsigned char *buf, int len)
{
    int i,n;
    unsigned char reply[1024];

    if (debugLevel > 2)
         displayString("PanasonicWJMX50 CMD", buf, len);
    i = readFromSwitcher(reply);
    if (debugLevel > 2)
	printf("PanasonicWJMX50::transmit--readFromSwitcher ret'd %d\n",i);	
    i = writen(portp->getFileDesc(), buf, len);
    i=0;
    while (i<40)
    {
        n = readFromSwitcher(reply); 
	if (n > 0)
	{
	   if (debugLevel > 2)
	       displayString("PanasonicWJMX50 reply", reply, n);
	   setAvail(n);
	   if (debugLevel > 2)
	   {
	       printf("PanasonicWJMX50::transmit--returning %d; avail: %d\n",
		    n,avail);
	   }
	   return n;
        }
	++i;
     }
     setAvail(n);
     if (debugLevel > 2) 
	printf("PanasonicWJMX50::transmit--returning %d; avail: %d\n",n,avail);
     return n;
}


int PanasonicWJMX50::readFromSwitcher(unsigned char *buf)
{
     unsigned char c = '\0';
     int n,i=0;	
     fd_set f;
     struct timeval t;
     int fd = portp->getFileDesc();

     t.tv_sec = 0;
     t.tv_usec = 300000;
     FD_ZERO(&f);
     FD_SET(fd,&f);
     n = select(fd+1, &f, NULL, NULL, &t);
     if (n > 0)
     {
        // Read until there is data and the first byte is 'stx'.
        // This throws away ACKs (6h), NAKs (15h), and '#' (23h)
        // that is sent periodically.
        
        n = read(fd, &c, 1);
	if (debugLevel > 2)	
	{
	   printf("PanasonicWJMX50: readFromSwitcher--first read() ret'd %d,",n);
	   printf(" c = %c\n",c);
	}
	if (n)
	   avail = 1;
	while (n && (char)c != '\x2')
	{
	   n = read(fd, &c, 1);
	   if (n < 0) 
	      return n;
	}
	// If we're here, we received 'stx', so read until 'etx'.
	if ((char)c == '\x2')
	{
	      buf[i++] = c;
	      while ((char)c!='\x3')
	      {
	     	n = read(fd, &c, 1);
	     	if (n == -1)	
	    	  return n;
	        if (n == 1)
		{
	          buf[i++] = c;
		  if (debugLevel > 2)
		  {
		     printf("PanasonicWJMX50: readFromSwitcher--buf[%d] = %c\n",
			(i-1),buf[i-1]);
		  }
		}
	      }
	 }
	 return i;
   }
   return n;
}


int PanasonicWJMX50::doQuery(int doRead)
{
	char buf[8];
	buf[0] = '\x02';
	buf[1] = 'Q';
	buf[2] = 'L';
	buf[3] = 'S';
	buf[4] = '\x03';
	transmit((unsigned char *)buf, 5);
	if (doRead)
	{
	   char *reply = new char[1024];
	   int i = readFromSwitcher((unsigned char *)reply);
	   delete [] reply;
	}
	return 0;
} 
