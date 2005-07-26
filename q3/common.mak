# $Id$
#
# Common makefile
#
# Targets
#     target
#     clean
#     clean.win
#     clean.wce

BINDIR					= d:/util/cygwin/bin
VS6DIR					= d:/dev/msvs
VC6DIR					= d:/dev/msvs/vc98
VS7DIR					= c:/Program Files/Microsoft Visual Studio .NET 2003
VC7DIR					= d:/dev/msvs2003/vc7
VC7						= 0
EVCDIR					= d:/dev/msevc4/evc
EVC4					= 1
PLATFORMSDKDIR			= d:/dev/mssdk
CESDKPPC2003JADIR		= d:/dev/msevc4/wce420/pocket pc 2003
CESDKPPC2003ENDIR		= d:/dev/msevc4/wce420/pocket pc 2003
CESDKSIGIIIDIR			= d:/dev/msevc4/wce410/sigmarioniii sdk
CESDKPPC2002JADIR		= d:/dev/msevt/wce300/pocket pc 2002
CESDKPPC2002ENDIR		= d:/dev/msevt/wce300/pocket pc 2002
CESDKHPC2000JADIR		= d:/dev/msevt/wce300/hpc2000
CESDKHPC2000ENDIR		= d:/dev/msevt/wce300/hpc2000
CESDKPPCJADIR			= d:/dev/msevt/wce300/ms pocket pc
CESDKPPCENDIR			= d:/dev/msevt/wce300/ms pocket pc
CESDKHPCPROJADIR		= d:/dev/msevt/wce211/ms hpc pro
CESDKHPCPROENDIR		= d:/dev/msevt/wce211/ms hpc pro
SVNDIR					= d:/dev/subversion
STLPORTDIR				= ../lib/stlport
KCTRLDIR				= ../lib/kctrl

-include ../env.mak


ifeq ($(PROJECTNAME),)
    $(error PROJECTNAME required.)
endif

ifeq ($(PROJECTTYPE),)
    $(error PROJECTTYPE required.)
endif


SHELL					= /bin/bash

ifdef EMULATION
	ifeq ($(shell if [ -z "$(CEVER)" ]; then echo 1; elif [ $(CEVER) -lt 400 ]; then echo 0; else echo 1; fi),0)
		OLDEMULATION	= 1
	endif
endif

ifeq ($(PLATFORM),win)
	# WINDOWS ###############################################################
	SDKDIR					= $(PLATFORMSDKDIR)
	ifeq ($(VC7),1)
		COMPILERDIR			= $(VC7DIR)
		COMMONBINDIR		= $(VS7DIR)/common7/ide
	else
		COMPILERDIR			= $(VC6DIR)
		COMMONBINDIR		= $(VS6DIR)/common/msdev98/bin
	endif
	COMPILERBINDIR			= $(COMPILERDIR)/bin
	ifeq ($(CPU),x86)
		SDKBINDIR			= $(SDKDIR)/bin
		
		SDKINCLUDEDIR		= $(SDKDIR)/include
		SDKLIBDIR			= $(SDKDIR)/lib
		ifeq ($(VC7),1)
			MFCINCLUDEDIR	= $(COMPILERDIR)/atlmfc/include
			MFCLIBDIR		= $(COMPILERDIR)/atlmfc/lib
			ATLINCLUDEDIR	= $(COMPILERDIR)/atlmfc/include
			ATLLIBDIR		= $(COMPILERDIR)/atlmfc/lib
		else
			MFCINCLUDEDIR	= $(COMPILERDIR)/mfc/include
			MFCLIBDIR		= $(COMPILERDIR)/mfc/lib
			ATLINCLUDEDIR	= $(COMPILERDIR)/atl/include
			ATLLIBDIR		= $(COMPILERDIR)/atl/lib
		endif
		COMPILERINCLUDEDIR	= $(COMPILERDIR)/include
		COMPILERLIBDIR		= $(COMPILERDIR)/lib
	endif
	ifeq ($(CPU),x64)
		SDKBINDIR			= $(SDKDIR)/bin/win64/x86/amd64
		
		SDKINCLUDEDIR		= $(SDKDIR)/include
		SDKLIBDIR			= $(SDKDIR)/lib/amd64
		MFCINCLUDEDIR		= $(SDKDIR)/include/mfc
		MFCLIBDIR			= $(SDKDIR)/lib/amd64/atlmfc
		ATLINCLUDEDIR		= $(SDKDIR)/include/atl
		ATLLIBDIR			= $(SDKDIR)/lib/amd64/atlmfc
		COMPILERINCLUDEDIR	= $(SDKDIR)/include/crt
		COMPILERLIBDIR		= $(SDKDIR)/lib/amd64
	endif
	
	BASEPLATFORM		=
	#########################################################################
