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
 *  $Id: devserv.cc,v 1.9 1999/08/09 23:36:25 mperry Exp $
 */
  
 /* MULTITHREADED VERSION */

 /* Linux patches courtesy of Lars Knudsen, Aitek Srl, Italy  */

#include "devserv.h"
	
// These probably should be List objects--change in later version.
CONFIGPTR configptr = NULL;
PORTLISTPTR portlistptr = NULL;
reqptr front = NULL, rear = NULL;

/* powerOff and debugLevel can be set by user in command line arg.
   If user doesn't want devices to be powered off at termination,
   powerOff is set to 0.  
   debugLevel: 0:  no display at all;  
	1: display initialization/termination status and error messages;
	2: display network stuff 
	3: display device communication
	4: display all of the above
*/
	     
short powerOff=1, debugLevel=1, udpport=5555, ipport=5556;
char ipaddr[40], cif_uumaddr[16], cif_udpport[8], cif_uumport[8];
char timestamp[25], devserver[80], video[30], text[80], video_host[80];
char lastcmd[256], requestor[80];
Mode mode = NORMAL; 

#ifdef USE_CIF
pthread_mutex_t cif_mutex;
#endif

pthread_mutex_t mutex;
pthread_mutex_t thr_mutex;
pthread_cond_t cond;
pthread_cond_t thr_cond; 
// thread count (e.g., needed for waiting for all threads to complete)
short thr_count; 
sigset_t signals;  // needed for sigmask()


// Instantiate unicast (UDP) and multicast (IP) objects. 
UDPNet udp;
IPNet ip; 
IPNet iprecv;

// If Akenti is supported, instantiate an SSLtcp object.
#ifdef USE_AKENTI 
SSLtcp ssltcp;
SecurityInfo secInfo;
#endif

#ifdef USE_CIF
  CIF_Comm_Listener *cif_listener[NUM_CIF_LISTENERS];
  CIF_Comm_Connection *cif_conn[NUM_CIF_CONNECTIONS];
#endif


