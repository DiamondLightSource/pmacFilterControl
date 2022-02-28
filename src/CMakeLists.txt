add_executable(pmacFilterControl pmacFilterControl.cpp)

target_link_libraries(pmacFilterControl ${ZEROMQ_LIBRARIES} pthread)  # pthread required here for ARM build
install(TARGETS pmacFilterControl RUNTIME DESTINATION bin)
