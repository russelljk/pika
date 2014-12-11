message (STATUS "********* Starting datetime library *********")

set (datetime_TESTS         tests/test_datetime.pika)
set (datetime_FILES         datetime.pika)

install(FILES ${datetime_FILES} DESTINATION lib/pika)
install(FILES ${datetime_TESTS} DESTINATION lib/pika/tests)