int main(int argc, char *argv[])
{
  int n=1, err;  
  short vidhost=0, ipsock=1, udpsock=1, ssl=1;
  char *file=NULL; 
  struct timespec timeout;
  reqptr msg = NULL;
  pthread_t tid;

#ifdef USE_CIF
  char url[80];
  printf("USING CIF\n");
#endif 

  // Required by sigmask().
  sigemptyset(&signals);
  sigfillset(&signals);

  signal(SIGINT, sighandler);
  signal(SIGTERM, sighandler);
  signal(SIGABRT, sighandler);
  signal(SIGSEGV, sighandler);
#ifdef SIGBUS
  signal(SIGBUS, sighandler);
#endif
#ifdef SIGHUP
  signal(SIGHUP, sighandler);
#endif
#ifdef SIGQUIT
  signal(SIGQUIT, sighandler);
#endif
#ifdef SIGPIPE
  signal(SIGPIPE, sighandler);
#endif

  pthread_mutex_init(&mutex, NULL);
  pthread_mutex_init(&thr_mutex, NULL);
  pthread_cond_init(&cond, NULL);
  pthread_cond_init(&thr_cond, NULL);

#ifdef USE_CIF
  pthread_mutex_init(&cif_mutex, NULL);
#endif

  strcpy(video, "0.0.0.0/0/0");
  strcpy(text, "\"NONE\""); 
  udp.getLocalname(video_host);
  strcpy(lastcmd, "NONE");

  /* Parse cmd line args; if user just wants the version or help,
     display the correct message and exit; otherwise continue parsing.  */

  if (argc > 1)
  {
     if (!strcmp(argv[1], "--version"))
     {
        fprintf(stderr,"Devserv version %s\n", VERSION);
        exit(0);	 	 
     }
     else if (!strcmp(argv[1], "--help"))
     {
	usage();
	exit(0);
     }
  }
  while (n < (argc-1))
  {
      if (!strcmp(argv[n], "-d"))
	  debugLevel = atoi(argv[++n]);
      else if (!strcmp(argv[n], "--version"))
      {
	 fprintf(stderr, "devserv version %s\n", VERSION);   	    	  	
	 exit(0);
      }
      else if (!strcmp(argv[n], "-s"))
      {
	n++;
	if ((argv[n][0]=='n') || (argv[n][0]=='N'))
	    powerOff = 0;
      }
      else if (!strcmp(argv[n], "-t"))
      {
	n++;
	ip.setTTL(atoi(argv[n]));
	iprecv.setTTL(atoi(argv[n]));
      }
      else if (!strcmp(argv[n],"-v")||!strcmp(argv[n],"-video")) 
	strcpy(video, argv[++n]);
      else if (!strcmp(argv[n], "-h"))
      {
	strcpy(video_host, argv[++n]);
	vidhost = 1;
      }
      else if (!strcmp(argv[n], "-m"))
      {	
	n++;
	if (argv[n][0] == 'S')
           mode = SIMULATED;		
      }
      else if (!strcmp(argv[n], "-f"))
      {
	file = new char[80];
	assert(file != NULL);
	strcpy(file, argv[++n]);	
      }
      else
      {	
	  usage();
	  exit(0);
      }
      n++;
  }
      	
  udp.setDebugLevel(debugLevel);
  ip.setDebugLevel(debugLevel);
  iprecv.setDebugLevel(debugLevel);

#ifdef USE_AKENTI 
  ssltcp.setDebugLevel(debugLevel); 
#endif

  strcpy(ipaddr, "224.35.36.37");
  strcpy(cif_udpport, "7775");
  strcpy(cif_uumaddr, "224.35.36.37");
  strcpy(cif_uumport, "7776");

  readConfigFile(vidhost, file);
  if (file)
	delete [] file; 
  doHardwareSetup();

  if (debugLevel > 0) 
      fprintf(stderr, "INITIALIZING DEVICES...THIS MAY TAKE A FEW SECONDS\n");
  // Tell devices to turn on power.
  if (debugLevel > 0)
      fprintf(stderr, "TURNING ON POWER\n");
   
  char temp[12];
  strcpy(temp, "power A 1"); 
  tellAllDevices(temp, 1);
  strcpy(temp, "home");
  tellAllDevices(temp, 1);

  if (debugLevel > 0)
      fprintf(stderr,"DEVICE INITIALIZATION PROCESS FINISHED\n");

  ip.setAddr(ipaddr);
  ip.setPort(ipport);
  udp.setPort(udpport);
  iprecv.setAddr(ipaddr);
  iprecv.setPort(ipport+2);

  if ((debugLevel==2) || (debugLevel==4)) 
  {
    printf("DEVSERV: network addresses are:\n\t");
    printf("udpport: %d, ipport: %d, cif_udpport: %s, cif_uumport: %s\n\t",
		udpport,ipport,cif_udpport,cif_uumport);
    printf("ipaddr: %s, cif_uumaddr: %s\n",ipaddr, cif_uumaddr);
  }
    
#ifdef USE_AKENTI 
  n = ssltcp.openSocket();
  if (n < 0)
  {
     if (debugLevel > 0)  	
	fprintf(stderr,"DEVSERV: ssltcp.openSocket() failed\n");
     ssl = 0;	
  }
#endif

  if (ssl)
  {
      n = udp.openSocket();
      if (n < 0)
      {
     	if (debugLevel > 0)  	
	   fprintf(stderr,"DEVSERV: udp.openSocket() failed\n");
        udpsock = 0;
      }
  }

  n = ip.openSocket();
  if (n < 0)
  {
     if (debugLevel > 0)  	
	fprintf(stderr,"DEVSERV: Can't open multicast sending socket\n");
     ipsock = 0;	
  }

  n = iprecv.openSocket();
  if (n < 0)
  {
     if (debugLevel > 0)  	
	fprintf(stderr,"DEVSERV: Can't open multicast receiving socket\n");
     ipsock = 0;	
  }  
	
  if (udpsock && ipsock)
  {
     if (debugLevel > 0)  	
        fprintf(stderr, "DEVSERV: OPENED UDP AND IP SOCKETS\n"); 
  }
  else
  {
#ifndef USE_CIF
  exit(1); 
#endif
  }
 
  /* If we support CIF communication, instantiate Listener, Connection, Message,
     and other needed objects.  Open uum (multicast) connections for sending/
     receiving. Then create threads to do the receives. */ 

#ifdef USE_CIF
  CIF_Comm::activate();
  sprintf(url,"x-cif-uum://%s:%s/ttl=%d+loopback=true",
	cif_uumaddr,cif_uumport,ip.getTTL());
  cif_conn[0] = CIF_Comm_Factory::get_Connection(url);

  if ((CIF_Comm::status != CIF_Comm_Status::SUCCESS)||(cif_conn[0]==CIF_NULL))
  {
	if (debugLevel > 0)
	    printf("DEVSERV: Can't get a CIF-uum send connection\n");
  }
  if ((debugLevel==2) || (debugLevel==4))
     printf("devserv: CIF-uum send connection established for URL: %s\n",url);

  sprintf(url,"x-cif-uum://%s:%d/ttl=%d+loopback=true",
	cif_uumaddr,(atoi(cif_uumport)+2),ip.getTTL());
  cif_conn[2] = CIF_Comm_Factory::get_Connection(url);
  if ((CIF_Comm::status != CIF_Comm_Status::SUCCESS)||(cif_conn[2]==CIF_NULL))
  {
     if (debugLevel > 0)
         printf("DEVSERV: Can't get a CIF-uum receive connection\n");	
  }
  if ((debugLevel == 2) || (debugLevel == 4))
     printf("DEVSERV: CIF-uum recv connection established for URL: %s\n",url); 
  
  sprintf(url, "x-cif-tcp://%s", cif_udpport);
  cif_listener[0] = CIF_Comm_Factory::get_Listener(url);
  if ((CIF_Comm::status!=CIF_Comm_Status::SUCCESS)||(cif_listener[0]==CIF_NULL))
  {
     if (debugLevel > 0)
	printf("DEVSERV: Can't get a CIF-tcp connection\n");
  } 
  if ((debugLevel==2) || (debugLevel==4))
     printf("devserv: Listening at %s\n\n", cif_listener[0]->get_URL()); 
#endif 
     
  strcpy(requestor, "NONE");
  
  /* Create threads to do NON-CIF network receives.  */   
  if (udpsock)
  {
      n = pthread_create(&tid, NULL, getMessage, &udp);
      assert(n == 0);	
  }

  if (ipsock)
  {
      err = pthread_create(&tid, NULL, getMessage, &iprecv);
      assert(err == 0);	
  }

#ifdef USE_CIF
  /* Create threads to do CIF receives.  */

  if (cif_listener[0] != CIF_NULL)
  {
      err = pthread_create(&tid, NULL, getCIF_udpMsg, NULL);
      assert(err == 0);	
  }  

  if (cif_conn[2] != CIF_NULL) 
  {
      err = pthread_create(&tid, NULL, getCIF_uumMsg, NULL);
      assert(err == 0);	
  }  
#endif
      
#ifdef USE_AKENTI 
  if (ssl)
  {
      err = pthread_create(&tid, NULL, doSSLAccept, NULL);
      assert(err == 0);	
  }  
#endif

  /* Initialize the timestamp  */
  strcpy(timestamp, "0");
    
  while (1)
  {  
    /* Set timeout for current time + TIMEOUT seconds. Wait until signalled 
       that the list of requests is not empty or until we time out. If we time 
       out, send a multicast reply; otherwise, delete the request from the 
       list and process it, and send a reply. */  
 
   timeout.tv_sec = time(NULL) + TIMEOUT;
   timeout.tv_nsec = 0;
   
   n = pthread_mutex_lock(&mutex);
   if (n != 0)
   {
	if (debugLevel > 0)
	   printf("DEVSERV: can't lock mutex for list\n");
   }
   while (front == NULL)
   {
      n = pthread_cond_timedwait(&cond, &mutex, &timeout);
      if (n == ETIMEDOUT)
      {
	    sendDescription(ipsock);
	    break; 
      }	
      if (n != 0)
      {
	 if (debugLevel > 0)
	    printf("DEVSERV: Condition timed wait failed\n"); 		
	 break;
      }
   }
		      
   while (front)
   {
      msg = listDelete();
      processRequest(msg->cmd, msg->len, msg->sender, ipsock);
      free(msg);
      msg = NULL;	
   }
   n = pthread_mutex_unlock(&mutex);
   if (n != 0)
   {
	if (debugLevel > 0)
	  printf("DEVSERV: can't unlock mutex for list\n");
   }
  }
  return 0;
}


