message (STATUS "********* Starting pikazlib library *********")

# Disable 'unsecure' warnings in VC++

if (WIN32)
    if (MSVC)
        add_definitions(-D_SCL_SECURE_NO_WARNINGS)
        add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    endif (MSVC)
endif (WIN32)
set (zlib_LIB_HEADERS "")
set (ADD_LIBS "")

if (EXTERNAL_ZLIB)
    include (FindZlib)

    set (zlib_LIB_SRCS "")
    set (zlib_LIB_HEADERS "")
    set (ADD_LIBS "")

    if (ZLIB_FOUND)
        message (STATUS "zlib: Using external zlib.")
        set (zlib_LIB_HEADERS ${zlib_LIB_HEADERS} ${ZLIB_INCLUDE_DIRS})
        set (ADD_LIBS ${ADD_LIBS} ${ZLIB_LIBRARIES})
    endif (ZLIB_FOUND)
endif (EXTERNAL_ZLIB)

if (NOT ZLIB_FOUND)
    message (STATUS "zlib: Using internal zlib.")
    set (SKIP_INSTALL_FILES     TRUE)
    set (SKIP_INSTALL_HEADERS   TRUE)
    add_subdirectory (zlib)
    include_directories (${PROJECT_BINARY_DIR})
    set (ADD_LIBS ${ADD_LIBS} zlib)
endif (NOT ZLIB_FOUND)

set (CPACK_PACKAGE_VERSION_MAJOR ${pika_LIB_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${pika_LIB_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${pika_LIB_VERSION_PATCH})

set (pikazlib_LIB_VERSION "${pika_LIB_VERSION_MAJOR}.${pika_LIB_VERSION_MINOR}.${pika_LIB_VERSION_PATCH}")

# configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake-zlib_config.h.in
#                ${CMAKE_CURRENT_SOURCE_DIR}/zlib_config.h)

set (zlib_LIB_SRCS ${zlib_LIB_SRCS} PZlib.cpp PZError.cpp PZStream.cpp PZDecompressor.cpp PZCompressor.cpp)
set (zlib_LIB_HEADERS ${zlib_LIB_HEADERS} PZlib.h PZError.h PZStream.h PZDecompressor.h PZCompressor.h)

source_group (include FILES ${zlib_LIB_HEADERS})
source_group (source  FILES ${zlib_LIB_SRCS})

include_directories (${Pika_SOURCE_DIR}/libpika)
link_directories (${Pika_BINARY_DIR}/libpika)

add_library (pikazlib SHARED ${zlib_LIB_SRCS} ${zlib_LIB_HEADERS})
target_link_libraries (pikazlib ${ADD_LIBS} pika)

set_target_properties ( pikazlib 	PROPERTIES
                           	VERSION           ${pikazlib_LIB_VERSION}
                           	SOVERSION         ${pikazlib_LIB_VERSION})
if (APPLE)
   set_target_properties( pikazlib 
               PROPERTIES 
               MACOSX_RPATH TRUE
               INSTALL_RPATH    "${CMAKE_INSTALL_PREFIX}/lib"
               INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endif (APPLE)

install (TARGETS       pikazlib
        RUNTIME        DESTINATION bin
        LIBRARY        DESTINATION lib/pika
        ARCHIVE        DESTINATION lib/pika         
        FRAMEWORK      DESTINATION Library/Frameworks)
