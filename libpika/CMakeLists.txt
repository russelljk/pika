
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

set (ADD_LIBS "")

include (CheckLibraryExists)

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

#--------------------- Generate config.h -----------------------

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake-pika_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/pika_config.h)

#---------------------- Target Files --------------------------

set (pika_LIB_SRCS PArray.cpp PAst.cpp PBasic.cpp PByteArray.cpp PClassInfo.cpp PCollector.cpp PCompiler.cpp PContext.cpp PDebugger.cpp PDef.cpp PEngine.cpp PEnumerator.cpp PError.cpp PFile.cpp PFunction.cpp PGenCode.cpp PHooks.cpp Pika.cpp PImport.cpp PLiteralPool.cpp PLocalsObject.cpp PMemory.cpp PMemPool.cpp PModule.cpp PNativeBind.cpp PNativeMethod.cpp PObject.cpp POpcode.cpp PPackage.cpp PParser.cpp PPlatform.cpp PProperty.cpp PRandom.cpp PScript.cpp PString.cpp PStringTable.cpp PSymbolTable.cpp PSystemLib.cpp PTable.cpp PTokenizer.cpp PType.cpp PUserData.cpp PValue.cpp PValueEnumerator.cpp PWorld.cpp)

set (pika_LIB_HEADERS pika_config.h PArray.h PAst.h PBasic.h PBuffer.h PByteArray.h PByteOrder.h PClassInfo.h PCollector.h PCompiler.h PConfig.h PContext.h PContext_Ops.inl PContext_Ops_Arith.inl PContext_Ops_Call.inl PContext_Ops_Std.inl PContext_Run.inl PDebugger.h PDef.h PEngine.h PEnumerator.h PError.h PFile.h PFunction.h PHooks.h Pika.h PikaSort.h PInstruction.h PLineInfo.h PLiteralPool.h PLocalsObject.h PMemory.h PMemPool.h PModule.h PNativeBind.h PNativeConstMethodDecls.h PNativeMethod.h PNativeMethodDecls.h PNativeStaticMethodDecls.h PObject.h PObjectEnumerator.h POpcode.def POpcode.h PPackage.h PParser.h PPlatform.h PProfiler.h PProperty.h PRandom.h PScript.h PString.h PStringTable.h PSymbolTable.h PTable.h PToken.def PTokenDef.h PTokenizer.h PType.h PUserData.h PUtil.h PValue.h PValueEnumerator.h)

if (WIN32)
    set (pika_LIB_SRCS ${pika_LIB_SRCS} PPlatformWin32.cpp)
endif (WIN32)

if (UNIX)
    set (pika_LIB_SRCS ${pika_LIB_SRCS} PPlatformPosix.cpp)
endif (UNIX)

source_group (include FILES ${pika_LIB_HEADERS})
source_group (source  FILES ${pika_LIB_SRCS})

add_library (pika SHARED ${pika_LIB_SRCS} ${pika_LIB_HEADERS})

target_link_libraries (pika ${ADD_LIBS})

set (pika_LIB_VERSION_MAJOR "0")
set (pika_LIB_VERSION_MINOR "1")

set (pika_LIB_VERSION "${pika_LIB_VERSION_MAJOR}.${pika_LIB_VERSION_MINOR}")

# what about SOVERSION
set_target_properties(pika PROPERTIES VERSION ${pika_LIB_VERSION}
                                      SOVERSION ${pika_LIB_VERSION})

install(FILES ${pika_LIB_HEADERS} DESTINATION include/Pika)

install (TARGETS pika
	 LIBRARY DESTINATION lib)
