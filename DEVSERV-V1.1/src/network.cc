/* 
 * Copyright (c) 1999 Regents of the University of California.
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
 * $Id: network.cc,v 1.6 1999/08/10 20:33:47 mperry Exp $
*/

#include "network.h"

extern int h_errno;

pthread_mutex_t Network::rmutex;
pthread_mutex_t Network::smutex;

Network::Network(int fd, int s, short p)
{
    sock = fd;
    seconds = s;	
    port = p;
    memset(&peer, 0, sizeof(peer));	
    int status = pthread_mutex_init(&rmutex, NULL);
    assert(status == 0);	
    status = pthread_mutex_init(&smutex, NULL);
    assert(status == 0);	
}

Network::~Network()
{
    close(sock);
}

void Network::setPort(short p)
{
    port = p;
}

void Network::getLocalname(char *host)
{
#ifdef sun 
    sysinfo(SI_HOSTNAME, host, (long)256);
    struct hostent *hp = gethostbyname(host);
    if (hp)
	strcpy(host, hp->h_name);
#else
    char buf[256];
    gethostname(buf,256);
    strcpy(host, buf);
#endif  
}
       
	
void Network::getRemoteHost(char *host)
{
    struct hostent *hp=NULL;
    struct in_addr inaddr;
    int err;

    inaddr.s_addr = peer.sin_addr.s_addr;

    err = pthread_mutex_lock(&rmutex); 
    if ((err!=0) && ( (debugLevel==2)||(debugLevel==4)))	
	printf("Network::getRemoteHost--can't lock receive mutex\n");
    hp = gethostbyaddr((char *)&peer.sin_addr,sizeof(peer.sin_addr),AF_INET); 
    err = pthread_mutex_unlock(&rmutex);
    if ((err!=0) && ( (debugLevel==2)||(debugLevel==4)))	
	printf("Network::getRemoteHost--can't unlock receive mutex\n");

    if (hp)
	strcpy(host, hp->h_name);
    else
	strcpy(host, inet_ntoa(inaddr)); 
}


int Network::doSend(const char *msg, long len)
{
    long n;
    int err;

    err = pthread_mutex_lock(&smutex);
    if ((err!=0) && ((debugLevel==2)||(debugLevel==4)))	
	printf("Network::getRemoteHost--can't lock send mutex\n");
    n = sendto(sock, msg, len, 0, (struct sockaddr *)&peer, sizeof(peer));
    if ((debugLevel==2)||(debugLevel==4))
	printf("Network::doSend--sending msg: %s\n",msg); 
    err = pthread_mutex_unlock(&smutex);
    if ((err!=0) && ( (debugLevel==2)||(debugLevel==4)))	
	printf("Network::getRemoteHost--can't unlock send mutex\n");
    if (n < 0) 
      perror("Network: sendto()");
    else if (n < len) 
      printf("Network: sendto() didn't send whole message\n");
    return n; 	
}
     

int Network::doRecv(char *buf, long len)
{
    int l, n;
    struct timeval t;      
    fd_set fd;

    t.tv_sec = seconds;
    t.tv_usec = 0;
    FD_ZERO(&fd);
    FD_SET(sock, &fd);
    n = select(sock+1, &fd, NULL, NULL, &t);
    if (n > 0)
    {
	l = sizeof(peer);
	n = recvfrom(sock, buf, len, 0, (struct sockaddr *)&peer, &l);
        if ((debugLevel==2) || (debugLevel==4))
	   printf("Network::doRecv--recvfrom returned %d\n",n);
	if (n < 0) 
	{	
	     if (debugLevel > 0)
	     	perror("Network: recvfrom() failed");	
	}
	else
	{
	     buf[n] = '\0';
	     n++;	
	}
    }
    if ((seconds > 0) && ((debugLevel==2) || (debugLevel==4)))
        printf("Network::doRecv--returning %d\n", n);
    return n;
}			
  
int Network::doBlockedRecv(char *buf, long len, char *sender)
{
    /* Multi-threaded version of devserv calls this routine instead of
       doRecv().  This routine blocks until data arrives on the socket and
       extracts the sender's ip addr returned by recvfrom(). */ 	 
  
    int l, n;

    sender[0] = '\0';	
    l = sizeof(peer);
    n = recvfrom(sock, buf, len, 0, (struct sockaddr *)&peer, &l);
    if ( ((debugLevel==2) || (debugLevel==4)) && (errno != EINTR))
        printf("Network::doBlockedRecv--recvfrom returned %d\n",n);
    if (n < 0)
    {	
        if (errno == EINTR)
	{
	   if ((debugLevel==2) || (debugLevel==4))
		printf("recvfrom() returned EINTR\n");
	   return -2;
	}
	if ((debugLevel==2) || (debugLevel==4))
	   printf("doBlockedRecv: recvfrom() errno: %d\n", errno);
        if (debugLevel > 0)
     	   perror("Network: recvfrom() failed");	
    }
    else
    {
        buf[n] = '\0';
        n++;	
    }
    getRemoteHost(sender);	
    if ((debugLevel==2) || (debugLevel==4))
        printf("Network::doBlockedRecv--returning %d\n", n);
    return n;
}			
