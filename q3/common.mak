# $Id$
#
# Common makefile
#
# Targets
#     target
#     clean
#     clean.desktop
#     clean.wce

BINDIR					= d:/util/cygwin/bin
VCDIR					= d:/dev/msvs/vc98
EVCDIR					= d:/dev/msevt/evc
PLATFORMSDKDIR			= d:/dev/mssdk
CESDKPPC2002JADIR		= d:/dev/msevt/wce300/pocket pc 2002
CESDKPPC2002ENDIR		= d:/dev/msevt/wce300/pocket pc 2002
CESDKHPC2000JADIR		= d:/dev/msevt/wce300/hpc2000
CESDKHPC2000ENDIR		= d:/dev/msevt/wce300/hpc2000
CESDKPPCJADIR			= d:/dev/msevt/wce300/ms pocket pc
CESDKPPCENDIR			= d:/dev/msevt/wce300/ms pocket pc
CESDKHPCPROJADIR		= d:/dev/cetools/wce211/ms hpc pro
CESDKHPCPROENDIR		= d:/dev/cetools/wce211/ms hpc pro
STLPORTDIR				= d:/dev/stlport/STLport-4.5.1/stlport
KCTRLDIR				= d:/home/wince/kctrl


ifeq ($(PROJECTNAME),)
    $(error PROJECTNAME required.)
endif

ifeq ($(PROJECTTYPE),)
    $(error PROJECTTYPE required.)
endif


SHELL					= /bin/bash

ifeq ($(PLATFORM),desktop)
	# DESKTOP ###############################################################
	SDKDIR				= $(PLATFORMSDKDIR)
	COMPILERDIR			= $(VCDIR)
	COMPILERBINDIR		= $(COMPILERDIR)/bin
	COMMONBINDIR		= $(COMPILERDIR)/../common/msdev98/bin
	
	SDKINCLUDEDIR		= $(SDKDIR)/include
	SDKLIBDIR			= $(SDKDIR)/lib
	MFCINCLUDEDIR		= $(COMPILERDIR)/mfc/include
	MFCLIBDIR			= $(COMPILERDIR)/mfc/lib
	ATLINCLUDEDIR		= $(COMPILERDIR)/atl/include
	ATLLIBDIR			= $(COMPILERDIR)/atl/lib
	COMPILERINCLUDEDIR	= $(COMPILERDIR)/include
	COMPILERLIBDIR		= $(COMPILERDIR)/lib
	#########################################################################
else
	# WINCE #################################################################
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
	COMPILERDIR			= $(EVCDIR)
	COMPILERBINDIR		= $(COMPILERDIR)/wce300/bin
	COMMONBINDIR		= $(COMPILERDIR)/../common/evc/bin
	
	SDKINCLUDEDIR		= $(SDKDIR)/include
	SDKLIBDIR			= $(SDKDIR)/lib/$(LIBCPU)
	MFCINCLUDEDIR		= $(SDKDIR)/mfc/include
	MFCLIBDIR			= $(SDKDIR)/mfc/lib/$(LIBCPU)
	ATLINCLUDEDIR		= $(SDKDIR)/atl/include
	ATLLIBDIR			= $(SDKDIR)/atl/lib/$(LIBCPU)
	#########################################################################
endif
SDKBINDIR				= $(SDKDIR)/bin


ifeq ($(PLATFORM),desktop)
	# DESKTOP ###############################################################
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

ifdef DEBUG
	BASENAME			= debug
	DSUFFIX				= d
	CCFLAGS				= -Z7 -Od -GZ
	DEFINES				= -D_DEBUG
	LDFLAGS				= -DEBUG -DEBUGTYPE:CV -PDB:NONE -FIXED:NO
	RCFLAGS				= -d _DEBUG
else
	BASENAME			= release
	DSUFFIX				=
	CCFLAGS				= -O1 -Oi
	DEFINES				= -DNDEBUG
	LDFLAGS				= -RELEASE -OPT:REF
	RCFLAGS				= -d NDEBUG
endif

ifneq ($(PLATFORM),desktop)
	CODE				= unicode
endif

ifeq ($(CODE),unicode)
	USUFFIX				= u
else
	USUFFIX				=
endif

SUFFIX					= $(USUFFIX)$(DSUFFIX)

ifeq ($(PLATFORM),desktop)
	# DESKTOP ###############################################################
	SUBSYSTEM			= WINDOWS
	SUBSYSVER			= 4.0
	#########################################################################
else
	# WINCE #################################################################
	ifndef EMULATION
		SUBSYSTEM		= WINDOWSCE
		ifeq ($(CEVER),300)
			SUBSYSVER	= 3.00
		endif
		ifeq ($(CEVER),211)
			SUBSYSVER	= 2.11
		endif
		ifeq ($(CEVER),201)
			SUBSYSVER	= 2.01
		endif
		ifeq ($(CEVER),200)
			SUBSYSVER	= 2.00
		endif
	else
		SUBSYSTEM		= WINDOWS
		SUBSYSVER		= 4.0
	endif
	#########################################################################
