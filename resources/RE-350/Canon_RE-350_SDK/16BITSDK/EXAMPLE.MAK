# Microsoft Visual C++ generated build script - Do not modify

PROJ = EXAMPLE
DEBUG = 0
PROGTYPE = 0
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = /d_DEBUG 
R_RCDEFINES = /dNDEBUG 
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = D:\SOURCE\CANON\RE-350\EXAMPL16\
USEMFC = 0
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = 
CUSEPCHFLAG = 
CPPUSEPCHFLAG = 
FIRSTC = EXAMPLE.C   
FIRSTCPP =             
RC = rc
CFLAGS_D_WEXE = /nologo /G2 /W3 /Zi /AM /Od /D "_DEBUG" /FR /GA /Fd"EXAMPLE.PDB"
CFLAGS_R_WEXE = /nologo /Gs /G2 /Zp1 /W3 /AM /D "NDEBUG" /FR /Gw 
LFLAGS_D_WEXE = /NOLOGO /NOD /PACKC:61440 /STACK:10240 /ALIGN:16 /ONERROR:NOEXE /CO  
LFLAGS_R_WEXE = /NOLOGO /NOD /NOE /ALIGN:16 /ONERROR:NOEXE /MAP 
LIBS_D_WEXE = oldnames libw mlibcew commdlg.lib olecli.lib olesvr.lib shell.lib 
LIBS_R_WEXE = libw mlibcew oldnames 
RCFLAGS = /nologo 
RESFLAGS = /nologo 
RUNFLAGS = 
DEFFILE = EXAMPLE.DEF
OBJS_EXT = 
LIBS_EXT = RE350DLL.LIB 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_WEXE)
LFLAGS = $(LFLAGS_D_WEXE)
LIBS = $(LIBS_D_WEXE)
MAPFILE = nul
RCDEFINES = $(D_RCDEFINES)
!else
CFLAGS = $(CFLAGS_R_WEXE)
LFLAGS = $(LFLAGS_R_WEXE)
LIBS = $(LIBS_R_WEXE)
MAPFILE = nul
RCDEFINES = $(R_RCDEFINES)
!endif
!if [if exist MSVC.BND del MSVC.BND]
!endif
SBRS = EXAMPLE.SBR


RE350DLL_DEP = 

EXAMPLE_DEP = d:\source\canon\re-350\exampl16\re350.h \
	d:\source\canon\re-350\exampl16\example.h


EXAMPLE_RCDEP = d:\source\canon\re-350\exampl16\example.h \
	d:\source\canon\re-350\exampl16\example.ico


all:	$(PROJ).EXE $(PROJ).BSC

EXAMPLE.OBJ:	EXAMPLE.C $(EXAMPLE_DEP)
	$(CC) $(CFLAGS) $(CCREATEPCHFLAG) /c EXAMPLE.C

EXAMPLE.RES:	EXAMPLE.RC $(EXAMPLE_RCDEP)
	$(RC) $(RCFLAGS) $(RCDEFINES) -r EXAMPLE.RC


$(PROJ).EXE::	EXAMPLE.RES

$(PROJ).EXE::	EXAMPLE.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
EXAMPLE.OBJ +
$(OBJS_EXT)
$(PROJ).EXE
$(MAPFILE)
c:\msvc\lib\+
c:\msvc\mfc\lib\+
RE350DLL.LIB+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) EXAMPLE.RES $@
	@copy $(PROJ).CRF MSVC.BND

$(PROJ).EXE::	EXAMPLE.RES
	if not exist MSVC.BND 	$(RC) $(RESFLAGS) EXAMPLE.RES $@

run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