else
	# WINCE #################################################################
	ifeq ($(shell if [ "$(PLATFORM)" = ppc2003 -o "$(PLATFORM)" = ppc2003se ]; then echo 0; else echo 1; fi),0)
		# PPC2003SE PPC2003 #################################################
		ifeq ($(BASELANG),ja)
			SDKDIR		= $(CESDKPPC2003JADIR)
		else
			SDKDIR		= $(CESDKPPC2003ENDIR)
		endif
		BASEPLATFORM	= ppc
		#####################################################################
	endif
	ifeq ($(PLATFORM),ppc2002)
		# PPC2002 ###########################################################
		ifeq ($(BASELANG),ja)
			SDKDIR		= $(CESDKPPC2002JADIR)
		else
			SDKDIR		= $(CESDKPPC2002ENDIR)
		endif
		BASEPLATFORM	= ppc
		#####################################################################
	endif
	ifeq ($(PLATFORM),hpc2000)
		# HPC2000 ###########################################################
		ifeq ($(BASELANG),ja)
			SDKDIR		= $(CESDKHPC2000JADIR)
		else
			SDKDIR		= $(CESDKHPC2000ENDIR)
		endif
		BASEPLATFORM	= hpc
		#####################################################################
	endif
	ifeq ($(PLATFORM),ppc)
		# PPC ###############################################################
		ifeq ($(BASELANG),ja)
			SDKDIR		= $(CESDKPPCJADIR)
		else
			SDKDIR		= $(CESDKPPCENDIR)
		endif
		BASEPLATFORM	= ppc
		#####################################################################
	endif
	ifeq ($(PLATFORM),hpcpro)
		# HPCPRO ############################################################
		ifeq ($(BASELANG),ja)
			SDKDIR		= $(CESDKHPCPROJADIR)
		else
			SDKDIR		= $(CESDKHPCPROENDIR)
		endif
		BASEPLATFORM	= hpc
		#####################################################################
	endif
	ifeq ($(PLATFORM),sig3)
		# SIGMARION3 ########################################################
		SDKDIR			= $(CESDKSIGIIIDIR)
		BASEPLATFORM	= hpc
		#####################################################################
	endif
	COMPILERDIR			= $(EVCDIR)
	ifeq ($(EVC4),1)
		COMPILERBINDIR	= $(COMPILERDIR)/wce420/bin
	else
		COMPILERBINDIR	= $(COMPILERDIR)/wce300/bin
	endif
	COMMONBINDIR		= $(COMPILERDIR)/../common/evc/bin
	SDKBINDIR			= $(SDKDIR)/bin
	
	ifeq ($(SDKINCLUDEDIR),)
		ifeq ($(shell if [ -z "$(CEVER)" ]; then echo 1; elif [ $(CEVER) -lt 400 ]; then echo 0; else echo 1; fi),0)
			SDKINCLUDEDIR		= $(SDKDIR)/include
		else
			ifndef EMULATION
				SDKINCLUDEDIR	= $(SDKDIR)/include/$(CPU)
			else
				SDKINCLUDEDIR	= $(SDKDIR)/include/Emulator
			endif
		endif
	endif
	ifeq ($(SDKLIBDIR),)
		ifeq ($(shell if [ -z "$(CEVER)" ]; then echo 1; elif [ $(CEVER) -lt 400 ]; then echo 0; else echo 1; fi),0)
			SDKLIBDIR		= $(SDKDIR)/lib/$(LIBCPU)
		else
			ifndef EMULATION
				SDKLIBDIR	= $(SDKDIR)/lib/$(LIBCPU)
			else
				SDKLIBDIR	= $(SDKDIR)/lib/Emulator
			endif
		endif
	endif
	ifeq ($(MFCINCLUDEDIR),)
		MFCINCLUDEDIR		= $(SDKDIR)/mfc/include
	endif
	ifeq ($(MFCLIBDIR),)
		MFCLIBDIR			= $(SDKDIR)/mfc/lib/$(LIBCPU)
	endif
	ifeq ($(ATLINCLUDEDIR),)
		ATLINCLUDEDIR		= $(SDKDIR)/atl/include
	endif
	ifeq ($(ATLLIBDIR),)
		ATLLIBDIR			= $(SDKDIR)/atl/lib/$(LIBCPU)
	endif
	#########################################################################
