# $Id$
#
# Common makefile
#
# Targets
#     target
#     clean
#     clean.win
#     clean.wce

VS6DIR					= C:/Program Files/Microsoft Visual Studio
VC6DIR					= $(VS6DIR)/VC98
VS7DIR					= C:/Program Files/Microsoft Visual Studio .NET 2003
VC7DIR					= $(VS7DIR)/Vc7
VS8DIR					= C:/Program Files/Microsoft Visual Studio 8
VC8DIR					= $(VS8DIR)/VC
VCVER					= 8
EVC3DIR					= C:/Program Files/Microsoft eMbedded Tools/EVC
EVC4DIR					= C:/Program Files/Microsoft eMbedded C++ 4.0/EVC
EVCVER					= 4
PLATFORMSDKDIR			= C:/Program Files/Microsoft SDKs/Windows/v6.0
OLDPLATFORMSDKDIR		= C:/Program Files/Microsoft Platform SDK for Windows Server 2003 R2
CESDKWM5JADIR			= C:/Program Files/Windows CE Tools/wce500/Windows Mobile 5.0 Pocket PC SDK
CESDKWM5ENDIR			= C:/Program Files/Windows CE Tools/wce500/Windows Mobile 5.0 Pocket PC SDK
CESDKPPC2003JADIR		= C:/Program Files/Windows CE Tools/wce420/POCKET PC 2003
CESDKPPC2003ENDIR		= C:/Program Files/Windows CE Tools/wce420/POCKET PC 2003
CESDKSIGIIIDIR			= C:/Program Files/Windows CE Tools/wce410/sigmarionIII SDK
CESDKPPC2002JADIR		= C:/Program Files/Windows CE Tools/wce300/Pocket PC 2002
CESDKPPC2002ENDIR		= C:/Program Files/Windows CE Tools/wce300/Pocket PC 2002
CESDKHPC2000JADIR		= C:/Program Files/Windows CE Tools/wce300/hpc2000
CESDKHPC2000ENDIR		= C:/Program Files/Windows CE Tools/wce300/hpc2000
CESDKPPCJADIR			= C:/Program Files/Windows CE Tools/wce300/MS Pocket PC
CESDKPPCENDIR			= C:/Program Files/Windows CE Tools/wce300/MS Pocket PC
CESDKHPCPROJADIR		= C:/Program Files/Windows CE Tools/wce211/MS HPC Pro
CESDKHPCPROENDIR		= C:/Program Files/Windows CE Tools/wce211/MS HPC Pro
SVNDIR					= C:/Program Files/Subversion
STLPORTDIR				= ../../lib/stlport
BOOSTDIR				= ../../lib/boost
KCTRLDIR				= ../../lib/kctrl

-include ../env.mak


ifeq ($(PROJECTNAME),)
    $(error PROJECTNAME required.)
endif

ifeq ($(PROJECTTYPE),)
    $(error PROJECTTYPE required.)
endif


include ../function.mak


ifdef OLDWINDOWS
	VCVER				= 6
	PLATFORMSDKDIR		= $(OLDPLATFORMSDKDIR)
endif
ifeq ($(call cever,-ge,500),0)
	EVCVER				= 8
endif

ifdef EMULATION
	ifeq ($(if $(call platform,ppc2002),1,$(call cever,-lt,400)),0)
		OLDEMULATION	= 1
	endif
endif

