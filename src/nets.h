#pragma once

#include <torchgl.h>
#include "single-view-geometry.h"

void load_nets( const std::filesystem::path& enc_path, 
                const std::filesystem::path& dec_path, 
                const std::filesystem::path& out_path);

void decode(InteropTexture2D out_col, const std::vector<InteropTexture2D>& fused_feats, const InteropTexture2D& fused_lin_depth);

InteropTexture2DArray encode(const SingleViewGeometry& lg);