endif


ifeq ($(PLATFORM),win)
	# WINDOWS ###############################################################
	CCC					= cl
	#########################################################################
else
	# WINCE #################################################################
	ifeq ($(CPU),sh3)
		ifeq ($(EVC4),1)
			CCC			= clsh
		else
			CCC			= shcl
		endif
	endif
	ifeq ($(CPU),sh4)
		ifeq ($(EVC4),1)
			CCC			= clsh
		else
			CCC			= shcl
		endif
	endif
	ifeq ($(CPU),mips)
		CCC				= clmips
	endif
	ifeq ($(CPU),arm)
		CCC				= clarm
	endif
	ifeq ($(CPU),xscale)
		CCC				= clarm
	endif
	ifeq ($(CPU),armv4)
		CCC				= clarm
	endif
	ifeq ($(CPU),armv4i)
		CCC				= clarm
	endif
	ifeq ($(CPU),x86em)
		CCC				= cl
	endif
	#########################################################################
endif
RC						= rc
AR						= lib
LD						= link
MIDL					= midl
DUMPBIN					= dumpbin
GCC						= gcc

ifeq ($(PLATFORM),sig3)
	RCPP				= grep -v "^FONT"
else
	RCPP				= cat
endif

ifdef DEBUG
	BASENAME			= debug
	DSUFFIX				= d
	CCFLAGS				= -Od
ifeq ($(PLATFORM),win)
	ifeq ($(VC7),1)
		ifneq ($(CPU),x64)
			CCFLAGS		+= -RTC1
		endif
	else
		CCFLAGS			+= -GZ
	endif
endif
	DEFINES				= -D_DEBUG
	RCFLAGS				= -d _DEBUG
	SYMBOL				= 1
else
	BASENAME			= release
	DSUFFIX				=
	CCFLAGS				= -O1 -Oi
	DEFINES				= -DNDEBUG
	RCFLAGS				= -d NDEBUG
endif

ifdef SYMBOL
	CCFLAGS				+= -Zi -Fd$(OBJDIR)/
	LDFLAGS				= -DEBUG -DEBUGTYPE:CV -PDB:$(TARGETDIR)/$(PROJECTNAME).pdb -FIXED:NO
else
	LDFLAGS				= -RELEASE -OPT:REF
endif

ifndef DEBUG
	ifeq ($(PLATFORM),win)
		ifeq ($(VC7),1)
			CCFLAGS		+= -GL
			LDFLAGS		+= -LTCG
		endif
	endif
endif

ifneq ($(PLATFORM),win)
	CODE				= unicode
endif

ifeq ($(CODE),unicode)
	USUFFIX				= u
else
	USUFFIX				=
endif

SUFFIX					= $(USUFFIX)$(DSUFFIX)

ifeq ($(PLATFORM),win)
	# WINDOWS ###############################################################
	SUBSYSTEM			= WINDOWS
	ifeq ($(CPU),x86)
		SUBSYSVER		= 4.0
	endif
	ifeq ($(CPU),x64)
		SUBSYSVER		= 5.10
	endif
	#########################################################################
else
	# WINCE #################################################################
	ifndef OLDEMULATION
		SUBSYSTEM		= WINDOWSCE
		SUBSYSVER		= $(shell echo $(CEVER) | sed -e 's/\(.\)\(..\)/\1.\2/')
	else
		SUBSYSTEM		= WINDOWS
		SUBSYSVER		= 4.0
	endif
	#########################################################################
endif

