cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(cliop CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(cliop 
    src/cliop.cpp
    src/util-string.cpp
)

option(ENABLE_TEST "build test" ON)
if(ENABLE_TEST)
list(APPEND CMAKE_PREFIX_PATH ${HOME})
find_package(couttast)
add_executable(utest-cliop
    utest/test-main.cpp
    utest/test-cliop.cpp
    utest/test-error.cpp
    utest/test-string.cpp
    utest/test-subcmd.cpp
)
target_link_libraries(utest-cliop
    PRIVATE cliop
#    PRIVATE couttast
)
target_include_directories(utest-cliop
    PRIVATE ${PROJECT_SOURCE_DIR}/src
    PRIVATE ${COUTTAST_INCLUDE_DIRS}/couttast
)

enable_testing()
add_test(NAME utest-clop COMMAND utest-cliop
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/utest
)
endif()

set(CMAKE_INSTALL_PREFIX ${HOME})
install(FILES src/cliop.h
    DESTINATION "include/cliop"
)

install(TARGETS cliop
    DESTINATION "lib"
)
