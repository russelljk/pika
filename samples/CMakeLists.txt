file(GLOB SAMPLE_FILES *.pika)
message(status ${SAMPLE_FILES})
install(FILES ${SAMPLE_FILES} DESTINATION lib/pika/samples)