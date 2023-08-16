#include "shader-data-structs.h"

layout (std430, binding=0) buffer counterBuffer {
    int counter[];
};

layout (std430, binding=1) buffer lockBuffer {
    int locks[];
};

layout (std430, binding=2) buffer FragmentDataBuffer {
    FragmentData fragmentdata[];
};

layout (std430, binding=3) buffer DeferredFuseDataBuffer {
    DeferredFusedData deferredfusedata[];
};

// layout (std430, binding=3) buffer rawDataBuffer {
// #ifdef HAS_INT64
//   int64_t data[];
// #else
//   ivec2 data[];
// #endif
// };

// layout (std430, binding=6) buffer rawDataBuffer {
//   float dataraw2[];
// };