ifeq ($(PLATFORM),win)
	# WINDOWS ###############################################################
	SDKDIR					= $(PLATFORMSDKDIR)
	ifeq ($(VCVER),8)
		COMPILERDIR			= $(VC8DIR)
		COMMONBINDIR		= $(VS8DIR)/common7/ide
		COMMONTOOLBINDIR	= $(VS8DIR)/common7/tools/bin
		COMMONTOOL2BINDIR	= $(VC8DIR)/vcpackages
	endif
	ifeq ($(VCVER),7)
		COMPILERDIR			= $(VC7DIR)
		COMMONBINDIR		= $(VS7DIR)/common7/ide
	endif
	ifeq ($(VCVER),6)
		COMPILERDIR			= $(VC6DIR)
		COMMONBINDIR		= $(VS6DIR)/common/msdev98/bin
		COMMONTOOLBINDIR	= $(VS6DIR)/common/msdev98/bin
	endif
	COMPILERBINDIR			= $(COMPILERDIR)/bin
	SDKCOMMONBINDIR			= $(SDKDIR)/bin
	ifeq ($(CPU),x86)
		SDKBINDIR			= $(SDKDIR)/bin
		
		SDKINCLUDEDIR		= $(SDKDIR)/include
		SDKLIBDIR			= $(SDKDIR)/lib
		ifneq ($(VCVER),6)
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
		ifeq ($(VCVER),8)
			COMPILER64BINDIR= $(COMPILERDIR)/bin/x86_amd64
		else
			SDKBINDIR		= $(SDKDIR)/bin/win64/x86/amd64
		endif
		
		SDKINCLUDEDIR		= $(SDKDIR)/include
		SDKLIBDIR			= $(SDKDIR)/lib/x64
		ifeq ($(VCVER),8)
			MFCINCLUDEDIR		= $(COMPILERDIR)/atlmfc/include
			MFCLIBDIR			= $(COMPILERDIR)/atlmfc/lib/amd64
			ATLINCLUDEDIR		= $(COMPILERDIR)/atlmfc/include
			ATLLIBDIR			= $(COMPILERDIR)/atlmfc/lib/amd64
			COMPILERINCLUDEDIR	= $(COMPILERDIR)/include
			COMPILERLIBDIR		= $(COMPILERDIR)/lib/amd64
		else
			MFCINCLUDEDIR		= $(SDKDIR)/include/mfc
			MFCLIBDIR			= $(SDKDIR)/lib/amd64/atlmfc
			ATLINCLUDEDIR		= $(SDKDIR)/include/atl
			ATLLIBDIR			= $(SDKDIR)/lib/amd64/atlmfc
			COMPILERINCLUDEDIR	= $(SDKDIR)/include/crt
			COMPILERLIBDIR		= $(SDKDIR)/lib/amd64
		endif
	endif
	
	BASEPLATFORM			= win
	#########################################################################
