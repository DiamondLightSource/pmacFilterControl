add_executable(pmacFilterControl pmacFilterControl.cpp)

# When building within the ARM container, set --dynamic-linker
if(EXISTS "/root/lib/ld-2.31.so")
    target_link_options(pmacFilterControl PRIVATE -Wl,--dynamic-linker,/root/lib/ld-2.31.so)
endif()

# pthread required here for ARM build
target_link_libraries(pmacFilterControl ${ZEROMQ_LIBRARIES} pthread)
install(TARGETS pmacFilterControl RUNTIME DESTINATION bin)
