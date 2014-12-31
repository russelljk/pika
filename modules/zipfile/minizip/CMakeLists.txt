include_directories (
    ${PROJECT_BINARY_DIR}/modules/zlib/zlib ${Pika_SOURCE_DIR}/modules/zlib/zlib
    ${Pika_SOURCE_DIR}/modules/bzip2/bzip2
)
set (ADD_LIBS ${ADD_LIBS} zlib bz2)

set (minizip_SOURCE ioapi.c unzip.c zip.c)

if (WIN32)
    set (minizip_SOURCE ${minizip_SOURCE} iowin32.c)
endif (WIN32)

set (minizip_HEADERS crypt.h ioapi.h zip.h unzip.h)

if (WIN32)
    set (minizip_HEADERS ${minizip_HEADERS} iowin32.h)
endif (WIN32)

source_group (include FILES ${minizip_HEADERS})
source_group (source  FILES ${minizip_SOURCE})

add_library(minizip SHARED ${minizip_SOURCE} ${minizip_HEADERS})

target_link_libraries(minizip ${ADD_LIBS})

install(TARGETS minizip
    RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pika"
    LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pika" )

if (APPLE)
   set_target_properties( minizip 
               PROPERTIES 
               MACOSX_RPATH TRUE
               INSTALL_RPATH       "${CMAKE_INSTALL_PREFIX}/lib/pika"
               INSTALL_NAME_DIR    "${CMAKE_INSTALL_PREFIX}/lib/pika" )
endif (APPLE)