else
	# WINCE #################################################################
	ifeq ($(PLATFORM),wm5)
		# WM5 ###############################################################
		ifeq ($(BASELANG),ja)
			SDKDIR		= $(CESDKWM5JADIR)
		else
			SDKDIR		= $(CESDKWM5ENDIR)
		endif
		BASEPLATFORM	= ppc
		#####################################################################
	endif
	ifneq ($(call platform,ppc2003 ppc2003se),)
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
	ifeq ($(EVCVER),8)
		COMPILERDIR			= $(VC8DIR)/ce
		ifeq ($(CPU),sh3)
			COMPILERBINDIR	= $(COMPILERDIR)/bin/x86_sh
		endif
		ifeq ($(CPU),sh4)
			COMPILERBINDIR	= $(COMPILERDIR)/bin/x86_sh
		endif
		ifeq ($(CPU),mips)
			COMPILERBINDIR	= $(COMPILERDIR)/bin/x86_mips
		endif
		ifeq ($(CPU),arm)
			COMPILERBINDIR	= $(COMPILERDIR)/bin/x86_arm
		endif
		ifeq ($(CPU),armv4)
			COMPILERBINDIR	= $(COMPILERDIR)/bin/x86_arm
		endif
		ifeq ($(CPU),armv4i)
			COMPILERBINDIR	= $(COMPILERDIR)/bin/x86_arm
		endif
		COMMONBINDIR		= $(VS8DIR)/common7/ide
		COMMONTOOLBINDIR	= $(VS8DIR)/common7/tools/bin
		COMMONTOOL2BINDIR	= $(VC8DIR)/bin
	endif
	ifeq ($(EVCVER),4)
		COMPILERDIR			= $(EVC4DIR)
		COMPILERBINDIR		= $(COMPILERDIR)/wce420/bin
		COMMONBINDIR		= $(COMPILERDIR)/../common/evc/bin
	endif
	ifeq ($(EVCVER),3)
		COMPILERDIR			= $(EVC34DIR)
		COMPILERBINDIR		= $(COMPILERDIR)/wce300/bin
		COMMONBINDIR		= $(COMPILERDIR)/../common/evc/bin
	endif
	SDKBINDIR				= $(SDKDIR)/bin
	
	ifeq ($(SDKINCLUDEDIR),)
		ifeq ($(call cever,-lt,400),0)
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
		SDKLIBDIR			= $(SDKDIR)/lib/$(LIBCPU)
		ifdef EMULATION
			ifeq ($(PLATFORM),ppc2002)
				SDKLIBDIR	= $(SDKDIR)/lib/x86
			endif
			ifeq ($(call cever,-ge,400),0)
				SDKLIBDIR	= $(SDKDIR)/lib/Emulator
			endif
		endif
	endif
	ifeq ($(call cever,-ge,500),0)
		ifeq ($(MFCINCLUDEDIR),)
			MFCINCLUDEDIR		= $(COMPILERDIR)/atlmfc/include
		endif
		ifeq ($(MFCLIBDIR),)
			MFCLIBDIR			= $(COMPILERDIR)/atlmfc/lib/$(LIBCPU)
		endif
		ifeq ($(ATLINCLUDEDIR),)
			ATLINCLUDEDIR		= $(COMPILERDIR)/atlmfc/include
		endif
		ifeq ($(ATLLIBDIR),)
			ATLLIBDIR			= $(COMPILERDIR)/atlmfc/lib/$(LIBCPU)
		endif
	else
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
	endif
	ifeq ($(EVCVER),8)
		COMPILERINCLUDEDIR	= $(COMPILERDIR)/include
	endif
	#########################################################################
endif


ifeq ($(PLATFORM),win)
	# WINDOWS ###############################################################
	CCC						= cl
	#########################################################################
else
	# WINCE #################################################################
	ifeq ($(EVCVER),8)
		CCC					= cl
	else
		ifeq ($(CPU),sh3)
			ifeq ($(EVCVER),4)
				CCC			= clsh
			else
				CCC			= shcl
			endif
		endif
		ifeq ($(CPU),sh4)
			ifeq ($(EVCVER),4)
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
	endif
	#########################################################################
endif
RC						= rc
AR						= lib
LD						= link
MIDL					= midl
MT						= mt
BSCMAKE					= bscmake
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
	ifneq ($(VCVER),6)
		ifneq ($(CPU),x64)
			CCFLAGS		+= -RTC1
		endif
	else
		CCFLAGS			+= -GZ
	endif
endif
	DEFINES				= -D_DEBUG
	RCFLAGS				= -d _DEBUG
	LDFLAGS				=
	SYMBOL				= 1
else
	BASENAME			= release
	DSUFFIX				=
	CCFLAGS				= -O2 -Oi
	DEFINES				= -DNDEBUG
	RCFLAGS				= -d NDEBUG
	LDFLAGS				= -OPT:REF -OPT:ICF
endif

ifdef SYMBOL
	CCFLAGS				+= -Zi -Fd$(OBJDIR)/
	LDFLAGS				+= -DEBUG -DEBUGTYPE:CV -PDB:$(TARGETDIR)/$(PROJECTNAME)$(MUILANGEXT).pdb -FIXED:NO
else
	LDFLAGS				+= -RELEASE
endif

ifndef DEBUG
	ifeq ($(PLATFORM),win)
		ifneq ($(VCVER),6)
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
		ifndef OLDWINDOWS
			SUBSYSVER	= 5.0
		else
			SUBSYSVER	= 4.0
		endif
	endif
	ifeq ($(CPU),x64)
		SUBSYSVER		= 5.2
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

