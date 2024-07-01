cmake_minimum_required(VERSION 3.15)
project(ldp_in)

# define library version
set(LDP_IN_VERSION_MAJOR 1 CACHE STRING "major version" FORCE)
set(LDP_IN_VERSION_MINOR 0 CACHE STRING "minor version" FORCE)
set(LDP_IN_VERSION ${LDP_IN_VERSION_MAJOR}.${LDP_IN_VERSION_MINOR} CACHE STRING "version" FORCE)

# from https://foonathan.net/2016/03/cmake-install/
set(include_dest "include/ldp_in-${LDP_IN_VERSION}")
set(main_lib_dest "lib/ldp_in-${LDP_IN_VERSION}")

# for config.h file since it may be different depending on whether we are doing Debug/Release/etc
set(lib_dest "${main_lib_dest}/${CMAKE_BUILD_TYPE}")

# don't look back
set(CMAKE_CXX_STANDARD 17)

# so that gmock doesn't get a different runtime than the rest of the system when using Visual Studio
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(INSTALL_GTEST "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" OFF)
add_subdirectory("thirdparty/googletest-1.14.0")

add_subdirectory("src")
add_subdirectory("tests")

configure_file(ldp_in-config-version.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/ldp_in-config-version.cmake @ONLY)

install(FILES ldp_in-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/ldp_in-config-version.cmake DESTINATION ${main_lib_dest})
install(EXPORT ldp_in FILE ldp_in.cmake DESTINATION "${lib_dest}")
