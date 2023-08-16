# target name to generate here
set(TARGET torchgl)

# glob source files
file(GLOB_RECURSE HEADERS "*.h")
file(GLOB_RECURSE SOURCES "*.cpp")

# define target
add_library(${TARGET} SHARED ${SOURCES} ${HEADERS})
if(WIN32)
	generate_export_header(${TARGET})
endif()
set_target_properties(${TARGET} PROPERTIES VERSION 1.0 DESCRIPTION "xx")

# install
install(TARGETS ${TARGET} DESTINATION lib)
install(DIRECTORY . DESTINATION include/${TARGET} FILES_MATCHING PATTERN "*.h")

# ----------------------------------------------------------
# dependencies

#private (only this target) include files (this src/)
target_include_directories(${TARGET} PRIVATE .)

#public interface (cppgl.h is an available header file for all dependant projects)
target_include_directories(${TARGET} PUBLIC . FILES_MATCHING PATTERN "./${TARGET}.h")

target_link_libraries(${TARGET} ${OPENGL_LIBRARIES})
if(UNIX)
	target_link_libraries(${TARGET} PUBLIC stdc++fs) # required for std::filesystem
else()
	target_compile_definitions(${TARGET} PUBLIC -DBUILD_TORCHGL_DLL)
endif()

# Torch and Torchvision
# find_package(Torch REQUIRED) defines the following variables:
#
#   TORCH_FOUND        -- True if the system has the Torch library
#   TORCH_INCLUDE_DIRS -- The include directories for torch
#   TORCH_LIBRARIES    -- Libraries to link against
#   TORCH_CXX_FLAGS    -- Additional (required) compiler flags
find_package(Torch REQUIRED NO_DEFAULT_PATH)

# This works!
target_include_directories(${TARGET}  PUBLIC ${TORCH_INCLUDE_DIRS})
target_link_libraries(${TARGET} PUBLIC ${TORCH_LIBRARIES})
# target_include_directories(${TARGET}  PUBLIC TorchVision)
# target_link_libraries(${TARGET} PUBLIC torchvision)
# This does not work!!
# target_include_directories(${TARGET}  PUBLIC Torch)
# target_include_directories(${TARGET}  PUBLIC TorchVision)
# target_link_libraries(${TARGET} PUBLIC Torch) <--- !!! ERROR
# target_link_libraries(${TARGET} PUBLIC TorchVision) <--- !!! ERROR

# cppgl
target_link_libraries(${TARGET} PUBLIC cppgl)

