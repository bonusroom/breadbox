#
#  Copyright (c) 1999 Regents of the University of California.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. All advertising materials mentioning features or use of this software
#     must display the following acknowledgment:
#        This product includes software developed by the Imaging and
#        Distributed Collaboratories Group at Lawrence Berkeley
#        National Laboratory.
#  4. Neither the name of the University nor of the Laboratory may be used
#     to endorse or promote products derived from this software without
#     specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  SUCH DAMAGE.
#
#  $Id: Makefile.am,v 1.6 1999/08/10 20:33:47 mperry Exp $
#


# freebsd has problems with deps in makefile
AUTOMAKE_OPTIONS = no-dependencies

bin_PROGRAMS = devserv

devserv_SOURCES = \
	Cam.cc \
	Cam.h \
	Device.cc \
	Device.h \
	Switcher.cc \
	Switcher.h \
	canonvcc1.cc \
	canonvcc1.h \
	canonvcc3.cc \
	canonvcc3.h \
	config.h.in \
	devserv.cc \
	devserv.h \
	ipnet.cc \
	ipnet.h \
	network.cc \
	network.h \
	panasonicwjmx50.cc \
	panasonicwjmx50.h \
	serialport.cc \
	serialport.h \
	sonyevid30.cc \
	sonyevid30.h \
	udpnet.cc \
	udpnet.h \
	utils.cc \
	utils.h

CPPFLAGS = @CPPFLAGS@ @DFLAGS@

INCLUDES = \
	@HEADER_SEARCH_PATH@

devserv_LDADD = \
	@LIBRARY_SEARCH_PATH@ \
	@XLIBS@ 