CCFLAGS					+= -nologo -W3 -WX -GF -Gy -Zp8 -X
DEFINES					+= -DWIN32 -D_WIN32 -D_MT -DSTRICT
LDFLAGS					+= -NOLOGO -INCREMENTAL:NO -SUBSYSTEM:$(SUBSYSTEM),$(SUBSYSVER)
RCFLAGS					+= -l 0x411
MIDLFLAGS				= -Oicf
ifeq ($(PLATFORM),win)
	# WINDOWS ###############################################################
	CCFLAGS				+= -EHsc -MD$(DSUFFIX)
	DEFINES				+= -DMT -D_DLL -DWINVER=0x400 -D_WIN32_WINNT=0x400 -D_WIN32_IE=0x600 -DTAPI_CURRENT_VERSION=0x00010004
	ifeq ($(CODE),unicode)
		DEFINES			+= -DUNICODE -D_UNICODE
	endif
	ifeq ($(CPU),x86)
		CCFLAGS			+= -GB
		DEFINES			+= -Dx86 -D_X86_
		LDFLAGS			+= -MACHINE:I386
	endif
	ifeq ($(CPU),x64)
		DEFINES			+= -D_AMD64_
		LDFLAGS			+= -MACHINE:AMD64
	endif
	
	LIBCPU				= $(CPU)
	EXLIBCPU			= $(CPU)
	
	LIBS				= msvcrt$(DSUFFIX).lib \
						  user32.lib \
						  kernel32.lib \
						  wsock32.lib \
						  gdi32.lib \
						  shell32.lib \
						  shlwapi.lib \
						  winmm.lib \
						  imm32.lib \
						  advapi32.lib \
						  comctl32.lib \
						  comdlg32.lib \
						  tapi32.lib \
						  ole32.lib \
						  oleaut32.lib \
						  uuid.lib \
						  urlmon.lib \
						  crypt32.lib \
						  wininet.lib
	ifneq ($(VC7),1)
		LIBS			+= msvcirt$(DSUFFIX).lib
	endif
	ifeq ($(CPU),x64)
		LIBS			+= bufferoverflowu.lib
	endif
	ifdef KCTRL
		LIBS			+= $(KCTRLDIR)/lib/win/$(CPU)/kctrl.lib
	endif
	#########################################################################
else
	# WINCE #################################################################
	ifeq ($(shell if [ -z "$(CEVER)" ]; then echo 1; elif [ $(CEVER) -ge 400 ]; then echo 0; else echo 1; fi),0)
		# WINCE >= 400 ######################################################
