# Note that this file belongs in replace/src, not here.

cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(USE_CIMG_LIB "Use CImg drawing library for image drawing" OFF)

if( USE_CIMG_LIB )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dcimg_use_jpeg=1 -D_GLIBCXX_USE_CXX11_ABI=0")
  set(CIMG_HOME ${REPLACE_HOME}/module/CImg)
  add_definitions(-DENABLE_CIMG_LIB=1)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=0")
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

set(EIGEN_HOME ${PROJECT_SOURCE_DIR}/src/eigen)

set(OPENROAD_HOME ${PROJECT_SOURCE_DIR})
set(OPENDB_HOME ${PROJECT_SOURCE_DIR}/src/OpenDB)

find_package(ZLIB REQUIRED)

# for cimg
if( USE_CIMG_LIB )
  find_package(X11 REQUIRED)
  find_package(JPEG REQUIRED)
  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${X11_LIBRARIES} ${JPEG_LIBRARIES}")
else()
endif()


set(REPLACE_WRAP ${CMAKE_CURRENT_BINARY_DIR}/replace_wrap.cpp)
set(REPLACE_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/ReplaceTclInitVar.cc)

set (REPLACE_SRC 
  src/MakeReplace.cpp
  ${REPLACE_WRAP}
  ${REPLACE_TCL_INIT}
  src/replace.cpp
  src/initialPlace.cpp
  src/nesterovPlace.cpp
  src/placerBase.cpp
  src/nesterovBase.cpp
  src/fft.cpp
  src/fftsg.cpp
  src/fftsg2d.cpp
  src/point.cpp
  src/plot.cpp
  src/logger.cpp
  )

set (REPLACE_HEADER
  include/replace/Replace.h
  include/replace/MakeReplace.h
)

add_custom_command(OUTPUT ${REPLACE_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -o ${REPLACE_WRAP} ${REPLACE_HOME}/src/replace.i
  WORKING_DIRECTORY ${REPLACE_HOME}
  DEPENDS ${REPLACE_HOME}/src/replace.i ${REPLACE_HOME}/include/replace/Replace.h 
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
  
target_include_directories( replace
  PUBLIC
  ${REPLACE_HOME}/include
  PRIVATE
  ${REPLACE_HOME}/src

  ${EIGEN_HOME}
  ${FLUTE_HOME}

  ${OPENSTA_HOME}/util
  ${OPENSTA_HOME}/liberty
  ${OPENSTA_HOME}/network
  ${OPENSTA_HOME}/graph
  ${OPENSTA_HOME}/sdc
  ${OPENSTA_HOME}/parasitics
  ${OPENSTA_HOME}/search
  ${OPENSTA_HOME}/app

  ${OPENDB_HOME}/include
  ${OPENROAD_HOME}/include
  ${TCL_INCLUDE_PATH}
)

# CIMG_USE case
if( USE_CIMG_LIB )
  target_link_libraries( replace
    PRIVATE
    ${X11_LIBRARIES} 
    ${JPEG_LIBRARIES} 
  )
  target_include_directories( replace
    PUBLIC
    # For CImg
    ${REPLACE_HOME}/module/CImg
    ${X11_INCLUDE_DIR}
    ${JPEG_INCLUDE_DIR}
  )
endif()
