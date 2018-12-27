CC					=	gcc
CXX					=	g++
WFLAG 			= -W -Wall -Wextra -Wunreachable-code
WSTRICT 		=	-Werror -Wundef -Wpointer-arith

TCLROOT			= /usr/include/tcl8.6
INCLUDE 		= -I$(CURDIR) -I$(TCLROOT) -I$(MKLROOT)/include -I$(IPPROOT)/include \
							-I$(HASHROOT) -I$(EIGENROOT) -I$(CIMGROOT) \
							-I$(LEFROOT)/include -I$(DEFROOT)/include -I$(VERILOGROOT)/src \
		    			-I$(FLUTEROOT)/include -I$(OPENSTAROOT)/include -I$(OPENSTASRCROOT)

DEBUG				= -g -ggdb ${WFLAG} ${WSTRICT}

FLAGS 			= -c -O3 -ffast-math -fPIC -DNDEBUG -Dcimg_display=1
CFLAGS	 		= ${FLAGS} -std=c99
CXXFLAGS 		=	${FLAGS} -std=c++17

LDFLAGS 		= -Wl,--start-group ${MKLLIB} ${OPENSTALIB} -Wl,--end-group \
							-lgomp -lpthread -lm -ldl -lX11 -ltcl8.6 -lz \
							${LEFLIB} ${DEFLIB} ${VERILOGLIB} ${FLUTELIB} -lc -lstdc++ 	

#Get Only the Internal Structure of SRCDIR Recursively
SRCDIR			=	$(CURDIR)
STRUCT			= $(shell find $(SRCDIR) -type d)

#Get All Files inside the STRUCTURE Variable
ALLSRCs 		=   $(addsuffix /*,$(STRUCT))
ALLSRCs 		:= 	$(wildcard $(ALLSRCs))

#Filter Only Specific Files                                
CFILES  		= 	$(filter %.c,$(ALLSRCs))
CPPFILES		= 	$(filter %.cpp,$(ALLSRCs))
HFILES 			= 	$(filter %.h,$(ALLSRCs))
COBJ				= 	$(subst $(SRCDIR),$(CURDIR),$(CFILES:%.c=%.o))
CPPOBJ			= 	$(subst $(SRCDIR),$(CURDIR),$(CPPFILES:%.cpp=%.o))
OBJFILES		= 	$(COBJ) $(CPPOBJ)

DEPENDS			=   $(HFILES)

LEFROOT 		= ../module/lef/5.8-p027
DEFROOT 		= ../module/def/5.8-p027
VERILOGROOT	= ../module/verilog-parser
FLUTEROOT 	= ../module/flute

DEFLIB      = -L$(DEFROOT)/lib -ldef -ldefzlib -lcdef -lcdefzlib
LEFLIB      = -L$(LEFROOT)/lib -llef -llefzlib -lclef -lclefzlib
VERILOGLIB 	= -L$(VERILOGROOT)/lib -lverilog_parser
FLUTELIB 		= -L$(FLUTEROOT)/lib -lflute
MKLLIB 			= ${MKLROOT}/lib/intel64/libmkl_intel_lp64.a \
							${MKLROOT}/lib/intel64/libmkl_gnu_thread.a \
							${MKLROOT}/lib/intel64/libmkl_core.a \
							${IPPROOT}/lib/intel64/libippi.a \
							${IPPROOT}/lib/intel64/libipps.a \
							${IPPROOT}/lib/intel64/libippcore.a \
							${IPPROOT}/lib/intel64/libippvm.a

OPENSTALIB	= $(OPENSTAROOT)/lib/libliberty.a \
							$(OPENSTAROOT)/lib/libutil.a \
							$(OPENSTAROOT)/lib/libsearch.a \
							$(OPENSTAROOT)/lib/libdcalc.a \
							$(OPENSTAROOT)/lib/libgraph.a \
							$(OPENSTAROOT)/lib/libnetwork.a \
							$(OPENSTAROOT)/lib/libparasitics.a \
							$(OPENSTAROOT)/lib/libsdc.a \
							$(OPENSTAROOT)/lib/libsdf.a \
							$(OPENSTAROOT)/lib/libverilog.a

ifndef	MKLROOT
    MKLROOT		=	/home/tool/intel/MKL/2018_0_128/compilers_and_libraries_2018.0.128/linux/mkl
endif

ifndef	IPPROOT
    IPPROOT		=	/home/tool/intel/MKL/2018_0_128/compilers_and_libraries_2018.0.128/linux/ipp
endif

HASHROOT			= ../module/sparsehash/install-sp/include
EIGENROOT 		= ../module/eigen-git-mirror
CIMGROOT 			= ../module/CImg
OPENSTAROOT 	= ../module/OpenSTA/install-sp
OPENSTASRCROOT= ../module/OpenSTA

### RULES ###############
%.o: %.c $(DEPENDS)
	@echo " "
	@echo "  Building... $@"
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $< 

%.o: %.cpp $(DEPENDS)
	@touch main.cpp
	@echo " "
	@echo "  Building... $@"
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ $< 


RePlAce: $(OBJFILES) $(OPENSTASRCROOT)/app/StaApp_wrap.o \
	$(OPENSTASRCROOT)/app/StaMain.o \
	$(OPENSTASRCROOT)/app/TclInitVar.o
	@echo " "
	@echo "  Linking..."
	@echo "    Intel Math Kernel Library (MKL 11.3.3) and"
	@echo "    Intel Integrated Performance Primitives (IPP 9.0.3) have been included"
	@echo "	   with static libs..."
	@echo "    If you want to replace them into shared libs, do '-L PATH -lippi,' etc."
	@echo " "

	$(CXX) -o $@ $(CPPFLAG) $^ ${LDFLAGS}
	@echo " "
	@echo "  Linking is completed !!"
	@echo " "
	@echo "  Tagging..."
	ctags -R *
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

tag :
	ctags -R *
