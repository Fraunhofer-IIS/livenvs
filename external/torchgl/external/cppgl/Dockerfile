FROM ubuntu

# set timezone
ENV TZ=Europe/Berlin
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# install deps
RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y build-essential libx11-dev xorg-dev libopengl-dev freeglut3-dev cmake

# build
WORKDIR /workspace
COPY CMakeLists.txt ./
COPY src/ src/
COPY subtrees/ subtrees/
COPY examples/ examples/
RUN cmake -S . -B build -DCPPGL_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release -Wno-dev && cmake --build build --parallel
