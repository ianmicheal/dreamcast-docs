# Microsoft Visual C++ generated build script - Do not modify

PROJ = EZPRINT
DEBUG = 1
PROGTYPE = 1
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = -d_DEBUG
R_RCDEFINES = -dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = C:\SRC\PRINTDLL\
USEMFC = 0
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = 
CUSEPCHFLAG = 
CPPUSEPCHFLAG = 
FIRSTC =             
FIRSTCPP =             
RC = rc
CFLAGS_D_WDLL = /nologo /W3 /FR /G2 /Zi /D_DEBUG /Od /GD /ALw /Fd"EZPRINT.PDB"
CFLAGS_R_WDLL = /nologo /W3 /FR /O1 /DNDEBUG /GD /ALw
LFLAGS_D_WDLL = /NOLOGO /ONERROR:NOEXE /NOD /PACKC:61440 /CO /NOE /ALIGN:16 /MAP:FULL
LFLAGS_R_WDLL = /NOLOGO /ONERROR:NOEXE /NOD /PACKC:61440 /NOE /ALIGN:16 /MAP:FULL
LIBS_D_WDLL = oldnames libw commdlg shell olecli olesvr ldllcew
LIBS_R_WDLL = oldnames libw commdlg shell olecli olesvr ldllcew
RCFLAGS = /nologo
RESFLAGS = /nologo
RUNFLAGS = 
DEFFILE = EZPRINT.DEF
OBJS_EXT = 
LIBS_EXT = 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_WDLL)
LFLAGS = $(LFLAGS_D_WDLL)
LIBS = $(LIBS_D_WDLL)
MAPFILE = nul
RCDEFINES = $(D_RCDEFINES)
!else
CFLAGS = $(CFLAGS_R_WDLL)
LFLAGS = $(LFLAGS_R_WDLL)
LIBS = $(LIBS_R_WDLL)
MAPFILE = nul
RCDEFINES = $(R_RCDEFINES)
!endif
!if [if exist MSVC.BND del MSVC.BND]
!endif
SBRS = EZPRINT.SBR \
		PRINT.SBR


EZPRINT_DEP = c:\src\printdll\ezprint.h


PRINT_DEP = c:\src\printdll\ezprint.h


EZPRINT_RCDEP = c:\src\printdll\rptgen01.h


all:	$(PROJ).DLL $(PROJ).BSC

EZPRINT.OBJ:	EZPRINT.C $(EZPRINT_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c EZPRINT.C

PRINT.OBJ:	PRINT.C $(PRINT_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c PRINT.C

EZPRINT.RES:	EZPRINT.RC $(EZPRINT_RCDEP)
	$(RC) $(RCFLAGS) $(RCDEFINES) -r EZPRINT.RC


$(PROJ).DLL::	EZPRINT.RES

$(PROJ).DLL::	EZPRINT.OBJ PRINT.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
EZPRINT.OBJ +
PRINT.OBJ +
$(OBJS_EXT)
$(PROJ).DLL
$(MAPFILE)
c:\msvc\lib\+
c:\msvc\mfc\lib\+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) EZPRINT.RES $@
	@copy $(PROJ).CRF MSVC.BND
	implib /nowep $(PROJ).LIB $(PROJ).DLL

$(PROJ).DLL::	EZPRINT.RES
	if not exist MSVC.BND 	$(RC) $(RESFLAGS) EZPRINT.RES $@

run: $(PROJ).DLL
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<