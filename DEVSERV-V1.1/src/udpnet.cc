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
 *  $Id: udpnet.cc,v 1.2 1999/07/19 21:10:08 mperry Exp $
 */


#include "udpnet.h"

UDPNet::UDPNet(int fd, int timeout, short p) : Network(fd,timeout,p)
{
}
    

UDPNet::~UDPNet()
{
}

int UDPNet::openSocket()
{
    // This opens a receiving UDP unicast socket.
    int i=1;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
	if (debugLevel > 0)
	  perror("Devserv: UDPNet--can't open socket");
	return (-1);
    }
    peer.sin_family = AF_INET;
    peer.sin_addr.s_addr = htonl(INADDR_ANY);
    peer.sin_port = htons(port); 
    /* If arg 3 (i) is not initialized, the socket may still show up
       when we run netstat   */ 
    if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char *)&i,(int)sizeof(i)) < 0) 
    {
	if (debugLevel > 0)
	   perror("Devserv: UDPNet--setsockopt failed for SO_REUSEADDR");
	return (-1);
    }
    if (bind(sock, (struct sockaddr *)&peer, sizeof(peer)) < 0)
    {
	if (debugLevel > 0)
	   perror("Devserv: UDPNet--bind failed");
	return (-1);
    }
    return 0;
}

int UDPNet::openSenderSocket(const char *host)
{
    struct hostent *hp;

    // This opens a sending UDP socket; can be used by a C++ client. 
    memset(&peer, 0, sizeof(peer));
    peer.sin_family = AF_INET;
    hp = gethostbyname(host);
    if (!hp) 
    {
	if (debugLevel > 0)
	   perror("Devserv: UDPNet--gethostbyname() failed");
	return (-1); 
    }
    memcpy(&peer.sin_addr, hp->h_addr, hp->h_length);
    peer.sin_port = htons(port);
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
	if (debugLevel > 0)
	   perror("Devserv: UDPNet::openSenderSock--socket() failed");
	return (-1);
    }
    memset(&sndr, 0, sizeof(sndr));
    sndr.sin_family = AF_INET;
    sndr.sin_addr.s_addr = htonl(INADDR_ANY);
    sndr.sin_port = htons(0);
    if (bind(sock, (struct sockaddr *)&sndr, sizeof(sndr)) < 0)
    {
	if (debugLevel > 0)
	   perror("Devserv: UDPNet::openSenderSock--bind() failed");
	return (-1);
    }
    return 0; 	
}
     	
