#
# CMakeLists.txt for pika
#
message (STATUS "********* Starting pika library *********")


# Disable 'unsecure' warnings in VC++

if (WIN32)
    if (MSVC)
        add_definitions(-D_SCL_SECURE_NO_WARNINGS)
        add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    endif (MSVC)
endif (WIN32)

#------------------- Headers -------------------------

include (CheckIncludeFiles)

check_include_files (ctype.h     HAVE_CTYPE_H)
check_include_files (dlfcn.h     HAVE_DLFCN_H)
check_include_files (dl.h        HAVE_DL_H)
check_include_files (sys/dl.h    HAVE_SYS_DL_H)
check_include_files (errno.h     HAVE_ERRNO_H)
check_include_files (inttypes.h  HAVE_INTTYPES_H)
check_include_files (malloc.h    HAVE_MALLOC_H)
check_include_files (memory.h    HAVE_MEMORY_H)
check_include_files (stdint.h    HAVE_STDINT_H)
check_include_files (stdio.h     HAVE_STDIO_H)
check_include_files (stdlib.h    HAVE_STDLIB_H)
check_include_files (strings.h   HAVE_STRINGS_H)
check_include_files (string.h    HAVE_STRING_H)
check_include_files (sys/stat.h  HAVE_SYS_STAT_H)
check_include_files (sys/types.h HAVE_SYS_TYPES_H)
check_include_files (unistd.h    HAVE_UNISTD_H)

#------------------- Libraries -------------------------

include (CheckLibraryExists)

set (ADD_LIBS "")

if (NOT PIKA_NO_READLINE)
   find_path(READLINE_INCLUDE_DIR readline/readline.h)
   find_library(READLINE_LIBRARY NAMES readline) 
   
   if (READLINE_INCLUDE_DIR AND READLINE_LIBRARY)
      set(HAVE_READLINE TRUE)
      set (ADD_LIBS ${ADD_LIBS} ${READLINE_LIBRARY})
      include_directories(${READLINE_INCLUDE_DIR})
   endif (READLINE_INCLUDE_DIR AND READLINE_LIBRARY)
endif (NOT PIKA_NO_READLINE)

# link to dlopen if available
check_library_exists (dl dlopen "" HAVE_LIBDL)
if (HAVE_LIBDL)
    set (ADD_LIBS ${ADD_LIBS} dl)
endif (HAVE_LIBDL)

# in Unix we need to explicitly link to libm
if (UNIX)
    set (ADD_LIBS ${ADD_LIBS} m)
endif (UNIX)

#--------------------- Functions -----------------------

include (CheckFunctionExists)

if (NOT HAVE_LIBDL)
    check_function_exists ("dlopen;dlclose;dlsym" HAVE_LIBC_DL)
endif (NOT HAVE_LIBDL)

check_function_exists (bcopy    HAVE_BCOPY)
check_function_exists (bzero    HAVE_BZERO)
check_function_exists (memcpy   HAVE_MEMCPY)
check_function_exists (memset   HAVE_MEMSET)
check_function_exists (index    HAVE_INDEX)
check_function_exists (rindex   HAVE_RINDEX)
check_function_exists (strchr   HAVE_STRCHR)
check_function_exists (strcpy   HAVE_STRCPY)
check_function_exists (strrchr  HAVE_STRRCHR)
check_function_exists (strtok   HAVE_STRTOK)
check_function_exists (strtok_r HAVE_STRTOK_R)
check_function_exists (strtok_s HAVE_STRTOK_S)

# -------------------------------------------------------------------------
# Set version numbers
# -------------------------------------------------------------------------

