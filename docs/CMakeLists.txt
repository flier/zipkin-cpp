if (BUILD_DOCS)
    if (NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif()

    set(doxyfile ${PROJECT_SOURCE_DIR}/Doxyfile)

    file (
        GLOB_RECURSE doxygen_DEPS
        RELATIVE ${PROJECT_SOURCE_DIR}
        *.h *.hpp *.c *.cpp
    )

    add_custom_target(
        doc ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        DEPENDS ${doxyfile} ${doxygen_DEPS}
        VERBATIM
    )
endif ()