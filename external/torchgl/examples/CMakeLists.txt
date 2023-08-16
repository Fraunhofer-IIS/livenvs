# target name to generate here
set(TARGET example_tgl)

# glob source files
file(GLOB_RECURSE HEADERS "*.h")
file(GLOB_RECURSE SOURCES "*.cpp")

# define target
add_executable(${TARGET} ${SOURCES} ${HEADERS})

# forces executables to be compiled to /example/ folder, to allow relative paths for shaders
set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

# ----------------------------------------------------------
# dependencies
if(WIN32)
	add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../external/thirdparty/win/bin/x64" $<TARGET_FILE_DIR:${TARGET}>)
	add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cppgl.dll" $<TARGET_FILE_DIR:${TARGET}>)
endif()

# As explained in this comment
	# Well, since you don't show origin of that generator expression, we could only guess the reason of the problem. My guessing is that variable TORCH_INCLUDE_DIRS contains this expression, and torch is an IMPORTED target found by find_package(Torch) or so in the subdirectory. Unlike to normal targets (libraries, executables), IMPORTED ones has a local scope. Because of that, torch target is visible in the subdirectory but is not visible in the main directory. This would explain the error you got. See also that CMake issue
# from 
	# https://stackoverflow.com/questions/63511419/cmake-publically-linked-library-cannot-evaluate-generator-expression

# find_package() puts targets and and variables only into local scope.
# Hence, it seems like we need to call find_package again for each new CMakeLists scope... We can't use add_subdirectory() as we obviously only have the pre-built binaries + TorchConfig.cmake (used for find_package())
# Alternatives: 
#	* build libtorch from source?
#	* use get_target_property() as mentioned here (https://gitlab.kitware.com/cmake/cmake/-/issues/19072) to put find_package stuff to global scope?

find_package(Torch REQUIRED)

# built libs
target_link_libraries(${TARGET} PUBLIC torchgl)
