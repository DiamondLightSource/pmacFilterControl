add_executable(pmacFilterController pmacFilterControl.cpp)

target_link_libraries(pmacFilterController ${ZEROMQ_LIBRARIES} pthread)  # pthread required here for ARM build
install(TARGETS pmacFilterController RUNTIME DESTINATION bin)
