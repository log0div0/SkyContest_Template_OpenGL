cmake_minimum_required(VERSION 3.15)

project(sky_contest)

set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()

if(MSVC)
	set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}")
	set(CMAKE_TRY_COMPILE_CONFIGURATION "${CMAKE_BUILD_TYPE}")

	add_compile_options(/diagnostics:column)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/sbin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/lib)

add_subdirectory(3rd_party)

add_executable(sky_contest main.cpp)
target_link_libraries(sky_contest glad glfw imgui efsw)

add_custom_target(assets_dir_link
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/assets ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets)

add_dependencies(sky_contest assets_dir_link)