#		CCFLAGS			+= -GX
		#####################################################################
	endif
	DEFINES				+= -D_WIN32_WCE=$(CEVER) -DUNDER_CE=$(CEVER) -DUNICODE -D_UNICODE
	LDFLAGS				+= -NODEFAULTLIB
	RCFLAGS				+=  -D _WIN32_WCE=$(CEVER) -D UNDER_CE=$(CEVER)
	
	LIBCPU				= $(CPU)
	EXLIBCPU			= $(CPU)
	ifeq ($(CPU),sh3)
		DEFINES			+= -DSHx -DSH3 -D_SH3_
		LDFLAGS			+= -MACHINE:SH3
	endif
	ifeq ($(CPU),sh4)
		CCFLAGS			+= -Qsh4
		DEFINES			+= -DSHx -DSH4 -D_SH4_
		LDFLAGS			+= -MACHINE:SH4
	endif
	ifeq ($(CPU),mips)
		CCFLAGS			+= -QMRWCE
		DEFINES			+= -DMIPS -D_MIPS_
		LDFLAGS			+= -MACHINE:MIPS
	endif
	ifeq ($(CPU),arm)
		DEFINES			+= -DARM -D_ARM_
		LDFLAGS			+= -MACHINE:ARM
	endif
	ifeq ($(CPU),xscale)
		CCFLAGS			+= -QRxscale
		DEFINES			+= -DARM -D_ARM_
		LDFLAGS			+= -MACHINE:ARM
		LIBCPU			= arm
		EXLIBCPU		= arm
	endif
	ifeq ($(CPU),armv4)
		DEFINES			+= -DARM -D_ARM_ -DARMV4
		LDFLAGS			+= -MACHINE:ARM
		EXLIBCPU		= arm
	endif
	ifeq ($(CPU),armv4i)
		CCFLAGS			+= -QRarch4T -QRinterwork-return
		DEFINES			+= -DARM -D_ARM_ -DARMV4I
		LDFLAGS			+= -MACHINE:THUMB
		EXLIBCPU		= arm
	endif
	ifeq ($(CPU),x86em)
		ifdef OLDEMULATION
			CCFLAGS		+= -Gz
		endif
		DEFINES			+= -Dx86 -D_X86_ -Di486
		LDFLAGS			+= -MACHINE:IX86
		RCFLAGS			+= -D x86 -D _X86 -D i486
	endif
	
	ifeq ($(BASEPLATFORM),ppc)
		DEFINES			+= -D_WIN32_WCE_PSPC -DWIN32_PLATFORM_PSPC
		RCFLAGS			+= -D _WIN32_WCE_PSPC -DWIN32_PLATFORM_PSPC
	endif
	
	ifndef EMULATION
		CCFLAGS			+= -MC
		DEFINES			+= -D_DLL
	endif
	ifdef OLDEMULATION
		DEFINES			+= -D_WIN32_WCE_EMULATION
		RCFLAGS			+= -D _WIN32_WCE_EMULATION
	endif
	
	ifeq ($(EVC4),1)
		MIDLFLAGS		+= -msc_ver 1000
	endif
	
	LIBS				= coredll.lib \
						  corelibc.lib \
						  winsock.lib \
						  ole32.lib \
						  oleaut32.lib \
						  uuid.lib \
						  commctrl.lib \
						  ceshell.lib \
						  commdlg.lib \
						  wininet.lib \
						  htmlview.lib
	ifeq ($(BASEPLATFORM),ppc)
		LIBS			+= aygshell.lib
	endif
	ifeq ($(shell if [ "$(PLATFORM)" = ppc2003 -o "$(PLATFORM)" = ppc2003se ]; then echo 0; else echo 1; fi),0)
		LIBS			+= ccrtrtti.lib
	endif
	ifeq ($(shell if [ "$(PLATFORM)" = ppc2003 -o "$(PLATFORM)" = ppc2003se -o "$(PLATFORM)" = ppc2002 -o "$(PLATFORM)" = sig3 ]; then echo 0; else echo 1; fi),0)
		LIBS			+= urlmon.lib
	endif
	ifeq ($(shell if [ "$(PLATFORM)" = ppc2003 -o "$(PLATFORM)" = ppc2003se -o "$(PLATFORM)" = ppc2002 ]; then echo 0; else echo 1; fi),0)
		LIBS			+= wvuuid.lib
	endif
	ifeq ($(shell if [ "$(PLATFORM)" = ppc2002 ]; then echo 0; elif [ -z "$(CEVER)" ]; then echo 1; elif [ $(CEVER) -ge 400 ]; then echo 0; else echo 1; fi),0)
		LIBS			+= crypt32.lib
	endif
	ifdef KCTRL
		LIBS			+= $(KCTRLDIR)/lib/wce/$(LIBCPU)/kctrl.lib
	endif
	#########################################################################
endif

CPROJS					+= -DPLATFORM_$(shell echo $(PLATFORM) | tr [a-z] [A-Z])

ifeq ($(BASELANG),ja)
	CPROJS				+= -DJAPAN
endif


ifeq ($(PROJECTTYPE),exe)
	TARGETDIRBASE		= bin
	EXTENSION			= exe
endif
ifeq ($(PROJECTTYPE),dll)
	TARGETDIRBASE		= lib
	EXTENSION			= dll
endif
ifeq ($(PROJECTTYPE),lib)
	TARGETDIRBASE		= lib
	EXTENSION			= lib
endif

INCLUDEDIR				= include
SRCDIR					= src
OBJDIRBASE				= obj
TLBDIRBASE				= tlb

ifeq ($(PLATFORM),win)
	# WINDOWS ###############################################################
	OBJDIR				= $(OBJDIRBASE)/$(PLATFORM)/$(CPU)/$(CODE)/$(BASENAME)
	TARGETDIR			= $(TARGETDIRBASE)/$(PLATFORM)/$(CPU)/$(CODE)/$(BASENAME)
	#########################################################################
