cmake_minimum_required(VERSION 3.15)

project(sky_contest)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/sbin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/lib)

if(MSVC)
	add_compile_options(/diagnostics:column)
endif()

add_subdirectory(3rd_party)

add_executable(sky_contest main.cpp)
target_link_libraries(sky_contest glad glfw imgui efsw)

add_custom_command(TARGET sky_contest
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:sky_contest>/assets)