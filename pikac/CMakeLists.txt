
include_directories (${Pika_SOURCE_DIR}/libpika)
link_directories (${Pika_BINARY_DIR}/libpika)

add_executable(pika Main.cpp)

target_link_libraries(pika libpika) 