else
	# WINCE #################################################################
	OBJDIR				= $(OBJDIRBASE)/$(PLATFORM)/$(CPU)/$(BASELANG)/$(BASENAME)
	TARGETDIR			= $(TARGETDIRBASE)/$(PLATFORM)/$(CPU)/$(BASELANG)/$(BASENAME)
	#########################################################################
endif
TLBDIR					= $(TLBDIRBASE)/$(PLATFORM)
TARGETBASE				= $(PROJECTNAME)$(SUFFIX)
TARGET					= $(TARGETBASE).$(EXTENSION)


# STLPORT ###################################################################
INCLUDES				= -I"$(STLPORTDIR)"

STLPORTFLAGS			= -D_STLP_NO_IOSTREAMS
ifdef DEBUG
	STLPORTFLAGS		+= -D_STLP_USE_NEWALLOC
endif
ifeq ($(PLATFORM),win)
	STLPORTFLAGS		+= -D_STLP_NEW_PLATFORM_SDK
endif
ifdef STLPORTEXPORT
	STLPORTFLAGS		+= -D_STLP_EXPORT_NODE_ALLOC
else
	STLPORTFLAGS		+= -D_STLP_IMPORT_NODE_ALLOC
endif
DEFINES					+= $(STLPORTFLAGS)
#############################################################################

# KCTRL #####################################################################
ifdef KCTRL
	INCLUDES			+= -I$(KCTRLDIR)/include
endif
#############################################################################

DEFINES					+= $(CPROJS)

REVISION				= $(shell cat revision)
NVERSION				= $(shell cat version | tr '.' ','),$(REVISION)
SVERSION				= $(shell cat version | sed -e 's/\./, /g'), $(REVISION)
RCDEFINES				= -DNVERSION="$(NVERSION)" -DSVERSION="\"$(SVERSION)\"" -DSUFFIX="\"$(SUFFIX)\""
RCDEFINES				+= $(CPROJS)

RCHEADER				= $(dir $(subst $(OBJDIR), $(SRCDIR), $(RESES)))resource$(shell echo $(RESES) | sed -e 's/\(.*\($(BASEPLATFORM)\)\|.*\)\.res/\2/').h

ifneq ($(TLBS),)
	INCLUDES			+= -I$(TLBDIR)
endif

INCLUDES				+= -I"$(SDKINCLUDEDIR)" -I"$(MFCINCLUDEDIR)" -I"$(ATLINCLUDEDIR)"
ifneq ($(COMPILERINCLUDEDIR),)
	INCLUDES			+= -I"$(COMPILERINCLUDEDIR)"
endif

PROJECTINCLUDES			= -I$(INCLUDEDIR)
PROJECTINCLUDES			+= $(foreach L,$(LIBRARIES),-I../$(L)/include)
PROJECTINCLUDES			+= $(foreach I,$(EXTRAINCLUDES),-I../$(I)/include)
INCLUDES				+= $(PROJECTINCLUDES)
ifeq ($(PLATFORM),win)
	LIBDIRBASE			= $(PLATFORM)/$(CPU)/$(CODE)/$(BASENAME)
else
	LIBDIRBASE			= $(PLATFORM)/$(CPU)/$(BASELANG)/$(BASENAME)
endif
DEPENDLIBS				= $(foreach L,$(LIBRARIES),../$(L)/lib/$(LIBDIRBASE)/$(shell F=../$(L)/libname; if [ -f $$F ]; then cat $$F; else echo $(L); fi)$(SUFFIX).lib)
LIBS					+= $(DEPENDLIBS)

INCLUDES				+= $(EXTERNALINCS)
LIBS					+= $(EXTERNALLIBS)

export PATH				= $(shell cygpath -u "$(BINDIR)"):$(shell cygpath -u "$(SDKBINDIR)"):$(shell cygpath -u "$(COMPILERBINDIR)"):$(shell cygpath -u "$(COMMONBINDIR)"):$(shell cygpath -u "$(SVNDIR)/bin")
export INCLUDE			= $(SDKINCLUDEDIR);$(MFCINCLUDEDIR);$(ATLINCLUDEDIR);$(COMPILERINCLUDEDIR)
export LIB				= $(SDKLIBDIR);$(MFCLIBDIR);$(ATLLIBDIR);$(COMPILERLIBDIR)


