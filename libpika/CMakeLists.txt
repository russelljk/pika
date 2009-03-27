INCLUDE (CheckIncludeFiles)

CHECK_INCLUDE_FILES (ctype.h HAVE_CTYPE_H)
CHECK_INCLUDE_FILES (dld.h HAVE_DLD_H)
CHECK_INCLUDE_FILES (dlfcn.h HAVE_DLFCN_H)

# CHECK_INCLUDE_FILES ("sys/param.h;sys/mount.h" HAVE_SYS_MOUNT_H)
# CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

ADD_LIBRARY(libpika SHARED PArray.cpp PAst.cpp PBasic.cpp PByteArray.cpp PClassInfo.cpp PCollector.cpp PCompiler.cpp PContext.cpp PDebugger.cpp PDef.cpp PEngine.cpp PEnumerator.cpp PError.cpp PFile.cpp PFunction.cpp PGenCode.cpp PHooks.cpp Pika.cpp PImport.cpp PLiteralPool.cpp PLocalsObject.cpp PMemory.cpp PMemPool.cpp PModule.cpp PNativeBind.cpp PNativeMethod.cpp PObject.cpp POpcode.cpp PPackage.cpp PParser.cpp PPlatform.cpp PPlatformPosix.cpp PPlatformWin32.cpp PProperty.cpp PRandom.cpp PScript.cpp PString.cpp PStringTable.cpp PSymbolTable.cpp PSystemLib.cpp PTable.cpp PTokenizer.cpp PType.cpp PUserData.cpp PValue.cpp PValueEnumerator.cpp PWorld.cpp)