endif

CCFLAGS					+= -nologo -W3 -GF -Gy -Zp8 -X
DEFINES					+= -DWIN32 -D_WIN32 -D_MT -DSTRICT
LDFLAGS					+= -NOLOGO -INCREMENTAL:NO -SUBSYSTEM:$(SUBSYSTEM),$(SUBSYSVER)
RCFLAGS					+= -l 0x411
MIDLFLAGS				= -Oicf
ifeq ($(PLATFORM),desktop)
	# DESKTOP ###############################################################
	CCFLAGS				+= -GX -GB -MD$(DSUFFIX)
	DEFINES				+= -DMT -D_DLL -DWINVER=0x400 -D_WIN32_WINNT=0x400 -D_WIN32_IE=0x400 -DTAPI_CURRENT_VERSION=0x00010004
	ifeq ($(CODE),unicode)
		DEFINES			+= -DUNICODE -D_UNICODE
	endif
	LDFLAGS				+= -MACHINE:I386
	
	LIBS				= msvcrt$(DSUFFIX).lib \
						  msvcirt$(DSUFFIX).lib \
						  user32.lib \
						  kernel32.lib \
						  wsock32.lib \
						  gdi32.lib \
						  shell32.lib \
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
						  mapiw32.lib
	ifdef KCTRL
		LIBS			+= $(KCTRLDIR)/lib/x86uni/kctrl.lib
	endif
	#########################################################################
else
	# WINCE #################################################################
	DEFINES				+= -D_WIN32_WCE=$(CEVER) -DUNDER_CE=$(CEVER) -DUNICODE -D_UNICODE
	LDFLAGS				+= -NODEFAULTLIB
	RCFLAGS				+=  -D _WIN32_WCE=$(CEVER) -D UNDER_CE=$(CEVER)
	
	LIBCPU				= $(CPU)
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
	endif
	ifeq ($(CPU),x86em)
		CCFLAGS			+= -Gz
		DEFINES			+= -Dx86 -D_X86_ -Di486
		LDFLAGS			+= -MACHINE:IX86
		RCFLAGS			+= -D x86 -D _X86 -D i486
	endif
	
	ifeq ($(BASEPLATFORM),ppc)
		DEFINES			+= -D_WIN32_WCE_PSPC -DWIN32_PLATFORM_PSPC
		RCFLAGS			+= -D _WIN32_WCE_PSPC -DWIN32_PLATFORM_PSPC
	endif
	
	ifndef EMULATION
		CCFLAGS		+= -MC
		DEFINES		+= -D_DLL
	else
		DEFINES			+= -D_WIN32_WCE_EMULATION
		RCFLAGS			+= -D _WIN32_WCE_EMULATION
	endif
	
	LIBS				= coredll.lib \
						  corelibc.lib \
						  winsock.lib \
						  ole32.lib \
						  oleaut32.lib \
						  uuid.lib \
						  commctrl.lib \
						  ceshell.lib \
						  commdlg.lib
	ifeq ($(BASEPLATFORM),ppc)
		LIBS			+= aygshell.lib
	endif
	ifdef KCTRL
		LIBS			+= $(KCTRLDIR)/lib/$(LIBCPU)/kctrl.lib
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

ifeq ($(PLATFORM),desktop)
	# DESKTOP ###############################################################
	OBJDIR				= $(OBJDIRBASE)/$(PLATFORM)/$(CODE)/$(BASENAME)
	TARGETDIR			= $(TARGETDIRBASE)/$(PLATFORM)/$(CODE)/$(BASENAME)
	#########################################################################
else
	# WINCE #################################################################
	OBJDIR				= $(OBJDIRBASE)/$(PLATFORM)/$(CPU)/$(BASELANG)/$(BASENAME)
	TARGETDIR			= $(TARGETDIRBASE)/$(PLATFORM)/$(CPU)/$(BASELANG)/$(BASENAME)
	#########################################################################
endif
TLBDIR					= $(TLBDIRBASE)/$(PLATFORM)
TARGET					= $(PROJECTNAME)$(SUFFIX).$(EXTENSION)


# STLPORT ###################################################################
INCLUDES				= -I$(STLPORTDIR)

STLPORTFLAGS			= -D_STLP_NO_IOSTREAMS
ifdef DEBUG
	STLPORTFLAGS		+= -D_STLP_USE_NEWALLOC
endif
ifeq ($(PLATFORM),desktop)
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

ifndef BUILDEXTRA
	BUILDEXTRA			= 0