void sighandler(int sig) 
{
   /* Since sigwait() is broken for freeBSD, am using this routine to
      catch and process interrupts.  Solaris 2.6 and freeBSD (2.2.6,
      2.2.7) are both happy.  */
     
   if (debugLevel > 0)
   {
     fprintf(stderr,"DEVSERV: SIGNAL %d\n", sig);	
     fprintf(stderr,"DEVSERV: SHUTTING DOWN ALL ATTACHED DEVICES...THIS MAY TAKE A WHILE\n");
   }
   if (powerOff)
   {
     thr_count = 0;
     /* 2nd arg is to make tellAllDevices to wait for all threads it creates
        to terminate before returning.  */ 

     // linux requires the strcpy or else it seg faults 
     char temp[10];
     strcpy(temp, "shutdown");
#ifndef __linux__
     tellAllDevices(temp, 1);
#else
     tellEachDevice(temp);
#endif
     if (debugLevel > 0)
        fprintf(stderr, "SHUTDOWN COMPLETE\n");
   } 
   _exit(1);

}

void doHardwareSetup()
{
  // Instantiate objects for supported devices.
  if (configptr)
      createDevObject();
  else
  {
      if (debugLevel > 0)
      {
          fprintf(stderr,"devserv can't find a configuration file...");
	  fprintf(stderr,"using defaults\n");
      }
      doDefaults();
  }
}


void readConfigFile(short flag, char *file)
{
   /* Read the configuration file; create a linked list of supported devices,
      createDevObject will instantiate an object for each device type. */

   char buf[80], label[20]; 
   char *home = getenv("HOME");
   FILE *fp = NULL; 

   strcpy(buf, home);
   if (file)
   {
	strcat(buf, "/");
	strcat(buf, file);
	fp = fopen(buf, "r");
   }
   if (!fp) 
   {
	strcat(buf, "/.devserv.config");
	fp = fopen(buf, "r");
   }

   if (fp)
   {
	while (fgets(buf, 80, fp) != NULL)
	{
	   if ((buf[0] == '#') || (buf[0] == '\n'))
		continue;
	   sscanf(buf, "%s", label);
   	   if ((label[0]=='d')&&(label[1]=='e')&&(label[2]=='v'))
	        insertDevice(buf);
	   else if ((label[0]=='a')&&(label[1]=='d')&&(label[2]=='d')) 
	        setAddrs(buf);
	   else if (!strcmp(label,"video")||!strcmp(label,"video:"))
		setVideoAddr(buf);
	   else if ((label[0]=='t')&&(label[1]=='e')&&(label[2]=='x'))
		setText(buf);
	   else if (!flag&&(!strcmp(label,"video_host")||!strcmp(label,"video_host:")))
	 	setDeviceHost(buf);
	}
	fclose(fp);
   }
}


