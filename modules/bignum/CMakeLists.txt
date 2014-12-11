#
# CMakeLists.txt for pikabignum
#
message (STATUS "********* Starting pikabignum library *********")

# Disable 'unsecure' warnings in VC++

if (WIN32)
    if (MSVC)
        add_definitions(-D_SCL_SECURE_NO_WARNINGS)
        add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    endif (MSVC)
endif (WIN32)

# -------------------- Look for needed libs -----------------------

set (ADD_LIBS "")

find_package(GMP)
if(GMP_FOUND)
    set (ADD_LIBS ${ADD_LIBS} ${GMP_LIBRARIES})
    include_directories (${GMP_INCLUDE_DIR})
    
    message(STATUS "Using gmp library ${GMP_LIBRARIES}")
else(GMP_FOUND)
    return ()
endif(GMP_FOUND)

# --------------------- Source + Header Files ----------------------



find_package(MPFR)
if(MPFR_FOUND)
    set (ADD_LIBS ${ADD_LIBS} ${MPFR_LIBRARIES})
    include_directories (${MPFR_INCLUDE_DIR})
    
    set (bignum_LIB_SRCS    ${bignum_LIB_SRCS}    PBigReal.cpp)
    set (bignum_LIB_HEADERS ${bignum_LIB_HEADERS} PBigReal.h)
    set (bignum_TESTS       ${bignum_TESTS}       tests/test_bigreal.pika)
    set (HAVE_MPFR 1)
    message(STATUS "Using mpfr library ${MPFR_LIBRARIES}")
else(MPFR_FOUND)
    message(WARNING "Building bignum module without floating point support. Install mpfr and rerun cmake to enable it.")
endif(MPFR_FOUND)

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/cmake-bignum_config.h.in 
                ${CMAKE_CURRENT_SOURCE_DIR}/bignum_config.h)

set (bignum_LIB_SRCS    ${bignum_LIB_SRCS}    PBigNum.cpp PBigInteger.cpp)
set (bignum_LIB_HEADERS ${bignum_LIB_HEADERS} PBigNum.h PBigInteger.h bignum_config.h)
set (bignum_TESTS       ${bignum_TESTS}       tests/test_biginteger.pika)

source_group (include FILES ${bignum_LIB_HEADERS})
source_group (source  FILES ${bignum_LIB_SRCS})

# -------------------------------------------------------------------------
# Set version numbers
# -------------------------------------------------------------------------

set (CPACK_PACKAGE_VERSION_MAJOR ${pika_LIB_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${pika_LIB_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${pika_LIB_VERSION_PATCH})

set (pikabignum_LIB_VERSION "${pika_LIB_VERSION_MAJOR}.${pika_LIB_VERSION_MINOR}.${pika_LIB_VERSION_PATCH}")

include_directories (${Pika_SOURCE_DIR}/libpika)

link_directories (${Pika_BINARY_DIR}/libpika)

add_library (pikabignum SHARED ${bignum_LIB_SRCS} ${bignum_LIB_HEADERS})
target_link_libraries (pikabignum ${ADD_LIBS} pika)

set_target_properties ( pikabignum  PROPERTIES
                        VERSION     ${pikabignum_LIB_VERSION}
                        SOVERSION   ${pikabignum_LIB_VERSION})

if (APPLE)
    set_target_properties( pikabignum       PROPERTIES 
                           MACOSX_RPATH     TRUE
                           INSTALL_RPATH    "${CMAKE_INSTALL_PREFIX}/lib"
                           INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endif (APPLE)

install (TARGETS        pikabignum
         RUNTIME        DESTINATION bin
         LIBRARY        DESTINATION lib/pika
         ARCHIVE        DESTINATION lib/pika         
         FRAMEWORK      DESTINATION Library/Frameworks)

install(FILES ${bignum_TESTS} DESTINATION lib/pika/tests)
