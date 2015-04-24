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
 *  $Id: ipnet.cc,v 1.2 1999/07/19 21:10:07 mperry Exp $
 */


#include "ipnet.h"

IPNet::IPNet(int fd, int timeout, short p) : Network(fd, timeout, p)
{
     setAddr("224.35.36.37");	
     setTTL(1); 
}

IPNet::~IPNet()
{
}


int IPNet::openSocket()
{
    sock = makeMulti();
    if (sock < 0)
	return (-1);	
    return 0;
}

int IPNet::makeMulti()
{
    char val;
    long on = 1;
    struct ip_mreq mreq;
    unsigned long addr = convertAddr(group_addr);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
	if (debugLevel>0)
	   perror("IPNet: socket()");
	return (-1);
    }
    peer.sin_family = AF_INET;
    peer.sin_port = htons(port);
    memset(&(peer.sin_addr), 0, sizeof(struct in_addr));
    peer.sin_addr.s_addr = INADDR_ANY;

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
    {
	if (debugLevel>0)
	   perror("IPNet:setsockopt: SO_REUSEADDR");
	close(sock);
	return (-1);
    }

    if (bind(sock, (struct sockaddr *)&peer, sizeof(peer)) < 0)
    {
	if (debugLevel>0)
	   perror("Devserv: IPNet--bind failed");
	return (-1);
    }
    peer.sin_addr.s_addr = addr;

    val = 0;	// turn off loopback
    /*if (setsockopt(sock,IPPROTO_IP, IP_MULTICAST_LOOP,(char *)&val,sizeof(val))<0)
    {
	if (debugLevel>0)
	   perror("Devserv: IPNet--setsockopt failed for IP_MULTICAST_LOOP");
	close(sock);
	return (-1);
    }  */ 

    val = ttl;
    if (setsockopt(sock,IPPROTO_IP,IP_MULTICAST_TTL,(char *)&val,sizeof(val)) < 0)
    {
	if (debugLevel>0)
	   perror("IPNet: setsockopt--IP_MULTICAST_TTL");
	close(sock);
	return (-1);
    }
    mreq.imr_multiaddr.s_addr = addr;
    mreq.imr_interface.s_addr = INADDR_ANY;
    if (setsockopt(sock,IPPROTO_IP,IP_ADD_MEMBERSHIP, (char *)&mreq,sizeof(mreq)) < 0)
    {
	if (debugLevel>0)
	   perror("Devserv: IPNet--setsockopt failed for IP_ADD_MEMBERSHIP");
	close(sock);
 	return (-1);
    }
    return sock;
} 	
     		

unsigned long IPNet::convertAddr(const char *s)
{
    // If group_addr is in dotted decimal, convert to 32-bit IP address.
    if (isdigit(*s))
	return (unsigned long)inet_addr(s);
    else
    {
        struct hostent *hp = gethostbyname(s);
	if (!hp)
	    return 0;
	return *((unsigned long **)hp->h_addr_list)[0];
    }
}	

void IPNet::setAddr(const char *s)
{
    strcpy(group_addr, s);	
    if ((debugLevel==2) || (debugLevel==4)) 
      printf("Devserv: IPNet::setAddr: s: %s, group_addr: %s\n",s, group_addr);
}  

void IPNet::setTTL(int t)
{
    if (t < 0)
	ttl = 0;
    else if (t > 127)
	ttl = 127;
    else
	ttl = t;
}
