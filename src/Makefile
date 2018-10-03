CC		=   gcc
CPPC	=   g++
LD      =   ld
OPTFLAG = 	-m64 -O3 -fPIC -DNDEBUG -ffast-math -Dcimg_display=1 
WFLAG 	= 	-W -Wall -Wextra -Wunreachable-code 
WSTRICT =	-Werror -Wundef -Wpointer-arith 

INCLUDE = 	-I$(CURDIR) -I$(MKLROOT)/include -I$(IPPROOT)/include \
			-I$(HASHROOT) -I$(EIGENROOT) -I$(CIMGROOT) \
			-I$(LEFROOT)/include -I$(DEFROOT)/include -I$(VERILOGROOT)/include

IMATH 	= 	-I$(MKLROOT)/include -I$(IPPROOT)/include

CFLAG   =   -std=c11   $(OPTFLAG)  
CPPFLAG =   -std=c++1y $(OPTFLAG)

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

DEFLIB         	= -L$(DEFROOT)/lib -ldef -ldefzlib -lcdef -lcdefzlib
LEFLIB         	= -L$(LEFROOT)/lib -llef -llefzlib -lclef -lclefzlib
VERILOGLIB 		= -L$(VERILOGROOT)/lib -lverilog_parser 

ifndef	MKLROOT
    MKLROOT		=	/home/tool/intel/compilers_and_libraries_2016.3.210/linux/mkl
endif

ifndef	IPPROOT
    IPPROOT		=	/home/tool/intel/compilers_and_libraries_2016.3.210/linux/ipp
endif

HASHROOT 		= ../module/sparsehash/install-sp/include
EIGENROOT 		= ../module/eigen-git-mirror/
CIMGROOT 		= ../module/CImg/

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

	$(CPPC) -o $@                               \
	$(CPPFLAG) 					    			\
	$^ 											\
 	-Wl,--start-group ${MKLROOT}/lib/intel64/libmkl_intel_lp64.a \
	${MKLROOT}/lib/intel64/libmkl_gnu_thread.a ${MKLROOT}/lib/intel64/libmkl_core.a \
	${IPPROOT}/lib/intel64/libippi.a  ${IPPROOT}/lib/intel64/libipps.a \
	${IPPROOT}/lib/intel64/libippcore.a  ${IPPROOT}/lib/intel64/libippvm.a \
	-Wl,--end-group -lgomp -lpthread -lm -ldl -lX11 ${LEFLIB} ${DEFLIB} ${VERILOGLIB} -static-libstdc++ 
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
