# target name to generate here
set(TARGET example)

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
	set(copyDest \"${CMAKE_CURRENT_SOURCE_DIR}/\")
	set(copySource \"${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/*.dll\")
	set(copySourceGlew \"${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/subtrees/glew/bin/*.dll\")
	#THESE STRING ESCAPES NEEDS TO BE THERE
	STRING(REGEX REPLACE "/" "\\\\" copyDest \"${CMAKE_CURRENT_SOURCE_DIR}/\")
	STRING(REGEX REPLACE "/" "\\\\" copySource \"${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/*.dll\")
	STRING(REGEX REPLACE "/" "\\\\" copySourceGlew \"${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/subtrees/glew/bin/*.dll\")
	add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND COMMAND copy ${copySource}  ${copyDest})
	add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND COMMAND copy ${copySourceGlew}  ${copyDest})
endif()

# built libs
target_link_libraries(${TARGET} cppgl)