CCFLAGS					+= -nologo -W3 -WX -GF -Gy -X
DEFINES					+= -DWIN32 -D_WIN32 -D_MT -DSTRICT
LDFLAGS					+= -NOLOGO -INCREMENTAL:NO -MAP -SUBSYSTEM:$(SUBSYSTEM),$(SUBSYSVER)
ifneq ($(MUILANG),)
	RCFLAGS				+= -l 0x$(MUILANG)
endif
MIDLFLAGS				= -Oicf
ifeq ($(PLATFORM),win)
	# WINDOWS ###############################################################
	CCFLAGS				+= -EHsc -MD$(DSUFFIX)
	DEFINES				+= -DMT -D_DLL -D_WIN32_IE=0x600
	ifeq ($(CODE),unicode)
		DEFINES			+= -DUNICODE -D_UNICODE
	endif
	ifeq ($(CPU),x86)
		ifneq ($(VCVER),8)
			CCFLAGS		+= -GB
		endif
		DEFINES			+= -Dx86 -D_X86_
		ifndef OLDWINDOWS
			DEFINES		+= -DWINVER=0x500 -D_WIN32_WINNT=0x500
		else
			DEFINES		+= -DWINVER=0x400 -D_WIN32_WINNT=0x400 -DTAPI_CURRENT_VERSION=0x00010004
		endif
		LDFLAGS			+= -MACHINE:I386
		RCFLAGS			+= -Dx86 -D_X86_
	endif
	ifeq ($(CPU),x64)
		DEFINES			+= -DWIN64 -D_WIN64 -D_AMD64_ -DWINVER=0x502 -D_WIN32_WINNT=0x502
		LDFLAGS			+= -MACHINE:AMD64
		RCFLAGS			+= -D_AMD64_
	endif
	ifeq ($(VCVER),7)
		CCFLAGS			+= -Zc:forScope
	endif
	ifeq ($(VCVER),8)
		CCFLAGS			+= -FC
		DEFINES			+= -D_CRT_SECURE_NO_DEPRECATE
	endif
	
	BSCFLAGS			= -El
	
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
						  wininet.lib \
						  version.lib
	ifeq ($(VCVER),6)
		LIBS			+= msvcirt$(DSUFFIX).lib
	endif
	ifeq ($(CPU),x64)
#		LIBS			+= bufferoverflowu.lib
	endif
	ifdef KCTRL
		LIBS			+= $(KCTRLDIR)/lib/win/$(CPU)/kctrl.lib
	endif
	#########################################################################
else
	# WINCE #################################################################
	ifeq ($(call cever,-ge,400),0)
		# WINCE >= 400 ######################################################
#		CCFLAGS			+= -GX
		#####################################################################
	endif
	DEFINES				+= -D_WIN32_WCE=0x$(CEVER) -DUNDER_CE=0x$(CEVER) -DUNICODE -D_UNICODE
	LDFLAGS				+= -NODEFAULTLIB
	RCFLAGS				+=  -D _WIN32_WCE=0x$(CEVER) -D UNDER_CE=0x$(CEVER)
	
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
	
	ifeq ($(PLATFORM),wm5)
		DEFINES			+= -D_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA
	endif
	
	ifndef EMULATION
		ifeq ($(EVCVER),8)
			CCFLAGS		+= -MD -GS-
		else
			CCFLAGS		+= -MC
		endif
		DEFINES			+= -D_DLL
	endif
	ifdef OLDEMULATION
		DEFINES			+= -D_WIN32_WCE_EMULATION
		RCFLAGS			+= -D _WIN32_WCE_EMULATION
	endif
	
	ifeq ($(EVCVER),8)
		CCFLAGS			+= -FC
	endif
	ifeq ($(EVCVER),4)
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
	ifneq ($(call platform,ppc2003se ppc2003),)
		LIBS			+= ccrtrtti.lib
	endif
	ifneq ($(call platform,wm5 ppc2003se ppc2003 ppc2002),)
		LIBS			+= wvuuid.lib
	endif
	ifeq ($(if $(call platform,ppc2002),0,$(call cever,-ge,400)),0)
		LIBS			+= urlmon.lib \
						   crypt32.lib
	endif
	ifdef KCTRL
		LIBS			+= $(KCTRLDIR)/lib/wce/$(EXLIBCPU)/kctrl.lib
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