void insertDevice(char *line)
{
   // Insert entry for device into front of device configuration list. 

   char buf[10], make[25], model[30], type[15], port[60];
   int devnum, unitnum;
   char lens[15];

   lens[0] = '\0';
   CONFIGPTR temp = (CONFIGPTR)malloc(sizeof(CONFIG));
   sscanf(line,"%s%s%s%d%s%d%s%s",buf,make,model,&devnum,type,&unitnum,port,lens);
   strcpy(temp->make, make);
   strcpy(temp->model, model);
   strcpy(temp->type, type); 
   strcpy(temp->port, port);
   temp->devnum = devnum;
   temp->unitnum = unitnum;  
   if ((lens[0]=='W') || (lens[0]=='w'))
	temp->lens = 1;
   else
	temp->lens = 0;
   temp->objptr = NULL; 
   temp->next = configptr;
   configptr = temp; 
   if (temp->devnum == 1)
      insertPort(port);	
}


void insertPort(char *port)
{
   // Insert port into ports list and instantiate a Serialport object.
   PORTLISTPTR temp = portlistptr;
   static int counter=0;

   /* If the port is in the list, there is either a duplicate entry
      in the config file or the user is trying to connect different
      devices to the same port.  Return in either case.*/

   while (temp)
   {
	if (!strcmp(temp->port, port))
	    return;
	temp = temp->next;
   }		
   temp = (PORTLISTPTR)malloc(sizeof(PORTS));
   strcpy(temp->port, port);
   temp->portptr = new Serialport(counter, debugLevel);
   temp->next = portlistptr;
   portlistptr = temp;
   ++counter;	
}


void doDefaults()
{ 
   Serialport *portp;
   char port[40];
   
   // This should probably be changed to prompt the user for this info.
#ifdef sun
   strcpy(port, "/dev/ttya");  
#endif
#ifdef sgi 
   strcpy(port, "/dev/ttym1");
#endif
#ifdef __FreeBSD__
   strcpy(port, "/dev/ttyd1");
#endif

   configptr = (CONFIGPTR)malloc(sizeof(CONFIG));
   assert(configptr != NULL);
   strcpy(configptr->port, port);
   strcpy(configptr->make, "Sony");
   strcpy(configptr->model,"EVI-D30");
   strcpy(configptr->type, "cam");
   configptr->devnum = 1;
   configptr->unitnum = 1;
   configptr->lens = 0; 	
   portp = new Serialport(0, debugLevel);
   configptr->objptr = new SonyEVID30(portp, port, 1, 0);
   configptr->next = NULL;
}
   

void setAddrs(char *buf)
{
   // Set the network addresses (CIF and non-CIF).
   char label[20], name[20], value[20] ;

   sscanf(buf, "%s%s%s", label, name, value);
   if (!strcmp(name,"multicastPort"))
	ipport = atoi(value);
   else if (!strcmp(name, "udpPort"))
	udpport = atoi(value);
   else if (!strcmp(name, "cifUDPPort"))
	strcpy(cif_udpport, value);
   else if (!strcmp(name, "cifUUMPort"))
	strcpy(cif_uumport, value);
   else if (!strcmp(name, "multicastAddress"))
	strcpy(ipaddr, value);
   else if (!strcmp(name, "CIFUUMAddress"))
	strcpy(cif_uumaddr, value);
}

void setVideoAddr(char *buf)
{
   /* Set the video transmission address/port/ttl if it wasn't set by cmd line
      arg (in which case we don't have the default value of "0.0.0.0/0/0"). */

   char label[20], value[30];
   if (strcmp(video, "0.0.0.0/0/0") != 0)
	return;
   if (sscanf(buf, "%s%s", label, value) == 2)
   	strcpy(video, value); 
}

void setText(char *buf)
{
   /* Set the text description of devserv: skip the keyword 'text',
      skip all whitespace following the keyword, then copy the value. */ 

   int i, j;
   for (i=0; ((buf[i] != ' ')&&(buf[i] != '\0')); i++);
   for (; buf[i]==' '; i++);
   for (j=0; ((buf[i]!='\0')&&(buf[i]!='\n')); i++,j++)
	text[j] = buf[i];
   text[j] = '\0';
}

