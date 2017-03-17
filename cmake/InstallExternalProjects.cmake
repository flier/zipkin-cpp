include(ExternalProject)

set (EXTERNAL_PROJECT_DIR ${CMAKE_BINARY_DIR}/externals)
set_directory_properties(PROPERTIES EP_PREFIX ${EXTERNAL_PROJECT_DIR})

list(APPEND CMAKE_MODULE_PATH ${EXTERNAL_PROJECT_DIR}/lib/cmake)

message(STATUS "Install external dependencies to ${EXTERNAL_PROJECT_DIR}")

include_directories(BEFORE SYSTEM ${EXTERNAL_PROJECT_DIR}/include)
link_directories(${EXTERNAL_PROJECT_DIR}/lib)

macro(install_external_project project_name)
    include(Install${project_name})

    if (TARGET ${project_name})
        if ($ENV{TRAVIS})
            ExternalProject_Add_Step(${project_name} ${project_name}_download
                COMMAND     echo "\\ntravis_fold:start:${project_name}.download\tDownload ${project_name}"
                DEPENDERS   download
            )
            ExternalProject_Add_Step(${project_name} ${project_name}_configure
                COMMAND     echo "\\ntravis_fold:end:${project_name}.download"
                COMMAND     echo "\\ntravis_fold:start:${project_name}.configure\tConfigure ${project_name}"
                DEPENDEES   download
                DEPENDERS   configure
            )

            ExternalProject_Add_Step(${project_name} ${project_name}_build
                COMMAND     echo "\\ntravis_fold:end:${project_name}.configure"
                COMMAND     echo "\\ntravis_fold:start:${project_name}.build\tBuild ${project_name}"
                DEPENDEES   configure
                DEPENDERS   build
            )

            ExternalProject_Add_Step(${project_name} ${project_name}_install
                COMMAND     echo "\\ntravis_fold:end:${project_name}.build"
                COMMAND     echo "\\ntravis_fold:start:${project_name}.install\tInstall ${project_name}"
                DEPENDEES   build
                DEPENDERS   install
            )

            ExternalProject_Add_Step(${project_name} ${project_name}_test
                COMMAND     echo "\\ntravis_fold:end:${project_name}.install"
                COMMAND     echo "\\ntravis_fold:start:${project_name}.test\tTest ${project_name}"
                DEPENDEES   install
                DEPENDERS   test
            )
            ExternalProject_Add_Step(${project_name} ${project_name}_end
                COMMAND     echo "\\ntravis_fold:end:${project_name}.test"
                DEPENDEES   test
            )
        endif ()
    endif ()
endmacro ()

install_external_project(ZLib)
install_external_project(CURL)
install_external_project(RapidJSON)
install_external_project(LibRDKafka)
install_external_project(DoubleConversion)
install_external_project(GFlags)
install_external_project(GLog)
install_external_project(GTest)
install_external_project(GBench)
install_external_project(Folly)
install_external_project(Thrift)
