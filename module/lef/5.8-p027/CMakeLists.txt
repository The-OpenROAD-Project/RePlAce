# LEF PARSER CMAKE Compile Settings
project(LEF_PARSER 
  LANGUAGES CXX 
)

cmake_minimum_required(VERSION 2.6)

set(LEFLIB_HOME ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS "leflib_home: ${LEFLIB_HOME}")

set(CMAKE_CXX_FLAGS_DEBUG "-g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# BISON and ZLIB is required to compile LEF/LEF Parsers.
find_package(BISON)
find_package(ZLIB)

############################################################
# Create a library for LEF
############################################################
set( LEFLIB_LEF_SRC
  lef/crypt.cpp
  lef/lefiArray.cpp
  lef/lefiCrossTalk.cpp
  lef/lefiDebug.cpp
  lef/lefiEncryptInt.cpp
  lef/lefiLayer.cpp
  lef/lefiMacro.cpp
  lef/lefiMisc.cpp
  lef/lefiNonDefault.cpp
  lef/lefiProp.cpp
  lef/lefiPropType.cpp
  lef/lefiTBExt.cpp
  lef/lefiUnits.cpp
  lef/lefiVia.cpp
  lef/lefiViaRule.cpp
  lef/lef_keywords.cpp
  lef/lefrCallbacks.cpp
  lef/lefrData.cpp
  lef/lefrReader.cpp
  lef/lefrSettings.cpp
  lef/lef.tab.cpp
  lef/lefwWriterCalls.cpp
  lef/lefwWriter.cpp
  )

set( LEFLIB_LEFZLIB_SRC 
  lefzlib/lefzlib.cpp
  )

set ( LEFLIB_CLEF_SRC 
  clef/lefiEncryptExt.c
  clef/lefiTBInt.c
  clef/xlefiArray.cpp
  clef/xlefiCrossTalk.cpp
  clef/xlefiDebug.cpp
  clef/xlefiEncryptInt.cpp
  clef/xlefiLayer.cpp
  clef/xlefiMacro.cpp
  clef/xlefiMisc.cpp
  clef/xlefiNonDefault.cpp
  clef/xlefiProp.cpp
  clef/xlefiPropType.cpp
  clef/xlefiUnits.cpp
  clef/xlefiUtil.cpp
  clef/xlefiVia.cpp
  clef/xlefiViaRule.cpp
  clef/xlefrReader.cpp
  clef/xlefwWriterCalls.cpp
  clef/xlefwWriter.cpp
)

set (LEFLIB_CLEFZLIB_SRC
  clefzlib/clefzlib.c
  )

set (LEFLIB_HEADERS
  lef/lef.tab.h
  lef/lex.h
  lef/crypt.hpp
  lef/lefiArray.hpp
  lef/lefiCrossTalk.hpp
  lef/lefiDebug.hpp
  lef/lefiDefs.hpp
  lef/lefiEncryptInt.hpp
  lef/lefiKRDefs.hpp
  lef/lefiLayer.hpp
  lef/lefiMacro.hpp
  lef/lefiMisc.hpp
  lef/lefiNonDefault.hpp
  lef/lefiProp.hpp
  lef/lefiPropType.hpp
  lef/lefiUnits.hpp
  lef/lefiUser.hpp
  lef/lefiUtil.hpp
  lef/lefiVia.hpp
  lef/lefiViaRule.hpp
  lef/lefrCallBacks.hpp
  lef/lefrData.hpp
  lef/lefrReader.hpp
  lef/lefrSettings.hpp
  lef/lefwWriterCalls.hpp
  lef/lefwWriter.hpp

  lefzlib/lefzlib.hpp

  clef/lefiArray.h
  clef/lefiCrossTalk.h
  clef/lefiDebug.h
  clef/lefiEncryptInt.h
  clef/lefiLayer.h
  clef/lefiMacro.h
  clef/lefiMisc.h
  clef/lefiNonDefault.h
  clef/lefiProp.h
  clef/lefiPropType.h
  clef/lefiTypedefs.h
  clef/lefiUnits.h
  clef/lefiUser.h
  clef/lefiUtil.h
  clef/lefiVia.h
  clef/lefiViaRule.h
  clef/lefMsgTable.h
  clef/lefrReader.h
  clef/lefwWriterCalls.h
  clef/lefwWriter.h

  clefzlib/lefzlib.h
)


# BISON dependencies
add_custom_command( 
  OUTPUT ${LEFLIB_HOME}/lef/lef.tab.cpp
  COMMAND ${BISON_EXECUTABLE} -v -plefyy -d ${LEFLIB_HOME}/lef/lef.y 
  COMMAND mv lef.tab.h ${LEFLIB_HOME}/lef/lef.tab.h
  COMMAND mv lef.tab.c ${LEFLIB_HOME}/lef/lef.tab.cpp
)
add_custom_target(LefBisonTarget ALL DEPENDS lef/lef.tab.cpp)


add_library( lef STATIC ${LEFLIB_LEF_SRC} )
add_library( lefzlib STATIC ${LEFLIB_LEFZLIB_SRC} )

add_library( clef STATIC ${LEFLIB_CLEF_SRC} )
add_library( clefzlib STATIC ${LEFLIB_CLEFZLIB_SRC})


set_target_properties( lef
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
  LIBRARY_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
  RUNTIME_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
)

set_target_properties( lefzlib
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
  LIBRARY_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
  RUNTIME_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
)

set_target_properties( clef
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
  LIBRARY_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
  RUNTIME_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
)

set_target_properties( clefzlib 
  PROPERTIES 
  ARCHIVE_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
  LIBRARY_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
  RUNTIME_OUTPUT_DIRECTORY ${LEFLIB_HOME}/lib
  LINKER_LANGUAGE CXX 
)

# include directory settings
target_include_directories( lef
  PUBLIC
  lef/
  )

target_include_directories( lefzlib
  PUBLIC
  lef/
  lefzlib/
  )

target_include_directories( clef 
  PUBLIC
  lef/
  clef/
  )

target_include_directories( clefzlib
  PUBLIC
  lef/
  lefzlib/
  clef/
  clefzlib/
  )

add_dependencies( clef lef )
add_dependencies( lef LefBisonTarget )
add_dependencies( lefzlib lef )
add_dependencies( clefzlib clef )

######################################################
# install

install(FILES ${LEFLIB_HEADERS} DESTINATION include)
install(TARGETS lef lefzlib clef clefzlib DESTINATION lib)

