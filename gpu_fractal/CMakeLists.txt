cmake_minimum_required(VERSION 3.16)

include(generic_compile_options)

set(CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/code)
file(GLOB_RECURSE target_headers ${CODE_DIR}/*.hpp)
file(GLOB_RECURSE target_sources ${CODE_DIR}/*.cpp)

set(content_dir ${CMAKE_CURRENT_SOURCE_DIR}/content)

set(target_name gpu_fractal)
add_executable(${target_name} ${target_sources} ${target_headers})
set_generic_compile_options(${target_name} PUBLIC)
target_include_directories(${target_name} PUBLIC ${CODE_DIR})
target_link_libraries(${target_name} PUBLIC lib_fractal klgl)

add_custom_command(TARGET ${target_name}
	POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
	${content_dir}
	$<TARGET_FILE_DIR:${target_name}>/content)