set (CPACK_PACKAGE_VERSION_MAJOR ${pika_LIB_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${pika_LIB_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH ${pika_LIB_VERSION_PATCH})

set (pika_LIB_VERSION "${pika_LIB_VERSION_MAJOR}.${pika_LIB_VERSION_MINOR}.${pika_LIB_VERSION_PATCH}")

#--------------------- Generate pika_config.h -----------------------

configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/cmake-pika_config.h.in" 
                "${CMAKE_CURRENT_SOURCE_DIR}/pika_config.h")

#---------------------- Target Files --------------------------------

set (pika_LIB_SRCS PAnnotations.cpp PArray.cpp PAst.cpp PBasic.cpp PByteArray.cpp PClassInfo.cpp PCollector.cpp PCompiler.cpp PContext.cpp PDate.cpp PDictionary.cpp PDebugger.cpp PDef.cpp PEngine.cpp PError.cpp PFile.cpp PFunction.cpp PGenCode.cpp PGenerator.cpp PHooks.cpp Pika.cpp PImport.cpp PIterator.cpp PLiteralPool.cpp PLocalsObject.cpp PMemory.cpp PMemPool.cpp PModule.cpp PNativeBind.cpp PNativeMethod.cpp PObject.cpp POpcode.cpp PPackage.cpp PParser.cpp PPlatform.cpp PPathManager.cpp PProperty.cpp PProxy.cpp PRandom.cpp PScript.cpp PString.cpp PStringTable.cpp PSymbolTable.cpp PSystemLib.cpp PTable.cpp PTokenizer.cpp PType.cpp PUserData.cpp PValue.cpp PWorld.cpp)

set (pika_LIB_HEADERS pika_config.h PArray.h PAst.h PBasic.h PBuffer.h PByteArray.h PByteOrder.h PClassInfo.h PCollector.h PCompiler.h PConfig.h PConfig_Borland.h PConfig_GCC.h PConfig_VisualStudio.h PContext.h PContext_Ops.inl PContext_Ops_Arith.inl PContext_Ops_Call.inl PContext_Ops_Std.inl PContext_Run.inl PDate.h PDictionary.h PDebugger.h PDef.h PEngine.h PError.h PFile.h PFunction.h PGenerator.h PHooks.h Pika.h PikaSort.h PInstruction.h PIterator.h PLineInfo.h PLiteralPool.h PLocalsObject.h PMemory.h PMemPool.h PModule.h PNativeBind.h PNativeConstMethodDecls.h PNativeMethod.h PNativeMethodDecls.h PNativeStaticMethodDecls.h PObject.h PObjectIterator.h POpcodeDef.inl POpcode.h PPackage.h PParser.h PPlatform.h PPathManager.h PProfiler.h PProperty.h PProxy.h PRandom.h PScript.h PString.h PStringTable.h PSymbolTable.h PTable.h PTokenDef.inl PTokenDef.h PTokenizer.h PType.h PUserData.h PUtil.h PValue.h)

#------------------------------------------------------------------
# Convert header list into comma seperated list. "a b c" -> "a;b;c"
#------------------------------------------------------------------

set (pika_PUB_HEADERS "")
foreach (header ${pika_LIB_HEADERS})
    if (pika_PUB_HEADERS STREQUAL "")
        set (pika_PUB_HEADERS "${header}")
    else (pika_PUB_HEADERS STREQUAL "")
        # the slash before the semi colon is required other wise it seems to be ignored.
        set (pika_PUB_HEADERS "${pika_PUB_HEADERS}\;${header}")
    endif (pika_PUB_HEADERS STREQUAL "")    
endforeach(header)

if (WIN32)
    set (pika_LIB_SRCS ${pika_LIB_SRCS} PPlatformWin32.cpp)
endif (WIN32)

if (UNIX)
    set (pika_LIB_SRCS ${pika_LIB_SRCS} PPlatformPosix.cpp)
endif (UNIX)

source_group (include FILES ${pika_LIB_HEADERS})
source_group (source  FILES ${pika_LIB_SRCS})

add_library (pika SHARED ${pika_LIB_SRCS} ${pika_LIB_HEADERS})
if (WIN32)
    add_definitions(-DPIKA_DLL)
    add_definitions(-DPIKA_EXPORTS)
endif (WIN32)
#set_property (SOURCE ${pika_LIB_HEADERS} PROPERTY MACOSX_PACKAGE_LOCATION Headers)

target_link_libraries (pika ${ADD_LIBS})

# -------------------------------------------------------------------------
# Change list format from "a;b;c" to "-la -lb -lc" for the pkg-config file.
# -------------------------------------------------------------------------

set (PKG_CONFIG_ADD_LIBS "")
foreach (lib ${ADD_LIBS})
    # The if statement keeps PKG_CONFIG_ADD_LIBS from starting with whitespace.
    # It probably doesn't matter but it keeps the output looking nice.

    if (PKG_CONFIG_ADD_LIBS STREQUAL "")
    set (PKG_CONFIG_ADD_LIBS "-l${lib}")
    else (PKG_CONFIG_ADD_LIBS STREQUAL "")
    set (PKG_CONFIG_ADD_LIBS "${PKG_CONFIG_ADD_LIBS} -l${lib}")
    endif (PKG_CONFIG_ADD_LIBS STREQUAL "")
endforeach (lib)

#---

if (PIKA_USE_FRAMEWORKS)
    set (pika_FRAMEWORKS "TRUE")
else (PIKA_USE_FRAMEWORKS)
    set (pika_FRAMEWORKS "FALSE")
endif (PIKA_USE_FRAMEWORKS)

set (pika_FRAME_VERSION ${pika_LIB_VERSION})
set_target_properties (pika PROPERTIES
                            VERSION           ${pika_LIB_VERSION}
                            SOVERSION         ${pika_LIB_VERSION}
                            FRAMEWORK         ${pika_FRAMEWORKS}
                            FRAMEWORK_VERSION ${pika_FRAME_VERSION}
                            PUBLIC_HEADER     ${pika_PUB_HEADERS})

if (PIKA_USE_FRAMEWORKS)
	set_target_properties (pika PROPERTIES
			       INSTALL_NAME_DIR @executable_path/../Frameworks)
endif (PIKA_USE_FRAMEWORKS)

#@loader_path
#--------------------- Install  -----------------------

set (PIKA_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set (PIKA_INCLUDE_DIR    "include/Pika")

#install (FILES ${pika_LIB_HEADERS} 
#        DESTINATION ${PIKA_INCLUDE_DIR})

install (TARGETS pika
         RUNTIME        DESTINATION bin
         LIBRARY        DESTINATION lib
         ARCHIVE        DESTINATION lib         
         FRAMEWORK      DESTINATION Library/Frameworks
         # For OSX Framework build.
         PUBLIC_HEADER  DESTINATION ${PIKA_INCLUDE_DIR}
         PRIVATE_HEADER DESTINATION ${PIKA_INCLUDE_DIR}/PrivateHeaders)

if (APPLE)
    set_target_properties( pika 
                PROPERTIES 
                MACOSX_RPATH TRUE
                INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
                INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endif (APPLE)

#--------------------- Generate + Install pika.pc ------------------------------
# We don't mess with the package configuration file if we are making a framework
# or its windows.
#-------------------------------------------------------------------------------

if (NOT PIKA_USE_FRAMEWORKS OR WIN32)
         configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/cmake-pika.pc.in" 
                         "${CMAKE_CURRENT_BINARY_DIR}/pc/pika.pc" 
                         @ONLY)
         
         install (FILES       "${CMAKE_CURRENT_BINARY_DIR}/pc/pika.pc"
                  DESTINATION "lib/pkgconfig"
                  COMPONENT   "pkgconfig")
endif (NOT PIKA_USE_FRAMEWORKS OR WIN32)

include(CPack)

SET(CPACK_NSIS_MODIFY_PATH ON)


