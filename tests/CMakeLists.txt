if(NOT ARM)
    add_executable(TestZMQSender zmqSender.c)
    target_link_libraries(TestZMQSender ${ZEROMQ_LIBRARIES})
    install(TARGETS TestZMQSender RUNTIME DESTINATION bin)

    add_executable(TestZMQPublisher zmqPublisher.c)
    target_link_libraries(TestZMQPublisher ${ZEROMQ_LIBRARIES})
    install(TARGETS TestZMQPublisher RUNTIME DESTINATION bin)
endif()
