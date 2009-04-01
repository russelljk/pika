
include_directories (${Pika_SOURCE_DIR}/libpika)
link_directories (${Pika_BINARY_DIR}/libpika)

add_executable(pikac Main.cpp)

target_link_libraries(pikac pika)
set_target_properties (pikac PROPERTIES OUTPUT_NAME pika)

INSTALL(TARGETS pikac RUNTIME DESTINATION bin)
