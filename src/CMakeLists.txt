include_directories(${ZEROMQ_INCLUDE_DIRS})

add_executable(pmacFilterController pmacFilterControl.cpp)
target_link_libraries(pmacFilterController ${ZEROMQ_LIBRARIES})
install(TARGETS pmacFilterController RUNTIME DESTINATION bin)
