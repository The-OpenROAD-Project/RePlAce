project(flute)

cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FLUTE_HOME ${CMAKE_CURRENT_SOURCE_DIR})

set( FLUTE_SRC 
  src/main.cpp
  src/flute.cpp 
  )

add_library( flute STATIC ${FLUTE_SRC} )

set_target_properties( flute 
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${FLUTE_HOME}/lib
  LIBRARY_OUTPUT_DIRECTORY ${FLUTE_HOME}/lib
  RUNTIME_OUTPUT_DIRECTORY ${FLUTE_HOME}/lib
  )

target_include_directories( flute 
  PUBLIC 
  src/
  )
