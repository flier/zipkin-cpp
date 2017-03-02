# add any subdir with a cmake file
file(GLOB dirents RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
foreach(e ${dirents})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${e} AND
       EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${e}/CMakeLists.txt)
        add_subdirectory(${e})
    endif ()
endforeach ()