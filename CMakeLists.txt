project("LiveNVS")
# fbx sdk
SET(CMAKE_MIN_REQ_VERSION 3.1.0) # variable needs to be set for fbxsdk settings

cmake_minimum_required(VERSION ${CMAKE_MIN_REQ_VERSION})
# set project name

# ---------------------------------------------------------------------
# path management

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# define compiler flags
# ---------------------------------------------------------------------
# compiler options

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

# setup for included dependencies
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
set(CMAKE_LINK_DIRECTORIES_BEFORE ON)


if(UNIX)
    add_compile_options(-pthread -march=native)
    add_compile_options(-Wall -Wno-unused-parameter -Wno-unused-variable -Wno-int-to-pointer-cast)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else()
    add_definitions(-DNOMINMAX)
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-D_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING) # wtf
    #with cmake version >3.8, CMAKE_INTERPROCEDURAL_OPTIMIZATION ON breaks windows dll export
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
endif()

# GL preference
set(OpenGL_GL_PREFERENCE "GLVND")
include(GenerateExportHeader)
# --------------------------------------------------------------------
# # traverse source tree
find_package(Torch REQUIRED)
include_directories(${TORCH_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

add_subdirectory(external)
add_subdirectory(src)

# for image loader
# was added via subtree add: 
# git subtree add --prefix external/thirdparty/tinyimageloader https://github.com/syoyo/tinydngloader.git master --squash
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/thirdparty/tinyimageloader")
