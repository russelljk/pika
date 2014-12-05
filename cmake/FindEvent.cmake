if (EVENT_INCLUDE_DIR AND EVENT_LIBRARIES)
  # Already in cache, be silent
  set(EVENT_FIND_QUIETLY TRUE)
endif (EVENT_INCLUDE_DIR AND EVENT_LIBRARIES)

find_path(EVENT_INCLUDE_DIR NAMES event2/event.h )
find_library(EVENT_LIBRARIES NAMES event libevent )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EVENT DEFAULT_MSG EVENT_INCLUDE_DIR EVENT_LIBRARIES)

mark_as_advanced(EVENT_INCLUDE_DIR EVENT_LIBRARIES)