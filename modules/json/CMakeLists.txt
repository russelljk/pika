#
# CMakeLists.txt for pikajson
#
message (STATUS "********* Starting pikajson library *********")

# Disable 'unsecure' warnings in VC++

if (WIN32)
    if (MSVC)
        add_definitions(-D_SCL_SECURE_NO_WARNINGS)
        add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    endif (MSVC)
endif (WIN32)

# -------------------- Look for regexp libs -----------------------

set (ADD_LIBS "")

# -------------------------------------------------------------------------
# Set version numbers
# -------------------------------------------------------------------------

set (CPACK_PACKAGE_VERSION_MAJOR ${pika_LIB_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${pika_LIB_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${pika_LIB_VERSION_PATCH})

set (pikajson_LIB_VERSION "${pika_LIB_VERSION_MAJOR}.${pika_LIB_VERSION_MINOR}.${pika_LIB_VERSION_PATCH}")

# --------------------- Source + Header Files ----------------------
    
set (json_LIB_SRCS    PJsonEncode.cpp PJsonDecode.cpp PJson.cpp)
set (json_LIB_HEADERS PJsonEncode.h PJsonDecode.h PJson.h)
set (json_TESTS       tests/test_json.pika)

source_group (include FILES ${json_LIB_HEADERS})
source_group (source  FILES ${json_LIB_SRCS})
    
include_directories (${Pika_SOURCE_DIR}/libpika)

link_directories (${Pika_BINARY_DIR}/libpika)
    
add_library (pikajson SHARED ${json_LIB_SRCS} ${json_LIB_HEADERS})

set_target_properties (pikajson PROPERTIES
                                VERSION     ${pikajson_LIB_VERSION}
                                SOVERSION   ${pikajson_LIB_VERSION})

target_link_libraries (pikajson ${ADD_LIBS} pika)

if (APPLE)
    set_target_properties( 
        pikajson 
        PROPERTIES 
        MACOSX_RPATH        TRUE
        INSTALL_RPATH       "${CMAKE_INSTALL_PREFIX}/lib"
        INSTALL_NAME_DIR    "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif (APPLE)

install (TARGETS        pikajson
         RUNTIME        DESTINATION bin
         LIBRARY        DESTINATION lib/pika
         ARCHIVE        DESTINATION lib/pika         
         FRAMEWORK      DESTINATION Library/Frameworks)

install(FILES ${json_TESTS} DESTINATION lib/pika/tests)