find_package(Doxygen)

if(DOXYGEN_FOUND)
    message("Configuring Doxygen build")
    configure_File(${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(
        doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating documentation with Doxygen" VERBATIM
    )
endif(DOXYGEN_FOUND)
