# Substitute current git version via version.h
execute_process(COMMAND git describe --dirty --tags --long COMMAND tr --delete '\n' OUTPUT_VARIABLE GIT_VERSION)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in "${CMAKE_CURRENT_BINARY_DIR}/version.h")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(pmacFilterControl pmacFilterControl.cpp ${CMAKE_CURRENT_BINARY_DIR}/version.h)

# When building within the ARM container, set --dynamic-linker
if(ARM)
    # Use --disable-new-dtags to set RPATH instead of RUNPATH - see docs for more info
    target_link_options(pmacFilterControl PRIVATE -Wl,--disable-new-dtags,-rpath,/root/prefix/lib/:/root/libzmq/prefix/lib/:/root/lib)
    target_link_options(pmacFilterControl PRIVATE -Wl,-dynamic-linker,/root/lib/ld-linux-armhf.so.3)
    target_link_libraries(pmacFilterControl ${ZEROMQ_LIBRARIES} pthread ppmac math opener pthread_rt xenomai)
else()
    target_link_libraries(pmacFilterControl ${ZEROMQ_LIBRARIES} pthread)
endif()

install(TARGETS pmacFilterControl RUNTIME DESTINATION bin)
