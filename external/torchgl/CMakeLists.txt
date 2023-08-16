project(TorchGL
  LANGUAGES CXX CUDA)
cmake_minimum_required(VERSION 3.10)

if (NOT UNIX AND NOT WIN32)
	message(FATAL_ERROR "Unsupported platform!")
endif()

# ---------------------------------------------------------------------
# path management

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# ---------------------------------------------------------------------
# cmake options

set(BUILD_TORCHGL_EXAMPLES ON)

# ---------------------------------------------------------------------
# compiler options

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
# CUDA
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_EXTENSIONS OFF)
set(CUDA_LINK_LIBRARIES_KEYWORD PUBLIC)
set(CMAKE_CUDA_RUNTIME_LIBRARY Shared)

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

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
include(GenerateExportHeader)

# ---------------------------------------------------------------------
# traverse source tree


# list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/thirdparty/libtorch/share/cmake/Torch")
# libtorch 
find_package(Torch REQUIRED)
include_directories(${TORCH_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

# # additional global includes
find_package(Torch REQUIRED)
# add_subdirectory(external/thirdparty/libtorch)
add_subdirectory(external/cppgl)
add_subdirectory(src)

# if (BUILD_TORCHGL_EXAMPLES)
  add_subdirectory(examples)
# endif()
