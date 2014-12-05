#
# CMakeLists.txt for pikaevent
#
message (STATUS "********* Starting pikaevent library *********")

# Disable 'unsecure' warnings in VC++

if (WIN32)
    if (MSVC)
        add_definitions(-D_SCL_SECURE_NO_WARNINGS)
        add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    endif (MSVC)
endif (WIN32)

set (ADD_LIBS "")

find_package(EVENT)
if(EVENT_FOUND)
    set (ADD_LIBS ${ADD_LIBS} ${EVENT_LIBRARIES})
    include_directories (${EVENT_INCLUDE_DIR})
    
    message(STATUS "Using event library ${EVENT_LIBRARIES}")
else(EVENT_FOUND)
    message(STATUS "Not builded event module.")
    return ()
endif(EVENT_FOUND)

# -------------------------------------------------------------------------
# Set version numbers
# -------------------------------------------------------------------------

set (CPACK_PACKAGE_VERSION_MAJOR ${pika_LIB_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${pika_LIB_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${pika_LIB_VERSION_PATCH})

set (pikaevent_LIB_VERSION "${pika_LIB_VERSION_MAJOR}.${pika_LIB_VERSION_MINOR}.${pika_LIB_VERSION_PATCH}")

# --------------------- Source + Header Files ----------------------
    
set (event_LIB_SRCS    PEventHelpers.cpp PEvent.cpp PEventBase.cpp PEventConfig.cpp PEventModule.cpp)
set (event_LIB_HEADERS PEventHelpers.h   PEvent.h   PEventBase.h   PEventConfig.h   PEventModule.h)
# set (event_TESTS       tests/test_event.pika)

source_group (include FILES ${event_LIB_HEADERS})
source_group (source  FILES ${event_LIB_SRCS})
    
include_directories (${Pika_SOURCE_DIR}/libpika)

link_directories (${Pika_BINARY_DIR}/libpika)
    
add_library (pikaevent SHARED ${event_LIB_SRCS} ${event_LIB_HEADERS})

set_target_properties (pikaevent PROPERTIES
                                 VERSION     ${pikaevent_LIB_VERSION}
                                 SOVERSION   ${pikaevent_LIB_VERSION})

target_link_libraries (pikaevent ${ADD_LIBS} pika)

if (APPLE)
    set_target_properties( 
        pikaevent 
        PROPERTIES 
        MACOSX_RPATH        TRUE
        INSTALL_RPATH       "${CMAKE_INSTALL_PREFIX}/lib"
        INSTALL_NAME_DIR    "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif (APPLE)

install (TARGETS        pikaevent
         RUNTIME        DESTINATION bin
         LIBRARY        DESTINATION lib/pika
         ARCHIVE        DESTINATION lib/pika         
         FRAMEWORK      DESTINATION Library/Frameworks)

# install(FILES ${event_TESTS} DESTINATION lib/pika/tests)