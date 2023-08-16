TorchGL Framework
=======

This is an extension of the basic CppGL framework. Its focus is the use for research applications in the context of interactive/ real-time neural rendering.

## Prerequisites

    * Linux C++17 development environment (build-essential, etc) (we use gcc-12 and g++-12 in the build script, but you can swap the by version available on your system)
    * recent gcc/g++ with support for std::filesystem
    * OpenGL, GLEW (libglew-dev)
    * GLFW (libglfw3-dev)
    * CMake (cmake)
    * conda or miniconda
    * 15 GB of disk space

The CppGL framework is already included as git subtree. 

## Setup

1. Run ```setup_torch.sh```. This will automatically download and unpack libtorch to ```external/thirdparty/``` and setup a conda environment that has all the necesary CUDA things.
2. Run ```conda activate torchgl``` to activate the new environment 
2. To build ```libtorchgl``` and the example execute ```./build.sh```. Adapt your compiler versions in the script if needed. 
3. To run the example
```
cd examples
./example_tgl
```

## Issues / Suggestions / Feedback

Please mail to <laura.fink@fau.de>.
