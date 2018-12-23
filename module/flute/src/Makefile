EXE = flute 

CXX = g++

CXXOPTS = -m64 -fPIC -fno-strict-aliasing -fexceptions -Wno-ctor-dtor-privacy -fno-permissive
CFLAGS =  -O1 -std=c++17 -fPIC  -fno-permissive

PREFIX = ..

# linking flag settings
LINKCFLAGS = $(CFLAGS) -static-libstdc++
 
DEBUG = -g -gstrict-dwarf -gdwarf-2

.PHONY: all
all: $(EXE) static

$(EXE): flute.o main.o 
	$(CXX) $(DEBUG) $(CXXOPTS) $^ -o $@ $(LINKCFLAGS)

# Verilog Traversing Module
flute.o: flute.cpp flute.h dbu.h dim.h 
	$(CXX) $(CFLAGS) -c -o $@ $<

main.o: main.cpp flute.h dbu.h dim.h 
	$(CXX) $(CFLAGS) -c -o $@ $<

.PHONY: static
static: libflute.a

libflute.a: flute.o 
	ar rc $@ $^
	mkdir -p $(PREFIX)/lib
	mv $@ $(PREFIX)/lib/$@
	mkdir -p $(PREFIX)/include
	cp *.h $(PREFIX)/include

.PHONY: clean
clean: 
	/bin/rm -f *.o $(EXE) $(PREFIX)/lib/libflute.a $(PREFIX)/include/*.h 2> /dev/null;


