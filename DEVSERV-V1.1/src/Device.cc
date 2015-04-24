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
 * $Id: Device.cc,v 1.3 1999/07/21 17:49:10 mcavoy Exp $
*/


#include <assert.h>
#include "Device.h"

short Device::debugLevel = 1;

Device::Device(int num, short d, Mode m)
{
	
    devmode = m;
    devnum = num;
    debugLevel = d;
    power = 1;
    int n = pthread_mutex_init(&devmutex, NULL);
    assert(n == 0); 
}

Device::~Device()
{
}


int Device::clearInput(int fd)
{
    unsigned char c;
    int i=0;

    // Clear any extra input so device is ready for next command.
    while ((i=read(fd, &c, 1)) > 0);
    return 0;
}

int Device::startup()
{
    printf("DEVICE::startup\n");
    power = 1;
    doPower();
    return goHome();
}


void Device::setAvail(int n)
{
    if (n <= 0)
	avail = 0;
    else
	avail = 1;
}		    

int Device::lockMutex(pthread_mutex_t *mutex)
{
    int x = pthread_mutex_lock(mutex);
    if (x != 0)
    {
        if (debugLevel > 0)
           printf("Device can't lock mutex\n");
    } 
    return x;
}


int Device::unlockMutex(pthread_mutex_t *mutex)
{
    int x = pthread_mutex_unlock(mutex);
    if (x != 0)
    {
        if (debugLevel > 0)
           printf("Device can't unlock mutex\n");
    } 
    return x;
}
