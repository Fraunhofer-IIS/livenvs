#pragma once

#include <torchgl.h>

struct render_settings {
	static float tile_size;
};

// draw random floats in [-1, 1]
inline float random_float() { return (float(rand() % 32768) / 16384.0f) - 1.0f; }
inline glm::vec2 random_vec2() { return glm::vec2(random_float(), random_float()); }
inline glm::vec3 random_vec3() { return glm::vec3(random_float(), random_float(), random_float()); }

void setup_light(const Shader& shader);
void deferred_debug_pass(const Framebuffer& gbuffer);
void deferred_shading_pass(const Framebuffer& gbuffer);
void tiled_blit(const std::vector<Texture2D>& texs);

void blit(const Texture2D& tex);
void blit(const Texture2D& tex);
void blit_weight_buffer(const int w, const int h);
void blit_fusion_buffer(const int w, const int h);
void layered_blit_fusion_buffer(const int w, const int h, const int layers);
void blit_neural_fusion_buffer(const int w, const int h);
void layered_blit_neural_fusion_buffer(const int w, const int h, const int layers);

void render_depthmap(const Texture2D& lin_depth_tex, const int w, const int h, const glm::mat4& inv_proj_view);
void render_confidencemap(const int w, const int h);
void depth_nonlinear_to_linear(const Texture2D& nonlin_depth_tex, const glm::mat4& inv_mat, glm::vec3 range_invalid);

// init render targets, textures, fbos, ...
void init_render_resources();

// fusion stuff
void init_fusion_buffers();
void reset_fusion_buffers();

Texture2D render_color();
Texture2D render_neural_forward();
Texture2D render_neural_deferred();
void render();
void auxiliaries_gui();
