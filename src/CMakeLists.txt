add_executable(pmacFilterControl pmacFilterControl.cpp)

# When building within the ARM container, set --dynamic-linker
if(EXISTS "/root/lib/ld-linux-armhf.so.3")
    # Use --disable-new-dtags to set RPATH instead of RUNPATH - see docs for more info
    target_link_options(pmacFilterControl PRIVATE -Wl,--disable-new-dtags,-rpath,/root/prefix/lib/:/root/libzmq/prefix/lib/:/root/lib)
    target_link_options(pmacFilterControl PRIVATE -Wl,-dynamic-linker,/root/lib/ld-linux-armhf.so.3)
    target_link_libraries(pmacFilterControl ${ZEROMQ_LIBRARIES} pthread ppmac math opener pthread_rt xenomai)
else()
    target_link_libraries(pmacFilterControl ${ZEROMQ_LIBRARIES} pthread)
endif()

install(TARGETS pmacFilterControl RUNTIME DESTINATION bin)