void *getMessage(void *arg)
{
   /* This is the start routine for non-CIF network receive threads. */ 

   int n;
   reqptr temp;  // change this to create a thread to process request
   Network *net = (Network *)arg;

   // Don't let this thread catch signals.
   n = pthread_sigmask(SIG_BLOCK, &signals, NULL);
   assert(n == 0);

   // W/O the following, main's timedwait won't work if no request is
   // received by a network thread. 

   n = pthread_mutex_lock(&mutex);
   if (n != 0) 
   {
      if (debugLevel > 0)
    	printf("DEVSERV: Can't lock mutex\n"); 
   } 
   n = pthread_cond_signal(&cond);
   if (n != 0) 
   {
      if (debugLevel > 0)
    	printf("DEVSERV: Can't signal\n"); 
   } 
   n = pthread_mutex_unlock(&mutex);
   if (n != 0) 
   {
      if (debugLevel > 0)
    	printf("Can't unlock mutex\n"); 
   } 

   /* Loop forever, doing a blocked recv to get client requests.  When a
      request is received, add it to a linked list and signal.  */

   while (1)  
   {
 	temp = (reqptr)malloc(sizeof(request_t));
	if (!temp)
	{
	    if (debugLevel > 0)
	      printf("DEVSERV: Can't get memory for request\n");
	    break;
	}
	n=net->doBlockedRecv(temp->cmd,(long)sizeof(temp->cmd),temp->sender);
        if (n == -2)
	    break;
        temp->len = n;
	if (n <= 0)	
	{
	    free(temp);
	    if ((debugLevel > 0) && (n != -2))
	       printf("Can't receive message\n");
	}
	else
	{
	    if (debugLevel == 2 || debugLevel == 4)
	       printf("RECVD REQUEST: length: %d, MSG: %s, SENDER: %s\n",
		   temp->len, temp->cmd, temp->sender); 
	    enqueue(temp);
	}
   }
   return NULL;
}
    
#ifdef USE_CIF
void *getCIF_udpMsg(void *arg)
{
   int n;
   CIF_Comm_Message cif_msg;

   // Don't let this thread catch signals.
   n = pthread_sigmask(SIG_BLOCK, &signals, NULL);
   assert(n == 0);

   while (1)
   {
       n = pthread_mutex_lock(&cif_mutex);
       if (n != 0)
       {
	 if (debugLevel > 0)
	   printf("DEVSERV: Can't lock cif_mutex\n");
	 break;
       }		
       cif_conn[1] = cif_listener[0]->get_Connection(CIF_TIMEOUT); 	

       n = pthread_mutex_unlock(&cif_mutex);
       if (n != 0)
       {
	 if (debugLevel > 0)
	   printf("DEVSERV: Can't unlock cif_mutex\n");
	 break;
       }		
       sleep(1);	
       if (cif_conn[1] != CIF_NULL)
       {
	 if (doCIFRecv(cif_conn[1], cif_msg) < 0)
	    break;
	 cif_conn[1]->close();
       }	  
       delete cif_conn[1]; 
       sleep(3);	
   }
   return NULL;
}


void *getCIF_uumMsg(void *arg)
{
   CIF_Comm_Message msg;

   // Don't let this thread catch signals.
   int n = pthread_sigmask(SIG_BLOCK, &signals, NULL);
   assert(n == 0);

   while (1)
   {
	sleep(3);
	if (doCIFRecv(cif_conn[2], msg) < 0)
	    break;
   }
   return NULL;
}


int doCIFRecv(CIF_Comm_Connection *conn, CIF_Comm_Message msg)
{
   reqptr temp;
   CIF_Bool rc;
   int err;

   err = pthread_mutex_lock(&cif_mutex);
   if (err != 0)
   {
      if (debugLevel > 0)
	 printf("DEVSERV: Can't lock cif_mutex\n");
      return -1;
   }
   rc = conn->receive(msg, CIF_TIMEOUT);
   if (rc == true)
   {
      temp = (reqptr)malloc(sizeof(request_t));
      if (!temp)
      {
	if (debugLevel > 0)
	  printf("DEVSERV: doCIFRecv--malloc() failed\n");
      }
      else
      {
	 strcpy(temp->sender, "CIF_Client");
	 if ((debugLevel==2) || (debugLevel==4))
	    printf("DEVSERV: CIF MSG: %s\n",msg.get_Data());
	 sprintf(temp->cmd, "%s", msg.get_Data()); 
         temp->len = msg.get_Info().size;
      }	
   }
   err = pthread_mutex_unlock(&cif_mutex);
   if (err != 0)
   {
      if (debugLevel > 0)
	 printf("DEVSERV: Can't unlock cif_mutex\n");
      return -1;
   }
   if (rc == true)
	enqueue(temp); 
   return 0;
}
#endif

#ifdef USE_AKENTI 
void *doSSLAccept(void *arg)
{
   int n;
   // Don't let this thread catch signals.
   n = pthread_sigmask(SIG_BLOCK, &signals, NULL);
   assert(n == 0);
   while (1)
   {
	n = ssltcp.doBlockedAccept(&secInfo);
	assert(n>=0);
        ssltcp.closeSockets(); 
   }
   return NULL;
}   
#endif

void setDeviceHost(char *buf)
{
   /* If a command line arg set the host to which the cameras are
      connected (the device host), this function won't get called.  */ 
   char host[80], label[20]; 
   if (sscanf(buf, "%s%s", label, host) == 2)
       strcpy(video_host, host);
}
	
void listInsert(reqptr ptr)
{
    /* Called by "network" threads to add a device request to a list.  
       Add to rear of list; delete from front.  */

    ptr->next = NULL;
    if (rear == NULL)
	front = ptr;
    else
	rear->next = ptr;
    rear = ptr;	
}

