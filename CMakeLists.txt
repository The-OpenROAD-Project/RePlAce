cmake_minimum_required(VERSION 3.1)

project(RePlAce)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# For Multithread
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dcimg_use_jpeg=1 -DUSE_FFT2D_PTHREADS=1 -DFFT_2D_MAX_THREADS=16 -O3 ${OpenMP_CXX_FLAGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dcimg_use_jpeg=1 -O3 -g -ggdb")
set(CMAKE_CXX_FLAGS_DEBUG "-g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(REPLACE_HOME ${PROJECT_SOURCE_DIR} )
set(DEFLIB_HOME
  ${PROJECT_SOURCE_DIR}/module/def
)
set(LEFLIB_HOME
  ${PROJECT_SOURCE_DIR}/module/lef
)
set(FLUTE_HOME
  ${PROJECT_SOURCE_DIR}/module/flute
)
set(OPENSTA_HOME
  ${PROJECT_SOURCE_DIR}/module/OpenSTA
)
set(CIMG_HOME
  ${PROJECT_SOURCE_DIR}/module/CImg
)
set(EIGEN_HOME
  ${PROJECT_SOURCE_DIR}/module/eigen-git-mirror 
)

add_subdirectory(${DEFLIB_HOME})
add_subdirectory(${LEFLIB_HOME})
add_subdirectory(${FLUTE_HOME})
add_subdirectory(${OPENSTA_HOME})

set(ENV{TYPE_CALL} library)




## configure settings
find_package(GoogleHash)
find_package(TCL REQUIRED)

find_package(ZLIB REQUIRED)
find_package(SWIG REQUIRED)
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

# for cimg
find_package(X11 REQUIRED)
find_package(JPEG REQUIRED)

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

# for multi threads
find_package(OpenMP REQUIRED)

set (REPLACE_SRC 
  src/argument.cpp
  src/bin.cpp
  src/bookShelfIO.cpp
  src/charge.cpp
  src/defParser.cpp
  src/detailPlace.cpp
  src/fft.cpp
  src/fftsg2d.cpp
  src/fftsg3d.cpp
  src/fftsg.cpp
  src/gcell.cpp
  src/initPlacement.cpp
  src/lefdefIO.cpp
  src/lefParser.cpp
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
  src/replace_wrap.cpp
  )


set (REPLACE_HEADER
  src/bin.h
  src/bookShelfIO.h
  src/charge.h
  src/fft.h
  src/gcell.h
  src/replace_private.h
  src/replace_external.h
  src/initPlacement.h
  src/lefdefIO.h
  src/macro.h
  src/ns.h
  src/opt.h
  src/plot.h
  src/routeOpt.h
  src/timing.h
  src/timingSta.h
  src/wlen.h
  src/util.h
)

set (REPLACE_SWIG_FILES
  src/replace.i
)

############################################################
# include directories 
############################################################
include_directories(
  ${X11_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}

  ${EIGEN_HOME}
  ${CIMG_HOME}

  ${DEFLIB_HOME}/def
  ${DEFLIB_HOME}/defzlib
  ${DEFLIB_HOME}/cdef
  ${DEFLIB_HOME}/cdefzlib

  ${LEFLIB_HOME}/lef
  ${LEFLIB_HOME}/lefzlib
  ${LEFLIB_HOME}/clef
  ${LEFLIB_HOME}/clefzlib

  ${FLUTE_HOME}/src
 
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

)

############################################################
# Create a library 
############################################################
# add_library( replacelib  SHARED
#   ${REPLACE_SRC}
# )

# set_target_properties( replacelib 
#   PROPERTIES OUTPUT_NAME "replace"
# )

############################################################
# Create an executable
############################################################
add_custom_command(OUTPUT ${REPLACE_HOME}/src/replace_wrap.cpp
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -o ${REPLACE_HOME}/src/replace_wrap.cpp ${REPLACE_HOME}/src/replace.i   
  WORKING_DIRECTORY ${REPLACE_HOME}
  DEPENDS ${REPLACE_SWIG_FILES} src/replace_external.h src/replace_external.cpp
)

add_executable( replace
  ${REPLACE_SRC}
)

add_dependencies( replace sta )
add_dependencies( replace flute )
add_dependencies( replace def )
add_dependencies( replace lef )

#add_dependencies( replacelib sta )
#add_dependencies( replacelib flute )
#add_dependencies( replacelib def )
#add_dependencies( replacelib lef )

target_link_libraries( replace
  PUBLIC
  OpenSTA
  flute
  def
  defzlib
  cdef
  cdefzlib
  lef
  lefzlib
  clef
  clefzlib

  ${ZLIB_LIBRARIES}
  ${X11_LIBRARIES}
  ${JPEG_LIBRARIES}
  ${TCL_LIB}

  ${OpenMP_CXX_LIBRARIES}
  Threads::Threads
  )

############################################################
# Install  
############################################################

install(FILES ${REPLACE_HEADER} DESTINATION include)
install(TARGETS replace DESTINATION bin)
# install(TARGETS replacelib DESTINATION lib)


