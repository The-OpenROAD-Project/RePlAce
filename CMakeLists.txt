# Note that this file belongs in replace/src, not here.

cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(USE_CIMG_LIB "Use CImg drawing library for image drawing" OFF)
if( USE_CIMG_LIB )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dcimg_use_jpeg=1 -O3 -D_GLIBCXX_USE_CXX11_ABI=0nnnn")
  set(CIMG_HOME ${PROJECT_SOURCE_DIR}/module/CImg)
else()
  add_definitions(-DDISABLE_CIMG_LIB=1)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -D_GLIBCXX_USE_CXX11_ABI=0")
endif()

# For Multithread
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dcimg_use_jpeg=1 -DUSE_FFT2D_PTHREADS=1 -DFFT_2D_MAX_THREADS=16 -O3 ${OpenMP_CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(REPLACE_HOME ${PROJECT_SOURCE_DIR}/src/replace )
set(FLUTE_HOME ${PROJECT_SOURCE_DIR}/src/flute3)
set(OPENSTA_HOME ${PROJECT_SOURCE_DIR}/src/OpenSTA)

set(EIGEN_HOME
  ${REPLACE_HOME}/module/eigen-git-mirror 
)

set(OPENROAD_HOME ${PROJECT_SOURCE_DIR})
set(OPENDB_HOME ${PROJECT_SOURCE_DIR}/src/OpenDB)

## configure settings
#find_package(GoogleHash)
find_package(TCL REQUIRED)

find_package(ZLIB REQUIRED)

# for cimg
if( USE_CIMG_LIB )
  find_package(X11 REQUIRED)
  find_package(JPEG REQUIRED)
  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${X11_LIBRARIES} ${JPEG_LIBRARIES}")
else()
  find_package(X11)
  find_package(JPEG)
endif()


find_package(Boost REQUIRED)

set(REPLACE_WRAP ${CMAKE_CURRENT_BINARY_DIR}/replace_wrap.cpp)
set(REPLACE_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/ReplaceTclInitVar.cc)

set (REPLACE_SRC 
  src/argument.cpp
  src/bin.cpp
  src/charge.cpp
  src/fft.cpp
  src/fftsg2d.cpp
  src/fftsg3d.cpp
  src/fftsg.cpp
  src/gcell.cpp
  src/initPlacement.cpp
  src/macro.cpp
  src/main.cpp
  src/util.cpp
  src/ns.cpp
  src/opt.cpp
  src/plot.cpp
  src/routeOpt.cpp
  src/tier.cpp
  src/timing.cpp
  src/timingSta.cpp
  src/trial.cpp
  src/wlen.cpp
  src/replace_external.cpp
  src/replace_private.cpp
  src/replace_private.cpp
  src/dbLefDefIO.cpp
  src/MakeReplace.cpp
  ${REPLACE_WRAP}
  ${REPLACE_TCL_INIT}
  )

set (REPLACE_HEADER
  src/bin.h
  src/charge.h
  src/fft.h
  src/gcell.h
  src/replace_private.h
  src/replace_external.h
  src/initPlacement.h
  src/macro.h
  src/ns.h
  src/opt.h
  src/plot.h
  src/routeOpt.h
  src/timing.h
  src/timingSta.h
  src/wlen.h
  src/util.h
  src/dbLefDefIO.h
  src/MakeReplace.h
)

add_custom_command(OUTPUT ${REPLACE_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -o ${REPLACE_WRAP} ${REPLACE_HOME}/src/replace.i
  WORKING_DIRECTORY ${REPLACE_HOME}
  DEPENDS ${REPLACE_HOME}/src/replace.i "src/replace_external.h"
)

add_custom_command(OUTPUT ${REPLACE_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${REPLACE_TCL_INIT} replace_tcl_inits ${REPLACE_HOME}/src/replace.tcl
  WORKING_DIRECTORY ${REPLACE_HOME}/src
  DEPENDS ${REPLACE_HOME}/src/replace.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

############################################################
# Create a library 
############################################################
add_library( replace
  ${REPLACE_SRC}
)

############################################################
# include directories 
############################################################
target_include_directories( replace
PUBLIC
  ${REPLACE_HOME}/src
PRIVATE
  ${Boost_INCLUDE_DIR}

  ${EIGEN_HOME}
  ${FLUTE_HOME}
 
  ${OPENSTA_HOME}/app
  ${OPENSTA_HOME}/dcalc
  ${OPENSTA_HOME}/graph
  ${OPENSTA_HOME}/liberty
  ${OPENSTA_HOME}/network
  ${OPENSTA_HOME}/parasitics
  ${OPENSTA_HOME}/sdc
  ${OPENSTA_HOME}/sdf
  ${OPENSTA_HOME}/search
  ${OPENSTA_HOME}/tcl
  ${OPENSTA_HOME}/util
  ${OPENSTA_HOME}/verilog

  ${OPENDB_HOME}/include/opendb
  # This SHOULD be ${OPENDB_HOME}/include but all library references are wrong
  # in replace -cherry
  ${OPENDB_HOME}/include/opendb
  ${OPENDB_HOME}/include
  ${OPENROAD_HOME}/include
)
