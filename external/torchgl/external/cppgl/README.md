CppGL Framework
=======

OpenGL framework for use in teaching and research with focus on simplicity and reusability.

## Dependencies

### Ubuntu (22.04)

    apt-get install -y build-essential libx11-dev xorg-dev libopengl-dev freeglut3-dev cmake
    apt-get install -y libassimp-dev # optional, to reduce compile times

### Windows

    Visual Studio 2019 or newer with cmake support

## Build

Without examples:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -Wno-dev && cmake --build build --parallel

With examples:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCPPGL_BUILD_EXAMPLES=ON -Wno-dev && cmake --build build --parallel

## Examples

Included is an example rendering application loading a ```.obj``` file from the command line and rendering it with a standard diffuse shader.
If no object file is given, a rgb debug color is displayed.
Also included is a compute shader example, which changes the output color to a greyscale image.
The debug menu can be accessed by pressing ```F1```.

## Issues / Suggestions / Feedback

Please mail to <nikolai.hofmann@fau.de>, <laura.fink@fau.de> or <linus.franke@fau.de>.