reqptr listDelete()
{
    reqptr temp = front;
    front = temp->next;
    if (front == NULL)
	rear = NULL;
    return temp;
}

void enqueue(reqptr ptr)
{
    /* Called by network threads to add a request to the list and signal main.
       Will be deleted if threads do the device moves. */

    int n;
   
    n = pthread_mutex_lock(&mutex);
    if (n != 0)
    {
	if (debugLevel > 0)
	    printf("DEVSERV: Can't lock list mutex\n");
	return;
    }
    listInsert(ptr);
    n = pthread_cond_signal(&cond);
	
    if (n != 0)
    {
	if (debugLevel > 0)
	    printf("DEVSERV: Can't signal\n");
    }
    n = pthread_mutex_unlock(&mutex);
    if (n != 0)
    {
	if (debugLevel > 0)
	  printf("DEVSERV: Can't unlock list mutex\n");
    }
}


void processRequest(char *msg, int len, const char *sender, short ip_flag)
{
   char cmd[128], time[20];
   short i, more=0;

   /* Get line 1, the header; parse header and determine further
      processing.  (Lines are separated by "#".)  */

   /* The first field of line 1 is the length of the actual message. 
      Compare 'len' (num. bytes received) to the value of this field
      and reject the msg if we didn't receive all of it.  */ 

   for (i=0; ((msg[i] != ' ')&&(msg[i]!='\0')); i++);
   msg[i] = '\0';

   if (len < atoi(msg))
   {
        if (debugLevel > 0)
          fprintf(stderr,"DEVSERV: Received an incomplete message...ignoring it\
n");
        return;
   }

   msg = &msg[++i];

   /* If we're supporting Akenti, the message is in the format:
               header#request#...#SecurityInfo
      header = length timestamp devserv_host
      All messages are null-terminated.
      Strip off "length" from the header and strip off SecurityInfo.

      Call checkAccess() with 2 args:
      the original message (w/o the security info and w/o "length") and the
      security info.
      If checkAccess returns FALSE, return (the client is not authorized).
   */

#ifdef USE_AKENTI 
   /* Go to end of msg, then walk back to find the last occurrence of '#'.
      Replace '#' with nul.  This nul separates the message (w/o the 'length')
      and the security info.
   */
   for (i=0; msg[i]!='\0'; i++);
   for (; ((msg[i]!='#') && (i>0)); i--);	
   msg[i] = '\0';
   if (!secInfo.checkAccess(&msg[++i], msg)
        return;
#endif
 
   /* Now that the 'length' has been stripped off, finish parsing line 1. 
      The next field is the timestamp.  Reject the message if this timestamp
      is earlier than that of the last accepted request (stored in the global
      'timestamp' variable).
   */

   for (i=0; ((msg[i]!=' ')&&(msg[i]!='\0')); i++);
   msg[i] = '\0';
   
   /* Save this timestamp for updating the global variable if this request
      is processed.
   */ 
   strcpy(time, msg);

   if ((double)atof(msg) < (double)atof(timestamp))
   {
	if (debugLevel > 0)
	   fprintf(stderr, "DEVSERV: Rejecting request...old timestamp\n");
	return;
   } 
   msg = &msg[++i];

   /* Get next field of line 1, the name of the devserver that is the
      target of this message.  If this value is a multicast address, we
      received a multicast query, probably for a description.  */

   for (i=0; ((msg[i]!='#')&&(msg[i]!='\0')); i++);

   /* As long as we see '#', there are more lines to process.  */
   if (msg[i] == '#')
   {
	more = 1;	
	msg[i] = '\0';
   }

   strcpy(devserver, msg);
   msg = &msg[++i];
   cmd[0] = '\0';
       
   // Get the remaining lines of the message, and process one line at a time.
   
   while (more)
   {
        /* processLine() returns 0 if no device move was carried out (e.g.,
           client not authorized, description explicitly requested).
           If a device was moved (processLine returned 1), copy the device
           reference into arg 1.  */

        for (i=0; ((msg[i]!='#')&&(msg[i]!='\0')); i++);
        if (msg[i] == '\0')
	    more = 0;
	else
	    msg[i] = '\0'; 

        /* Since line 2 of our own messages specify the message type as
	   'description' or 'status', if we see these values, this is our
           own message received via loopback, so do not process it.
        */ 
  
        if (!strcmp(msg,"description") || !strcmp(msg, "status"))
             return;

        /* If a device was moved, copy the device reference into 'cmd'
           (if it's the first device in the request) or concatenate it
           to 'cmd' if it's not the first reference. */

        if (processLine(msg, ip_flag) > 0)
        {
           if (cmd[0] == '\0')
                strcpy(cmd, msg);
           else
           {
                strcat(cmd, "@");
                strcat(cmd, msg);
           }
        }
   }

   /* If there was >= 1 device reference, update 'lastcmd' & 'timestamp' 
      and send a description message as a reply. */
   if (cmd[0] != '\0')
   {
        strcpy(lastcmd, cmd);
        strcpy(requestor, sender);
	strcpy(timestamp, time);
        sendDescription(ip_flag);
   }
}


int processLine(char *line, short ip_flag)
{
   char field1[16], field2[16];
   int i,j;
   Device *obj;

   /* The first two fields are either a device and number (e.g., "cam 1" or
      "camera 1") or a request to send a description.  If device, return the
      device reference since the description will be sent after the entire
      request has been processed. */  

   for (i=0,j=0; ((line[i]!=' ')&&(line[i]!='\0')); i++,j++)
	field1[j] = line[i];
   field1[j] = '\0';

   for (++i,j=0; ((line[i]!=' ')&&(line[i]!='\0')); i++,j++)
	field2[j] = line[i];
   field2[j] = '\0'; 	

   if (!strcmp(field1, "devserv") && !strcmp(field2, "description"))
   {
	// multicast description or status message 
        sendDescription(ip_flag);
  	return 0; 
   }

   // Return if the first field is not a device type.
   if (!isDevice(field1))
	return 0;

   /* If the request is "devserv home", send all the devices to their
      home positions.  */

   if (!strcmp(field1,"devserv") && !strcmp(field2, "home"))
   {
	tellAllDevices(field2, 1);
	strcpy(line, "devserv");
	return 1;
   }

   /* Get a device object based on the client reference and tell the object   
      to move.  */

   obj = getDevice(field1, atoi(field2));
   if (obj)
       obj->move(&line[++i]); 
   sprintf(line, "%s %s", field1, field2);  
   return 1;
}	


void sendDescription(short ip_flag)
{
   char *desc = new char[1024];

   memset(desc, 0, 1024);
   formatDescription(desc);
   if (ip_flag)
      ip.doSend(desc, (long)strlen(desc));
#ifdef USE_CIF
   CIF_Bool rc;
   if (cif_conn[0] != CIF_NULL)
   {
      CIF_Byte buf[1024];
      sprintf((char *)buf, "%s", desc);
      rc = cif_conn[0]->send(buf,strlen((char *)buf)+1);
      if ((rc == false) || (CIF_Comm::status!=CIF_Comm_Status::SUCCESS))	
      {
	if (debugLevel > 0)
	    printf("DEVSERV: Can't send CIF uum description\n");	
      }
   }
#endif 
   delete [] desc;
}


void formatDescription(char *desc)
{
   char *temp;
   long i,j;
   CONFIGPTR listp = configptr;
   struct timeval t;
   struct timezone tz;

   temp = new char[1024];
   if (!temp)
   {
	if (debugLevel > 0)
	   fprintf(stderr,"CAN'T FORMAT DESCRIPTION--OUT OF MEMORY\n");
	return;
   } 

   // Line 1 of message (per Camera Remote Control Command Language).
   gettimeofday(&t,&tz);
   sprintf(temp,"%ld%ld ",t.tv_sec, (t.tv_usec/1000)); 
   udp.getLocalname(&temp[strlen(temp)]);

   // Line 2 - message type
   strcat(temp, "#description#");

   // Line 3 - devserv description, version, and features 
   strcat(temp, text);
   strcat(temp, " ");
   strcat(temp, VERSION);
   strcat(temp, " ");
   getFeatures(&temp[strlen(temp)]); 

   // Line 4 - video address/port/ttl and host with devices attached
   sprintf(&temp[strlen(temp)], "#%s %s#", video, video_host);

   // Line 5 - identification of last cmd accepted including requestor
   //          and timestamp

   sprintf(&temp[strlen(temp)],"\"%s\" %s %s",lastcmd,requestor,timestamp);
  
   // Status lines returned by each device
   while (listp)
   {
       sprintf(&temp[strlen(temp)],"#%s %d ",listp->type,listp->unitnum);
       if (listp->objptr)
	   listp->objptr->getDesc(&temp[strlen(temp)]);
       else
	   strcat(temp, "-1");
       listp = listp->next;
   } 
   i = strlen(temp);
   sprintf(desc, "%ld", i);
   j = strlen(desc);
   i += j;
   sprintf(desc, "%ld ", i);
   strcat(desc, temp); 
   delete [] temp;
}

int getFeatures(char *buf)
{
   int i=0;
#ifdef USE_CIF
   strcpy(buf, "CIF");
   i++;
#endif
#ifdef USE_AKENTI 
   if (i>0)
   {
	strcat(buf, "@");
	strcat(buf, "Akenti");
   }
   else
	strcpy(buf, "Akenti");
   i++;
#endif 
   return i;
}


void createDevObject()
{
   CONFIGPTR temp = configptr;
   PORTLISTPTR portp;

   // Instantiate one object for each hardware device.
   while (temp)
   {
       portp = portlistptr;
       // Match the port names on the hardware-config & port lists.

       while (portp && (strcmp(temp->port,portp->port)!=0))
	   portp = portp->next;
       if (portp && portp->portptr)
       {
	   if (!strcmp(temp->make,"Sony")&&(!strcmp(temp->model,"EVI-D30")||!strcmp(temp->model,"EVI D30")))
	     temp->objptr = new SonyEVID30(portp->portptr,portp->port,temp->devnum,debugLevel,temp->lens,mode);
	   else if (!strcmp(temp->make,"Canon")&&(!strcmp(temp->model,"VC-C3")||!strcmp(temp->model,"VC C3")))
	     temp->objptr = new CanonVCC3(portp->portptr,portp->port,temp->devnum,debugLevel,temp->lens,mode);
	   else if (!strcmp(temp->make,"Canon")&&(!strcmp(temp->model,"VC-C1")||!strcmp(temp->model,"VC C1")))
	     temp->objptr = new CanonVCC1(portp->portptr,portp->port,temp->devnum,debugLevel,temp->lens,mode);
	   else if (!strcmp(temp->make,"Panasonic")&&(!strcmp(temp->model,"WJ-MX50")||!strcmp(temp->model,"WJ MX50")))
	     temp->objptr = new PanasonicWJMX50(portp->portptr,portp->port,temp->devnum,debugLevel,mode);
	   temp = temp->next;
       }
   }
   // Remove the ports list.
   portp = portlistptr;
   while (portp)
   {
	portlistptr = portlistptr->next;
	free(portp);
	portp = portlistptr;
   }
}

Device *getDevice(char *s, int unit)
{
   /* Map client handle to device object by walking linked list and comparing
      'type' and 'unitnum' fields of structs.  Return a pointer to the device
      device object associated with that type and unitnum. If handle is invalid,
      return NULL.  */

   CONFIGPTR temp = configptr;
   while (temp)
   {
      if ((temp->unitnum==unit)&&(temp->type[0]==s[0])&&(temp->type[1]==s[1])&&(temp->type[2]==s[2]))
         return temp->objptr;
      temp = temp->next;
   }
   return NULL;
}

void tellAllDevices(char *cmd, int wait_flag)
{
	int err;
	pthread_t id;
	CONFIGPTR temp = configptr;
	DEVCMD cmdp = NULL;

	while (temp)
	{
	    if (temp->objptr)
  	    {
                cmdp = (DEVCMD)malloc(sizeof(device_cmd_t));
	        if (!cmdp)
	        {
	            if (debugLevel > 0)
		       printf("DEVSERV: malloc() from tellAllDevices failed\n");
	            return;
	        }
		cmdp->objptr = temp->objptr;
		cmdp->cmd = cmd;
		err = pthread_create(&id, NULL, tellDevice, cmdp);	
		if (err != 0)
	        {
		    if (debugLevel > 0)
			printf("DEVSERV: can't create thread to move device\n");
	        }  
		else
                {
		    err = pthread_mutex_lock(&thr_mutex);
		    thr_count++;
		    err = pthread_mutex_unlock(&thr_mutex);
	        }
	    }
	    temp = temp->next;
	}
        if (wait_flag)
        {
	   err = pthread_mutex_lock(&thr_mutex);
           while (thr_count > 0)
	       err = pthread_cond_wait(&thr_cond, &thr_mutex);
	   err = pthread_mutex_unlock(&thr_mutex);
	}  
}

void tellEachDevice(char *cmd)
{
        CONFIGPTR temp = configptr;
        while (temp)
        {
            if (temp->objptr)
	    {
		if (!strcmp(cmd, "shutdown"))
		   temp->objptr->shutDown();
		else
    	           temp->objptr->move(cmd);
	    }
            temp = temp->next;
        }
}


void *tellDevice(void *arg)
{
    int err;
    DEVCMD devp = (DEVCMD)arg;
    
    pthread_detach(pthread_self());
    if (!strcmp(devp->cmd, "shutdown"))
	devp->objptr->shutDown();
    else
    	devp->objptr->move(devp->cmd);
    free(devp);	
    err = pthread_mutex_lock(&thr_mutex);
    if (err != 0)
    {
	if (debugLevel > 0)
	   printf("DEVSERV: tellDevice--can't lock thr_mutex\n");
    }
    thr_count--; 
    if (thr_count == 0)
    {
    	err = pthread_cond_signal(&thr_cond); 
    	if (err != 0)
    	{
	     if (debugLevel > 0)
	   	printf("DEVSERV: tellDevice--cond signal failed\n");
    	}
    }
    err = pthread_mutex_unlock(&thr_mutex);
    if (err != 0)
    {
	if (debugLevel > 0)
	   printf("DEVSERV: tellDevice--can't unlock thr_mutex\n");
    }
    return NULL;
}

int isDevice(const char *s)
{
   if (!strcmp(s,"vid")||!strcmp(s,"videoswitch")||!strcmp(s,"cam")||!strcmp(s,"camera"))
	return 1;
   return 0;
}


void usage()
{
 fprintf(stderr,"Usage: devserv [-d <debug level>] [-s N] [-m S] [-t <ttl>] ");
 fprintf(stderr,"[-h <video_host>] [-v <video_address/video_port/video_ttl]\n");
 fprintf(stderr,"For help, type \"devserv --help\"\n"); 
 fprintf(stderr,"To get the version, type \"devserv --version\"\n"); 
}

