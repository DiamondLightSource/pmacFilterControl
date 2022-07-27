add_executable(pmacFilterControl pmacFilterControl.cpp)

# When building within the ARM container, set --dynamic-linker
if(EXISTS "/root/lib/ld-2.31.so")
    target_link_options(pmacFilterControl PRIVATE -Wl,--dynamic-linker,/root/lib/ld-2.31.so)
    target_link_libraries(pmacFilterControl ${ZEROMQ_LIBRARIES} pthread ppmac math opener pthread_rt xenomai)
else()
    target_link_libraries(pmacFilterControl ${ZEROMQ_LIBRARIES})
endif()

install(TARGETS pmacFilterControl RUNTIME DESTINATION bin)