endif
BUILDNUMBER				= $(shell echo $$((`date +%-y` << 11 | `date +%-m` << 7 | `date +%-d` << 2 | $(BUILDEXTRA))))
NVERSION				= $(shell cat version | tr '.' ','),$(BUILDNUMBER)
SVERSION				= $(shell cat version | sed -e 's/\./, /g'), $(BUILDNUMBER)
RCDEFINES				= -DNVERSION="$(NVERSION)" -DSVERSION="\"$(SVERSION)\"" -DSUFFIX="\"$(SUFFIX)\""

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
ifeq ($(PLATFORM),desktop)
	LIBDIRBASE			= $(PLATFORM)/$(CODE)/$(BASENAME)
else
	LIBDIRBASE			= $(PLATFORM)/$(CPU)/$(BASELANG)/$(BASENAME)
endif
DEPENDLIBS				= $(foreach L,$(LIBRARIES),../$(L)/lib/$(LIBDIRBASE)/$(shell F=../$(L)/libname; if [ -f $$F ]; then cat $$F; else echo $(L); fi)$(SUFFIX).lib)
LIBS					+= $(DEPENDLIBS)

INCLUDES				+= $(EXTERNALINCS)
LIBS					+= $(EXTERNALLIBS)

export PATH				= $(shell cygpath -u "$(BINDIR)"):$(shell cygpath -u "$(SDKBINDIR)"):$(shell cygpath -u "$(COMPILERBINDIR)"):$(shell cygpath -u "$(COMMONBINDIR)")
export INCLUDE			= $(SDKINCLUDEDIR);$(MFCINCLUDEDIR);$(ATLINCLUDEDIR);$(COMPILERINCLUDEDIR)
export LIB				= $(SDKLIBDIR);$(MFCLIBDIR);$(ATLLIBDIR);$(COMPILERLIBDIR)


target: $(TARGETDIR)/$(TARGET)

clean:
	-for d in $(OBJDIRBASE) $(TLBDIRBASE) $(TARGETDIRBASE); do \
		if [ -d $$d ]; then rm -rf $$d; fi \
	done
	-rm -f version

clean.desktop:
	-for d in $(OBJDIRBASE) $(TLBDIRBASE) $(TARGETDIRBASE); do \
		if [ -d $$d/desktop ]; then rm -rf $$d/desktop; fi \
	done
	-rm -f version

clean.wce:
	-for d in $(OBJDIRBASE) $(TLBDIRBASE) $(TARGETDIRBASE); do \
		for p in ppc2002 hpc2000 ppc hpcpro; do \
			if [ -d $$d/$$p ]; then rm -rf $$d/$$p; fi \
		done \
	done
	-rm -f version

$(OBJDIR)/%.obj: $(SRCDIR)/%.cpp
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(CCC) $(CCFLAGS) $(DEFINES) $(INCLUDES) -c -Fo$@ $<

$(OBJDIR)/%.obj: $(SRCDIR)/%.c
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(CCC) $(CCFLAGS) $(DEFINES) $(INCLUDES) -c -Fo$@ $<

$(OBJDIR)/%.res: $(SRCDIR)/%.rc $(TLBS)
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(RC) $(RCFLAGS) $(RCDEFINES) -fo $@ $<

$(TLBDIR)/%.tlb: $(SRCDIR)/%.idl
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(MIDL) $(MIDLFLAGS) -out $(TLBDIR) -h $(notdir $(patsubst %.idl, %.h, $<)) -tlb $(notdir $(patsubst %.idl, %.tlb, $<)) $<

$(OBJDIR)/%.d: $(SRCDIR)/%.cpp
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(GCC) $(DEFINES) $(PROJECTINCLUDES) -DDEPENDCHECK -MM -MG -nostdinc $< 2>/dev/null | sed -e 's#.*:#$(@:.d=.obj) $@ :#g' >$@

$(OBJDIR)/%.d: $(SRCDIR)/%.c
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(GCC) $(DEFINES) $(PROJECTINCLUDES) -DDEPENDCHECK -MM -MG -nostdinc $< 2>/dev/null | sed -e 's#.*:#$(@:.d=.obj) $@ :#g' >$@

$(TARGETDIR)/$(TARGET): $(TLBS) $(OBJS) $(RESES) $(DEPENDLIBS)
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	case "$(PROJECTTYPE)" in \
	exe) \
		$(LD) $(LDFLAGS) -OUT:$@ $(OBJS) $(RESES) $(LIBS); \
		;; \
	dll) \
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
		fi; \
		;; \
	lib) \
		$(AR) -OUT:$@ $(OBJS); \
		;; \
	esac

ifneq ($(PLATFORM),)
    ifndef NODEPEND
        -include $(OBJS:.obj=.d)
    endif
endif

include ../targets.mak