target: $(TARGETDIR)/$(TARGET)

clean:
	-for d in $(OBJDIRBASE) $(TLBDIRBASE) $(TARGETDIRBASE); do \
		if [ -d $$d ]; then rm -rf $$d; fi \
	done
	-rm -f version revision

clean.win:
	-for d in $(OBJDIRBASE) $(TLBDIRBASE) $(TARGETDIRBASE); do \
		if [ -d $$d/win ]; then rm -rf $$d/win; fi \
	done
	-rm -f version revision

clean.wce:
	-for d in $(OBJDIRBASE) $(TLBDIRBASE) $(TARGETDIRBASE); do \
		for p in ppc2003se ppc2003 ppc2002 hpc2000 ppc hpcpro sig3; do \
			if [ -d $$d/$$p ]; then rm -rf $$d/$$p; fi \
		done \
	done
	-rm -f version revision

$(OBJDIR)/%.obj: $(SRCDIR)/%.cpp
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(CCC) $(CCFLAGS) $(DEFINES) $(INCLUDES) -c -Fo$@ $<

$(OBJDIR)/%.obj: $(SRCDIR)/%.c
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(CCC) $(CCFLAGS) $(DEFINES) $(INCLUDES) -c -Fo$@ $<

$(SRCDIR)/%.rcx: $(SRCDIR)/%.rc
	$(RCPP) $< > $@

$(OBJDIR)/%.res: $(SRCDIR)/%.rcx $(RCHEADER) $(TLBS) version revision
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(RC) $(RCFLAGS) $(RCDEFINES) -fo $@ $<

$(TLBDIR)/%.tlb: $(SRCDIR)/%.idl
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(MIDL) $(MIDLFLAGS) -out $(TLBDIR) -h $(notdir $(patsubst %.idl, %.h, $<)) -tlb $(notdir $(patsubst %.idl, %.tlb, $<)) $<

$(OBJDIR)/%.d: $(SRCDIR)/%.cpp
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(GCC) $(DEFINES) $(PROJECTINCLUDES) -DDEPENDCHECK -MM -MG -MT $(@:.d=.obj) -MT $@ -nostdinc $< 2>/dev/null | gawk -f ../dep.awk > $@

$(OBJDIR)/%.d: $(SRCDIR)/%.c
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(GCC) $(DEFINES) $(PROJECTINCLUDES) -DDEPENDCHECK -MM -MG -MT $(@:.d=.obj) -MT $@ -nostdinc $< 2>/dev/null | gawk -f ../dep.awk > $@

$(TARGETDIR)/$(TARGETBASE).exe: $(TLBS) $(OBJS) $(RESES) $(DEPENDLIBS)
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(LD) $(LDFLAGS) -OUT:$@ $(OBJS) $(RESES) $(LIBS)

$(TARGETDIR)/$(TARGETBASE).dll: $(TLBS) $(OBJS) $(RESES) $(DEPENDLIBS)
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(LD) $(LDFLAGS) -DLL -OUT:$@ $(OBJS) $(RESES) $(LIBS); \
	if [ $$? -eq 0 ]; then \
		$(DUMPBIN) -EXPORTS $(TARGETDIR)/$(TARGET) | tr '\r' ' ' | gawk -v LIBNAME=$(PROJECTNAME)$(SUFFIX) -f ../def.awk > `echo $(DEFFILE)`; \
		if [ -s "$(DEFFILE)" ]; then \
			if [ ! -z "$(EXTRADEFFILE)" -a -f "$(EXTRADEFFILE)" ]; then \
				cat `echo $(EXTRADEFFILE)` >> `echo $(DEFFILE)`; \
			fi; \
			$(LD) $(LDFLAGS) -DLL -DEF:$(DEFFILE) -BASE:$(BASEADDRESS) -OUT:$@ $(OBJS) $(RESES) $(LIBS) 2>&1 | grep -v "LNK4197"; \
		else \
			exit 1; \
		fi; \
	fi

$(TARGETDIR)/$(TARGETBASE).lib: $(OBJS)
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(AR) -OUT:$@ $(OBJS)

ifneq ($(PLATFORM),)
    ifndef NODEPEND
        -include $(OBJS:.obj=.d)
    endif
endif

include ../targets.mak
