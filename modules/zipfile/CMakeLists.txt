message (STATUS "********* Starting pikazipfile library *********")

# Disable 'unsecure' warnings in VC++

if (WIN32)
    if (MSVC)
        add_definitions(-D_SCL_SECURE_NO_WARNINGS)
        add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    endif (MSVC)
endif (WIN32)
set (zipfile_LIB_HEADERS "")
set (ADD_LIBS "")

message (STATUS "zipfile: Using internal minizip.")
set (SKIP_INSTALL_FILES     TRUE)
set (SKIP_INSTALL_HEADERS   TRUE)
add_subdirectory (minizip)
include_directories (${PROJECT_SOURCE_DIR}/modules/zipfile/minizip)
set (ADD_LIBS ${ADD_LIBS} minizip)

set (CPACK_PACKAGE_VERSION_MAJOR ${pika_LIB_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${pika_LIB_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${pika_LIB_VERSION_PATCH})

set (pikazipfile_LIB_VERSION "${pika_LIB_VERSION_MAJOR}.${pika_LIB_VERSION_MINOR}.${pika_LIB_VERSION_PATCH}")

set (zipfile_LIB_SRCS    ${zipfile_LIB_SRCS}    PZipFile.cpp PZipReader.cpp)
set (zipfile_LIB_HEADERS ${zipfile_LIB_HEADERS} PZipFile.h   PZipReader.h)

source_group (include FILES ${zipfile_LIB_HEADERS})
source_group (source  FILES ${zipfile_LIB_SRCS})

include_directories (${Pika_SOURCE_DIR}/libpika)
link_directories (${Pika_BINARY_DIR}/libpika)

add_library (pikazipfile SHARED ${zipfile_LIB_SRCS} ${zipfile_LIB_HEADERS})
target_link_libraries (pikazipfile ${ADD_LIBS} pika)

set_target_properties ( pikazipfile 	PROPERTIES
                           	VERSION           ${pikazipfile_LIB_VERSION}
                           	SOVERSION         ${pikazipfile_LIB_VERSION})
if (APPLE)
   set_target_properties( pikazipfile 
               PROPERTIES 
               MACOSX_RPATH TRUE
               INSTALL_RPATH    "${CMAKE_INSTALL_PREFIX}/lib"
               INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endif (APPLE)

install (TARGETS       pikazipfile
        RUNTIME        DESTINATION bin
        LIBRARY        DESTINATION lib/pika
        ARCHIVE        DESTINATION lib/pika         
        FRAMEWORK      DESTINATION Library/Frameworks)
