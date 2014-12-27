message (STATUS "********* Starting Modules *********")

add_subdirectory(curses)
add_subdirectory(re)
add_subdirectory(base64)
add_subdirectory(bignum)
add_subdirectory(unittest)
add_subdirectory(json)
add_subdirectory(socket)
add_subdirectory(event)
add_subdirectory(datetime)
add_subdirectory(zlib)
add_subdirectory(bzip2)
add_subdirectory(zipfile) # Keep this after zlib and bzip2