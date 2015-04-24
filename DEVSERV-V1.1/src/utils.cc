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
 *  $Id: utils.cc,v 1.2 1999/07/19 21:10:08 mperry Exp $
 */


#include "utils.h"

int asciiToPackedHex(unsigned char *buf, int len)
{
        int i,j;

        for (i=0,j=0; i<(len); i++,j++)
        {
            if (buf[i] >= '0' && buf[i] <= '9')
                buf[j] = buf[i] - 48;
            else
                buf[j] = buf[i] - 55;
            buf[j] <<= 4;
            i++;
            if (buf[i] >= '0' && buf[i] <= '9')
                buf[j] |= buf[i] - 48;
            else
                buf[j] |= buf[i] - 55;
        }
        return j;
}



int make0XString(int pos, char *buf, int len)
{
       char temp[80];
       int i,j;

       // This function creates a string in the form: "0Y0Y0Y0Y"
       // where YYYY is the hex representation of "pos".
       // This is needed for the pan and tilt fields of message to 
       // the Sony EVI-D30.

       sprintf(buf, "%X", pos);

       if ((int)strlen(buf) < len)
       {
          j = len - strlen(buf);
          for (i=0; i<j; i++)
               temp[i] = '0';
          strcpy(&temp[i],buf);
       }
       else
       {
          j = strlen(buf) - len;
          strcpy(temp, &buf[j]);
       }
       for (i=0,j=0; temp[i]!='\0';i++,j++)
       {
          buf[j] = '0';
          j++;
          buf[j] = temp[i];
       }
       buf[j] = '\0';
       return 0;
}


void intToString(int n, int len, char *buf)
{
	/* Convert the first int argument into an upper-case string
	   representing n as hex chars.  Prepend as many '0's as needed
	   to reach 'len' length.  */ 

	int i, j, l;
	sprintf(buf, "%X", n);
	if ((int)strlen(buf) < len)
	{
	    l = len - strlen(buf);
	    for (i=0,j=l; j<len; i++,j++)
		buf[j] = buf[i];
	    buf[j] = '\0';	
	    for (i=0; i<l; i++)
		buf[i] = '0';
	}
}


int doubleToInt(double x)
{
        double y = 0.0;
        double f = modf(x, &y);
        if (f < -0.5)
            return ((int)x-1);
	else if ((f<0) && (f>-0.5))
	    return ((int)x);
	else if ((f>=0) && (f<0.5))
	    return ((int)x); 
        else
            return ((int)x + 1);
}



int stringToInt(char *str)
{
        if (!strcmp(str, "on"))
                return 1;
        else if (!strcmp(str, "off"))
                return 0;
        else
                return atoi(str);
}


int writen(int fd, const unsigned char *buf, int len)
{
	int n, nleft=len;
	while (nleft > 0)
	{
	   n = write(fd, buf, nleft);
	   if (n < 0)
	   {
		perror("write failed");
		return n;
	   }
	   nleft -= n;
	   buf += n;
	}
	return (len - nleft);
}


int readn(int fd, unsigned char *buf, int len)
{
	// Read "len" number of bytes (modeled after R. Stevens, 
	// "UNIX Network Programming").

	int n, nleft=len;

	while (nleft > 0)
	{
	    n = read(fd, buf, nleft);
	    if (n < 0)
	    {
		perror("Devserv: read() failed");
		return n;
	    }
	    nleft -= n;
	    buf += n;
	}
	return (len-nleft);
}		 	
	

void displayString(char *name, unsigned char *buf, int len)
{
	printf("%s: ", name);
	for (int i=0; i<len; i++)
		printf("%X ", buf[i]);
	printf("\n");
}
