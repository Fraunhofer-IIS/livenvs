#pragma once

#include <cppgl.h>
#include <torch/torch.h> // One-stop header.
#include <torch/script.h>

#include "torch_utils.h"
#include "UNet.h"
#include "interop_texture.h"
#include "layered_framebuffer.h"


// TODO: pretty ugly lel
using namespace cppgl;

void testTorch();