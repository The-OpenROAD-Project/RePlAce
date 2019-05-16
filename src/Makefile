CC		=   gcc
CPPC	=   g++
LD      =   ld
# For Multi-thread
# OPTFLAG = 	-g -ggdb -m64 -O3 -fPIC -DNDEBUG -ffast-math -Dcimg_display=1 -fopenmp -DUSE_FFT2D_PTHREADS=1 -DFFT_2D_MAX_THREADS=16 
OPTFLAG = -m64 -O3 -fPIC -DNDEBUG -ffast-math -Dcimg_display=1 
WFLAG 	= -W -Wall -Wextra -Wunreachable-code 
WSTRICT =	-Werror -Wundef -Wpointer-arith 

INCLUDE = -I$(CURDIR) -I$(MKLROOT)/include -I$(IPPROOT)/include \
			-I$(HASHROOT) -I$(EIGENROOT) -I$(CIMGROOT) \
			-I$(LEFROOT)/include -I$(DEFROOT)/include -I$(VERILOGROOT)/include \
			-I$(FLUTEROOT)/include -I$(OPENSTAROOT)/include -I$(OPENSTASRCROOT)

CPPFLAG =   -std=c++0x $(OPTFLAG)

#Get Only the Internal Structure of SRCDIR Recursively
SRCDIR	= 	$(CURDIR)
STRUCT	=   $(shell find $(SRCDIR) -type d)

#Get All Files inside the STRUCTURE Variable
ALLSRCs =   $(addsuffix /*,$(STRUCT))
ALLSRCs := 	$(wildcard $(ALLSRCs))

#Filter Only Specific Files                                
CFILES  		= 	$(filter %.c,$(ALLSRCs))
CPPFILES		= 	$(filter %.cpp,$(ALLSRCs))
HFILES 			= 	$(filter %.h,$(ALLSRCs))
COBJ			= 	$(subst $(SRCDIR),$(CURDIR),$(CFILES:%.c=%.o))
CPPOBJ			= 	$(subst $(SRCDIR),$(CURDIR),$(CPPFILES:%.cpp=%.o))
OBJFILES		= 	$(COBJ) $(CPPOBJ)

DEPENDS			=   $(HFILES)

LEFROOT 		= ../module/lef/5.8-p027
DEFROOT 		= ../module/def/5.8-p027
VERILOGROOT 	= ../module/verilog-parser
FLUTEROOT 		= ../module/flute

DEFLIB         	= -L$(DEFROOT)/lib -ldef -ldefzlib -lcdef -lcdefzlib
LEFLIB         	= -L$(LEFROOT)/lib -llef -llefzlib -lclef -lclefzlib
VERILOGLIB 		= -L$(VERILOGROOT)/lib -lverilog_parser
FLUTELIB 		= -L$(FLUTEROOT)/lib -lflute
MKLLIB 			= ${MKLROOT}/lib/intel64/libmkl_intel_lp64.a \
					${MKLROOT}/lib/intel64/libmkl_gnu_thread.a \
					${MKLROOT}/lib/intel64/libmkl_core.a \
					${IPPROOT}/lib/intel64/libippi.a \
				   	${IPPROOT}/lib/intel64/libipps.a \
					${IPPROOT}/lib/intel64/libippcore.a \
				   	${IPPROOT}/lib/intel64/libippvm.a

OPENSTALIB 		= $(OPENSTAROOT)/lib/libOpenSTA.a 

ifndef	MKLROOT
    MKLROOT		=	/opt/intel/mkl
endif

ifndef	IPPROOT
    IPPROOT		=	/opt/intel/ipp
endif

HASHROOT 		= ../module/sparsehash/install-sp/include
EIGENROOT 		= ../module/eigen-git-mirror/
CIMGROOT 		= ../module/CImg/
OPENSTAROOT 	= ../module/OpenSTA/install-sp/
OPENSTASRCROOT 	= ../module/OpenSTA/

### RULES ###############
%.o:		%.c 	$(DEPENDS)
	@echo " "
	@echo "  Building... $@"
	$(CPPC) $(CPPFLAG) $(IFLUTE) -c -o $@ $< 

%.o:		%.cpp 	$(DEPENDS)
	@touch main.cpp
	@echo " "
	@echo "  Building... $@"
	$(CPPC) $(CPPFLAG) $(INCLUDE) -c -o $@ $< 

RePlAce	: $(OBJFILES) 
	@echo " "
	@echo "  Linking..."
	@echo "    Intel Math Kernel Library (MKL 11.3.3) and"
	@echo "    Intel Integrated Performance Primitives (IPP 9.0.3) have been included"
	@echo "	   with static libs..."
	@echo "    If you want to replace them into shared libs, do '-L PATH -lippi,' etc."
	@echo " "

	$(CPPC) -o $@ $(CPPFLAG) $^ \
 	-Wl,--start-group ${MKLLIB} ${OPENSTALIB} -Wl,--end-group \
	-lgomp -lpthread -lm -ldl -lX11 -ltcl8.5 -lz \
	${LEFLIB} ${DEFLIB} ${VERILOGLIB} ${FLUTELIB} -static-libstdc++ 	
	@echo " "
	@echo "  Linking is completed !!"
	@echo " "
	@echo "- Compiling RePlAce is done."
	@echo " "
	@echo "GNU GCC/G++ compiler has been used."
	@gcc --version
	@echo " "

clean : 
	@echo "Delete: OBJs"
	@rm -rvf $(OBJFILES) 
	@echo "Delete: binary"
	@rm -rvf $(CURDIR)/RePlAce

