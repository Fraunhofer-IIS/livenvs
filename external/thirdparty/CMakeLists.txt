message("=================================")
message("Adding Submodule libPNG")

# libpng configurable options

# set(PNG_BUILD_ZLIB ON CACHE INTERNAL "")
set(PNG_SHARED ON CACHE BOOL "" FORCE)
set(PNG_STATIC OFF CACHE BOOL "" FORCE)
set(PNG_EXECUTABLES OFF CACHE BOOL "" FORCE) # we only want lib
set(PNG_TESTS OFF CACHE BOOL "" FORCE) # we only want lib
set(SKIP_INSTALL_ALL OFF CACHE BOOL "" FORCE) # we only want lib

# include_directories(${ZLIB_INCLUDE_DIRS})
# message("zblib thingy")
# message(STATUS "${ZLIB_INCLUDE_DIRS}")

# add libpng

include_directories(libpng)
# generated files from libpng during build
include_directories(${CMAKE_CURRENT_BINARY_DIR}/libpng)

add_subdirectory(libpng)

# link to libpng

# add_library(png_example SHARED example.cpp)
# link to png_static if build static is on
# target_link_libraries(png_example png)

message("=================================")

