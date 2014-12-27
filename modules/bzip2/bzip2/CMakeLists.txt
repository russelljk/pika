
ADD_DEFINITIONS ( -D_FILE_OFFSET_BITS=64 )

SET ( BZIP2_SRCS blocksort.c huffman.c crctable.c randtable.c compress.c decompress.c bzlib.c )
ADD_LIBRARY ( bz2 ${BZIP2_SRCS} libbz2.def )

set(INSTALL_BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin"       CACHE PATH "Installation directory for executables")
set(INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib"       CACHE PATH "Installation directory for libraries")
set(INSTALL_INC_DIR "${CMAKE_INSTALL_PREFIX}/include"   CACHE PATH "Installation directory for headers")

if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL )
    install(TARGETS bz2
        RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
        ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
        LIBRARY DESTINATION "${INSTALL_LIB_DIR}")
    
    if (APPLE)
       set_target_properties(bz2 
                   PROPERTIES 
                   MACOSX_RPATH TRUE
                   INSTALL_RPATH    "${INSTALL_LIB_DIR}"
                   INSTALL_NAME_DIR "${INSTALL_LIB_DIR}")
    endif (APPLE)
endif()