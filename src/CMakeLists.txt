set(TARGET "livenvs")
set(CMAKE_CXX_STANDARD 17)
add_definitions(-DPROJECT_NAME="${TARGET}")
# glob for source files
file(GLOB_RECURSE HEADERS "*.h")
file(GLOB_RECURSE SOURCES "*.cpp")
# file(GLOB_RECURSE SOURCES "import_utils/*.cpp")

# include_directories("./import_utils")
# include_directories("./export_utils")
# for image loader
# was added via subtree add: 
# git subtree add --prefix external/thirdparty/tinyimageloader https://github.com/syoyo/tinydngloader.git master --squash
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../external/thirdparty/tinyimageloader")


# # executable target
add_executable(${TARGET} ${SOURCES})

# # needed for fbx sdk
# SET_SAMPLES_GLOBAL_FLAGS()

# output in source
set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" CXX_STANDARD 17)

# define base path for mesh/image lookups
add_definitions(-DEXECUTABLE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# # handle dependencies
# # GLEW
# # find_package(GLEW REQUIRED)
# # include_directories(${GLEW_INCLUDE_DIRS})
# #target_link_libraries(neural_renderer ${GLEW_LIBRARIES})
# # OpenGL
# find_package(OpenGL REQUIRED)
# # Toch and Torchvision
# # find_package(TorchVision REQUIRED)

# include_directories(${OPENGL_INCLUDE_DIRS})
# target_include_directories(${TARGET} PUBLIC .)
# target_link_libraries(${TARGET} ${OPENGL_LIBRARIES})


# common libs
if(UNIX)
# std::filesystem
	target_link_libraries(${TARGET} stdc++fs tbb)
else()
	add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../external/torchgl/external/cppgl/external/thirdparty/win/bin/x64" $<TARGET_FILE_DIR:${TARGET}>)
	add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/external/torchgl/torchgl.dll" $<TARGET_FILE_DIR:${TARGET}>)
	#add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/external/thirdparty/win/bin/x64" $<TARGET_FILE_DIR:${TARGET}>)
	add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/external/torchgl/external/cppgl/cppgl.dll" $<TARGET_FILE_DIR:${TARGET}>)
endif()

find_package(Torch REQUIRED NO_DEFAULT_PATH)
#find_package(Torch REQUIRED)

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})
target_link_libraries(${TARGET} torchgl ${PNG_LIBRARY}) # TorchVision::TorchVision) # messages)
