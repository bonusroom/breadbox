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
 * $Id: devserv.h,v 1.8 1999/08/10 19:54:48 mperry Exp $
*/


#ifndef Devserv_h
#define Devserv_h

#include "config.h"
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <assert.h>
#include "udpnet.h"
#include "ipnet.h"
#ifdef USE_AKENTI
#include "ssltcp.h"
#include "SecurityInfo.h"
#endif

#include "serialport.h"
#include "sonyevid30.h"
#include "panasonicwjmx50.h"
#include "canonvcc3.h"
#include "canonvcc1.h"

#ifdef USE_CIF
#include "CIF_Comm.H"
#endif

#define MAX_BUFSIZE             2000
#define TIMEOUT                 450
#define NUM_THREADS            	5 
#define CIF_TIMEOUT             0
#define NUM_CIF_LISTENERS       1
#define NUM_CIF_CONNECTIONS     3

typedef struct config *CONFIGPTR;
typedef struct config {
        char port[60];
        char make[30];
        char model[30];
        int devnum;
        char type[10];
        int unitnum;
        short lens;
        Device *objptr;
        CONFIGPTR next;
} CONFIG;

typedef struct ports *PORTLISTPTR;
typedef struct ports {
        char port[80];
        Serialport *portptr;
        PORTLISTPTR next;
} PORTS;

// This will be deleted when threads are created to process requests.
typedef struct request *reqptr;
typedef struct request {
        char cmd[MAX_BUFSIZE];
        char sender[256];
        int len;
        reqptr next;
} request_t;

// Used for thread routines that move the devices. 
typedef struct device_cmd *DEVCMD;
typedef struct device_cmd {
	Device *objptr;
	char *cmd;
} device_cmd_t; 


void sighandler(int sig);
void doHardwareSetup();
void readConfigFile(short flag, char *file);
void insertDevice(char *line);
void insertPort(char *port);
void doDefaults();
void setAddrs(char *buf);
void setVideoAddr(char *buf);
void setText(char *buf);
void setDeviceHost(char *buf);
void *getMessage(void *arg);
#ifdef USE_CIF
void *getCIF_udpMsg(void *arg);
void *getCIF_uumMsg(void *arg);
int doCIFRecv(CIF_Comm_Connection *conn, CIF_Comm_Message msg);
#endif

#ifdef USE_AKENTI 
void *doSSLAccept(void *arg);
#endif
void listInsert(reqptr ptr);
reqptr listDelete();
void enqueue(reqptr ptr);
void processRequest(char *msg, int len, const char *sender, short ip_flag);
int processLine(char *line, short ip_flag);
void sendDescription(short ip_flag);
void formatDescription(char *desc);
int getFeatures(char *buf);
void createDevObject();
Device *getDevice(char *s, int unit);
int isDevice(const char *s);
void tellAllDevices(char *cmd, int wait_flag);
void tellEachDevice(char *cmd);
void *tellDevice(void *arg);
void usage();

#endif