ifeq ($(MUILANG),)
	MUILANGEXT			=
else
	MUILANGEXT			= .$(MUILANG)
endif

INCLUDEDIR				= include
SRCDIR					= src$(MUILANGEXT)
OBJDIRBASE				= obj$(MUILANGEXT)
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
MUITARGET				= $(TARGET).$(MUILANG).mui
BSCTARGET				= $(TARGETBASE).bsc


# STLPORT ###################################################################
INCLUDES				+= -I"$(STLPORTDIR)/stlport"
ifeq ($(PLATFORM),win)
	LIBS				+= $(STLPORTDIR)/lib/win/$(EXLIBCPU)/stlport$(DSUFFIX).5.1.lib
else
	LIBS				+= $(STLPORTDIR)/lib/wce/$(EXLIBCPU)/stlport$(DSUFFIX).5.1.lib
endif

STLPORTFLAGS			=
ifdef DEBUG
	STLPORTFLAGS		+= -D_STLP_USE_NEWALLOC
endif
DEFINES					+= $(STLPORTFLAGS)
#############################################################################

# BOOST #####################################################################
INCLUDES				+= -I"$(BOOSTDIR)"
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

RCHEADER				= $(dir $(subst $(OBJDIR),$(SRCDIR),$(RESES)))resource$(shell echo `basename $(RESES)` | sed -e 's/\(.*\($(BASEPLATFORM)\)\|.*\)\.res/\2/').h

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

export PATH				= /bin:$(call win2unix,$(COMPILER64BINDIR)):$(call win2unix,$(SDKBINDIR)):$(call win2unix,$(SDKCOMMONBINDIR)):$(call win2unix,$(COMPILERBINDIR)):$(call win2unix,$(COMMONBINDIR)):$(call win2unix,$(COMMONTOOLBINDIR)):$(call win2unix,$(COMMONTOOL2BINDIR)):$(call win2unix,$(SVNDIR)/bin)
ifeq ($(VCVER),8)
	export INCLUDE		= $(COMPILERINCLUDEDIR);$(SDKINCLUDEDIR);$(MFCINCLUDEDIR);$(ATLINCLUDEDIR)
	export LIB			= $(COMPILERLIBDIR);$(SDKLIBDIR);$(MFCLIBDIR);$(ATLLIBDIR)
else
	export INCLUDE		= $(SDKINCLUDEDIR);$(MFCINCLUDEDIR);$(ATLINCLUDEDIR);$(COMPILERINCLUDEDIR)
	export LIB			= $(SDKLIBDIR);$(MFCLIBDIR);$(ATLLIBDIR);$(COMPILERLIBDIR)
endif

SBRS					= $(patsubst %.obj, %.sbr, $(OBJS))


target: $(TARGETDIR)/$(TARGET)

target.mui: $(TARGETDIR)/$(MUITARGET)

target.bsc: $(TARGETDIR)/$(BSCTARGET)

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
		for p in wm5 ppc2003se ppc2003 ppc2002 hpc2000 ppc hpcpro sig3; do \
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

$(OBJDIR)/%.sbr: $(SRCDIR)/%.cpp
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(CCC) $(CCFLAGS) $(DEFINES) $(INCLUDES) -Zs -c -FR$@ $<

$(OBJDIR)/%.sbr: $(SRCDIR)/%.c
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(CCC) $(CCFLAGS) $(DEFINES) $(INCLUDES) -Zs -c -FR$@ $<

$(SRCDIR)/%.rcx: $(SRCDIR)/%.rc
	$(RCPP) $< > $@

