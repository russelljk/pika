message (STATUS "********* Starting pikabzip2 library *********")

# Disable 'unsecure' warnings in VC++

if (WIN32)
    if (MSVC)
        add_definitions(-D_SCL_SECURE_NO_WARNINGS)
        add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    endif (MSVC)
endif (WIN32)
set (bzip2_LIB_HEADERS "")
set (ADD_LIBS "")

message (STATUS "bzip2: Using internal bz2.")
set (SKIP_INSTALL_FILES     TRUE)
set (SKIP_INSTALL_HEADERS   TRUE)
add_subdirectory (bzip2)
include_directories (${Pika_SOURCE_DIR}/modules/zlib/bzip2)
set (ADD_LIBS ${ADD_LIBS} bz2)

set (CPACK_PACKAGE_VERSION_MAJOR ${pika_LIB_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${pika_LIB_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${pika_LIB_VERSION_PATCH})

set (pikabzip2_LIB_VERSION "${pika_LIB_VERSION_MAJOR}.${pika_LIB_VERSION_MINOR}.${pika_LIB_VERSION_PATCH}")

set (bzip2_LIB_SRCS ${bzip2_LIB_SRCS} BZip2.cpp BZError.cpp BZStream.cpp BZDecompressor.cpp BZCompressor.cpp)
set (bzip2_LIB_HEADERS ${bzip2_LIB_HEADERS} BZip2.h BZError.h BZStream.h BZDecompressor.h BZCompressor.h)

source_group (include FILES ${bzip2_LIB_HEADERS})
source_group (source  FILES ${bzip2_LIB_SRCS})

include_directories (${Pika_SOURCE_DIR}/libpika)
link_directories (${Pika_BINARY_DIR}/libpika)

add_library (pikabzip2 SHARED ${bzip2_LIB_SRCS} ${bzip2_LIB_HEADERS})
target_link_libraries (pikabzip2 ${ADD_LIBS} pika)

set_target_properties ( pikabzip2 	PROPERTIES
                           	VERSION           ${pikabzip2_LIB_VERSION}
                           	SOVERSION         ${pikabzip2_LIB_VERSION})
if (APPLE)
   set_target_properties( pikabzip2 
               PROPERTIES 
               MACOSX_RPATH TRUE
               INSTALL_RPATH    "${CMAKE_INSTALL_PREFIX}/lib"
               INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endif (APPLE)

install (TARGETS       pikabzip2
        RUNTIME        DESTINATION bin
        LIBRARY        DESTINATION lib/pika
        ARCHIVE        DESTINATION lib/pika         
        FRAMEWORK      DESTINATION Library/Frameworks)