$(OBJDIR)/%.res: $(SRCDIR)/%.rcx $(RCHEADER) $(TLBS) version revision
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(RC) $(RCFLAGS) $(RCDEFINES) -fo $@ $<

$(TLBDIR)/%.tlb: $(SRCDIR)/%.idl
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(MIDL) $(MIDLFLAGS) -out $(TLBDIR) -h $(notdir $(patsubst %.idl,%.h,$<)) -tlb $(notdir $(patsubst %.idl,%.tlb,$<)) $<

$(OBJDIR)/%.d: $(SRCDIR)/%.cpp
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(GCC) $(DEFINES) $(PROJECTINCLUDES) -DDEPENDCHECK -MM -MG -MT $(@:.d=.obj) -MT $@ -nostdinc $< 2>/dev/null | gawk -f ../dep.awk > $@

$(OBJDIR)/%.d: $(SRCDIR)/%.c
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(GCC) $(DEFINES) $(PROJECTINCLUDES) -DDEPENDCHECK -MM -MG -MT $(@:.d=.obj) -MT $@ -nostdinc $< 2>/dev/null | gawk -f ../dep.awk > $@

$(TARGETDIR)/$(TARGETBASE).exe: $(TLBS) $(OBJS) $(RESES) $(DEPENDLIBS)
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(LD) $(LDFLAGS) -OUT:$@ $(OBJS) $(RESES) $(LIBS)
	if [ "$(PLATFORM)" = "win" -a $(VCVER) -eq 8 ]; then \
		$(MT) -nologo -inputresource:$@ -manifest $@.manifest -outputresource:$@; \
	fi

$(TARGETDIR)/$(TARGETBASE).dll: $(TLBS) $(OBJS) $(RESES) $(DEPENDLIBS)
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(LD) $(LDFLAGS) -DLL -OUT:$@ $(OBJS) $(RESES) $(LIBS); \
	if [ $$? -eq 0 ]; then \
		$(DUMPBIN) -EXPORTS $(TARGETDIR)/$(TARGET) | tr '\r' ' ' | gawk -v LIBNAME=$(PROJECTNAME)$(SUFFIX) -f ../def.awk > `echo $(DEFFILE)`; \
		if [ -s "$(DEFFILE)" ]; then \
			if [ ! -z "$(EXTRADEFFILE)" -a -f "$(EXTRADEFFILE)" ]; then \
				cat `echo $(EXTRADEFFILE)` >> `echo $(DEFFILE)`; \
			fi; \
			$(LD) $(LDFLAGS) -DLL -DEF:$(DEFFILE) -BASE:$(BASEADDRESS) -OUT:$@ $(OBJS) $(RESES) $(LIBS) 2>&1 | grep -v "LNK4197" | cat; \
			if [ "$(PLATFORM)" = "win" -a $(VCVER) -eq 8 ]; then \
				$(MT) -nologo -manifest $@.manifest -outputresource:$@\;#2; \
			fi \
		else \
			exit 1; \
		fi; \
	fi

$(TARGETDIR)/$(TARGETBASE).lib: $(OBJS)
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(AR) -OUT:$@ $(OBJS)

$(TARGETDIR)/$(MUITARGET): $(OBJS) $(RESES)
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(LD) $(LDFLAGS) -DLL -OUT:$@ $(OBJS) $(RESES) $(LIBS)
	if [ "$(PLATFORM)" = "win" -a $(VCVER) -eq 8 ]; then \
		$(MT) -nologo -manifest $@.manifest -outputresource:$@\;#2; \
	fi

$(TARGETDIR)/$(TARGETBASE).bsc: $(SBRS)
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(BSCMAKE) $(BSCFLAGS) -o $@ $(SBRS)

ifneq ($(PLATFORM),)
    ifndef NODEPEND
        ifneq ($(OBJS),)
            -include $(OBJS:.obj=.d)
        endif
    endif
endif

include ../targets.mak
