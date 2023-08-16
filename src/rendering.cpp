#include <iostream>

#include "rendering.h"
#include "single-view-geometry.h"
#include "globals+gui.h"
#include "shader/shader-data-structs.h"
#include "view-selector.h"
#include "nets.h"
#include "encoded-cache.h"

extern globals_n_gui gg;


using namespace std;

float render_settings::tile_size = 10;

void setup_light(const Shader& shader) {
	shader->uniform("ambient_col", glm::vec3(0.32f, 0.34f, 0.36f));
	shader->uniform("light_dir", glm::normalize(glm::vec3(1.f, -0.6f, -0.4f)));
	shader->uniform("light_col", glm::vec3(0.9f, 0.85f, 0.7f));
    shader->uniform("cam_pos", current_camera()->pos);
}


void deferred_debug_pass(const Framebuffer& gbuffer) {
    static auto shader = Shader("deferred-debug", "shader/copytex.vs", "shader/deferred-debug.fs");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader->bind();
    setup_light(shader);
    shader->uniform("gbuf_depth", gbuffer->depth_texture, 0);
    shader->uniform("gbuf_diff", gbuffer->color_textures[0], 1);
    shader->uniform("gbuf_pos", gbuffer->color_textures[1], 2);
    shader->uniform("gbuf_norm", gbuffer->color_textures[2], 3);
    shader->uniform("near_far", glm::vec2(current_camera()->near, current_camera()->far));
	glDisable(GL_DEPTH_TEST);
    Quad::draw();
	glEnable(GL_DEPTH_TEST);
    shader->unbind();
}

void deferred_shading_pass(const Framebuffer& gbuffer) {
    static auto shader = Shader("deferred-lighting", "shader/copytex.vs", "shader/deferred-lighting.fs");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader->bind();
    setup_light(shader);
    shader->uniform("gbuf_depth", gbuffer->depth_texture, 0);
    shader->uniform("gbuf_diff", gbuffer->color_textures[0], 1);
    shader->uniform("gbuf_spec", gbuffer->color_textures[1], 2);
    shader->uniform("gbuf_pos", gbuffer->color_textures[2], 3);
    shader->uniform("gbuf_norm", gbuffer->color_textures[3], 4);
    shader->uniform("near_far", glm::vec2(current_camera()->near, current_camera()->far));
    shader->uniform("screenres", glm::vec2(Context::resolution()));
    Quad::draw();
    shader->unbind();
}

void tiled_blit(const std::vector<Texture2D>& texs){
	if(texs.size() > 4){
		std::cout << "too many textures! return ..." << std::endl;
		return;
	}
	static auto shader = Shader("tiled-blit", "shader/copytex.vs", "shader/tiled-blit2x2.fs");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader->bind();
    setup_light(shader);
    for (uint32_t i = 0; i < texs.size(); ++i){
    	shader->uniform("tex"+std::to_string(i), texs[i], i);
    }

    glDisable(GL_DEPTH_TEST);
    Quad::draw();
	glEnable(GL_DEPTH_TEST);
    Quad::draw();
    shader->unbind();
}

void blit(const std::shared_ptr<Texture2D>& tex) {
    blit(*tex);
}

void blit(const Texture2D& tex) {
	static auto shader = Shader("blit", "shader/copytex.vs", "shader/copytex.fs");
	shader->bind();
    shader->uniform("tex", tex, 0);
	glDisable(GL_DEPTH_TEST);
    Quad::draw();
	glEnable(GL_DEPTH_TEST);
    shader->unbind();
}


void blit_weight_buffer(const int w, const int h) {
	static auto shader = Shader("blit_weight_buffer", "shader/blit_weight_buffer.vs", "shader/blit_weight_buffer.fs");
	shader->bind();
    shader->uniform("inv_proj_view", glm::inverse(current_camera()->proj*current_camera()->view));
    shader->uniform("tgt_cam_pos_ws", current_camera()->pos);
    shader->uniform("WIDTH", w);
    shader->uniform("HEIGHT", h);
    shader->uniform("exponent", gg.weight_exponent);
	glDisable(GL_DEPTH_TEST);
    Quad::draw();
	glEnable(GL_DEPTH_TEST);
    shader->unbind();
}


void blit_fusion_buffer(const int w, const int h) {
	static auto shader = Shader("blit_fusion_buffer", "shader/copytex.vs", "shader/blit_buffer.fs");
	shader->bind();
    shader->uniform("WIDTH", w);
    shader->uniform("HEIGHT", h);
	glDisable(GL_DEPTH_TEST);
    Quad::draw();
	glEnable(GL_DEPTH_TEST);
    shader->unbind();
}

void layered_blit_fusion_buffer(const int w, const int h, const int layers) {
	static auto shader = Shader("layered_blit_fusion_buffer", "shader/copytex.vs", "shader/pass_to_layers.gs","shader/layered_blit_buffer.fs");
	shader->bind();
    shader->uniform("layers", layers);
    shader->uniform("WIDTH", w);
    shader->uniform("HEIGHT", h);
	glDisable(GL_DEPTH_TEST);
    Quad::draw();
	glEnable(GL_DEPTH_TEST);
    shader->unbind();
}

void blit_neural_fusion_buffer(const int w, const int h) {
	static auto shader = Shader("blit_neural_fusion_buffer", "shader/blit_neural_buffer.vs", "shader/blit_neural_buffer.fs");
	shader->bind();
    shader->uniform("inv_proj_view", glm::inverse(current_camera()->proj*current_camera()->view));
    shader->uniform("tgt_cam_pos_ws", current_camera()->pos);
    shader->uniform("WIDTH", w);
    shader->uniform("HEIGHT", h);
	glDisable(GL_DEPTH_TEST);
    Quad::draw();
	glEnable(GL_DEPTH_TEST);
    shader->unbind();
}

void layered_blit_neural_fusion_buffer(const int w, const int h, const int layers) {
	static auto shader = Shader("layered_blit_neural_fusion_buffer", "shader/blit_neural_buffer.vs", "shader/layered_blit_neural_buffer.gs", "shader/layered_blit_neural_buffer.fs");
	shader->bind();
    shader->uniform("inv_proj_view", glm::inverse(current_camera()->proj*current_camera()->view));
    shader->uniform("tgt_cam_pos_ws", current_camera()->pos);
    shader->uniform("layers", layers);
    shader->uniform("WIDTH", w);
    shader->uniform("HEIGHT", h);
	glDisable(GL_DEPTH_TEST);
    Quad::draw();
	glEnable(GL_DEPTH_TEST);
    shader->unbind();
}

void render_depthmap(const Texture2D& nonlin_depth_tex, const int w, const int h, const glm::mat4& inv_proj_view){
    static auto shader = Shader("render_depthmap", "shader/copytex.vs", "shader/render_depthmap.fs");
    shader->bind();
    shader->uniform("depth_tex", nonlin_depth_tex, 0);
    shader->uniform("WIDTH", w);
    shader->uniform("HEIGHT", h);
    shader->uniform("inv_proj_view", inv_proj_view);
	glDisable(GL_DEPTH_TEST);
    Quad::draw();
	glEnable(GL_DEPTH_TEST);
    shader->unbind();
}


void depth_nonlinear_to_linear(const Texture2D& nonlin_depth_tex, const glm::mat4& inv_mat, glm::vec3 range_invalid){
    static auto shader = Shader("depth_nonlinear_to_linear", "shader/copytex.vs", "shader/depth_nonlinear_to_linear.fs");
    shader->bind();
    shader->uniform("valid_range_min_max_invalid", range_invalid);
    shader->uniform("depth_tex", nonlin_depth_tex, 0);
    shader->uniform("inv_mat", inv_mat);
	glDisable(GL_DEPTH_TEST);
    Quad::draw();
	glEnable(GL_DEPTH_TEST);
    shader->unbind();
}


void render_confidencemap(const int w, const int h){
    blit_weight_buffer(w,h);
}


void render_geom_mask(const Texture2D& nonlin_depth_tex, const int w, const int h){
    static auto shader = Shader("render_geom_mask", "shader/copytex.vs", "shader/geom_mask.fs");
    shader->bind();
    shader->uniform("depth_tex", nonlin_depth_tex, 0);
    shader->uniform("WIDTH", w);
    shader->uniform("HEIGHT", h);
	glDisable(GL_DEPTH_TEST);
    Quad::draw();
	glEnable(GL_DEPTH_TEST);
    shader->unbind();
}


void blit_interp_color(const Texture2D& tex, const Texture2D& alpha_tex, const glm::vec4& color=glm::vec4(1)) {
	static auto shader = Shader("interp_tex_color", "shader/copytex.vs", "shader/blend_tex_with_color.fs");
	shader->bind();
    shader->uniform("tex", tex, 0);
    shader->uniform("alpha_tex", alpha_tex, 1);
    shader->uniform("color", color);
	glDisable(GL_DEPTH_TEST);
    Quad::draw();
	glEnable(GL_DEPTH_TEST);
    shader->unbind();
}


extern std::shared_ptr<ViewSelector> view_selector;
extern std::shared_ptr<EncodedCache<SingleViewGeometry, InteropTexture2DArray>> enc_cache;

SSBO counter_buffer;
SSBO lock_buffer;
SSBO fragmentdata_buffer;
// SSBO deferredfusedata_buffer;
void init_fusion_buffers(){
    counter_buffer = SSBO("counter", gg.WIDTH*gg.HEIGHT*sizeof(int));
    lock_buffer = SSBO("locks", gg.WIDTH*gg.HEIGHT*sizeof(int));
    fragmentdata_buffer = SSBO("fragmentdata_buffer", gg.WIDTH*gg.HEIGHT*sizeof(FragmentData)); 
    // deferredfusedata_buffer = SSBO("deferredfusedata_buffer", view_selector->num_closest_views*sizeof(DeferredFusedData));    
}

void reset_fusion_buffers() {
        static Shader reset_buffers = Shader("reset_fusion_buffers", "shader/reset_fusion_buffers.cs");
        reset_buffers->bind();
        counter_buffer->bind_base(0);
        lock_buffer->bind_base(1);
        fragmentdata_buffer->bind_base(2);
        reset_buffers->uniform("WIDTH", gg.WIDTH);
        reset_buffers->uniform("HEIGHT", gg.HEIGHT);
        reset_buffers->dispatch_compute(gg.WIDTH, gg.HEIGHT, 1);
        reset_buffers->unbind();
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
} 


// resources
Framebuffer fusedfeature_buffer;
Framebuffer linear_depth_buffer;
Framebuffer geom_mask_buffer;
Framebuffer confidence_buffer;
Framebuffer shaded_geom_buffer;

Texture2D fused_nonlin_depth;
InteropTexture2D fused_lin_depth;
std::vector<InteropTexture2D> fused_feats;
Texture2D fused_tgt_view;
Texture2D fused_src_view;
InteropTexture2D out_col;
Texture2D geom_mask;
Texture2D confidence;
Texture2D shaded_geom;

void init_textures_and_framebuffers(){
    //////////////////////////////////////////////////////////////////////
    // we need an extra depth tex as we cannot write into a deptch_component tex in a compute shader
    fused_nonlin_depth = Texture2D("fused_nonlin_depth", gg.WIDTH, gg.HEIGHT, GL_R32F, GL_RED, GL_FLOAT);
    fused_lin_depth = InteropTexture2D("fused_lin_depth", gg.WIDTH, gg.HEIGHT, GL_R16F, GL_RED, GL_HALF_FLOAT);
    for(uint32_t i = 0; i < FEAT_SIZE; i++){
        fused_feats.push_back(InteropTexture2D("fused_feat_"+std::to_string(i),
                                gg.WIDTH, gg.HEIGHT, GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT));       
    }

    fused_tgt_view = Texture2D("fused_tgt_view", gg.WIDTH, gg.HEIGHT, GL_RGB32F, GL_RGB, GL_FLOAT);      
    fused_src_view = Texture2D("fused_src_view", gg.WIDTH, gg.HEIGHT, GL_RGB32F, GL_RGB, GL_FLOAT);     

    out_col = InteropTexture2D("out_col", gg.WIDTH, gg.HEIGHT, GL_RGB32F, GL_RGB, GL_FLOAT);    

    {
        fusedfeature_buffer = Framebuffer("fusedfeature_buffer", gg.WIDTH, gg.HEIGHT);
        fusedfeature_buffer->attach_depthbuffer(Texture2D("fused_depth", gg.WIDTH, gg.HEIGHT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT));
        fusedfeature_buffer->attach_colorbuffer(fused_tgt_view);
        fusedfeature_buffer->attach_colorbuffer(fused_src_view);
        fusedfeature_buffer->attach_colorbuffer(fused_nonlin_depth);
        for(auto& fused_feat : fused_feats)
            fusedfeature_buffer->attach_colorbuffer(fused_feat->texture2D);
        fusedfeature_buffer->check();
    }
    {
        linear_depth_buffer = Framebuffer("linear_depth_buffer", gg.WIDTH, gg.HEIGHT);
        linear_depth_buffer->attach_colorbuffer(fused_lin_depth->texture2D);
        linear_depth_buffer->attach_depthbuffer();
        linear_depth_buffer->check();
    }
    {
        geom_mask = Texture2D("geom_mask", gg.WIDTH, gg.HEIGHT, GL_R32F, GL_RED, GL_FLOAT); 
        geom_mask_buffer = Framebuffer("geom_mask_buffer", gg.WIDTH, gg.HEIGHT);
        geom_mask_buffer->attach_colorbuffer(geom_mask);
        geom_mask_buffer->attach_depthbuffer();
        geom_mask_buffer->check();
    }
    {
        confidence = Texture2D("confidence", gg.WIDTH, gg.HEIGHT, GL_RGB32F, GL_RGB, GL_FLOAT); 
        confidence_buffer = Framebuffer("confidence_buffer", gg.WIDTH, gg.HEIGHT);
        confidence_buffer->attach_colorbuffer(confidence);
        confidence_buffer->attach_depthbuffer();
        confidence_buffer->check();
    }
    {
        shaded_geom = Texture2D("shaded_geom", gg.WIDTH, gg.HEIGHT, GL_RGB32F, GL_RGB, GL_FLOAT); 
        shaded_geom_buffer = Framebuffer("shaded_geom_buffer", gg.WIDTH, gg.HEIGHT);
        shaded_geom_buffer->attach_colorbuffer(shaded_geom);
        shaded_geom_buffer->attach_depthbuffer();
        shaded_geom_buffer->check();
    }
}

static TimerQueryGL view_selection_timer;
static TimerQueryGL view_selection_timer_cpu;
static TimerQueryGL holefilling_timer;
static TimerQuery warp_timer;
static TimerQueryGL reset_buffers_timer;
static TimerQuery complete_encode_timer;
static TimerQuery src_feat_copy_timer;

void init_timers(){
    view_selection_timer = TimerQueryGL("View Selection (GPU)");
    view_selection_timer_cpu = TimerQueryGL("View Selection (CPU)");
    holefilling_timer = TimerQueryGL("Hole Filling");
    warp_timer = TimerQuery("Warp per View (CPU)");
    reset_buffers_timer = TimerQueryGL("Reset Buffers");
    complete_encode_timer = TimerQuery("Warp+Encode (CPU)");
    src_feat_copy_timer = TimerQuery("Copy Features Per View");
}

void init_render_resources(){
    init_fusion_buffers();
    init_textures_and_framebuffers();
    init_timers();

    // TODO does not fit here right?
    view_selector = make_shared<ViewSelector>();
    auto fun = std::function<InteropTexture2DArray (const SingleViewGeometry&)>(encode);
    enc_cache = make_shared<EncodedCache<SingleViewGeometry, InteropTexture2DArray>>(fun);

}

Texture2D render_color(){
    auto local_geoms = view_selector->get_closest();

    complete_encode_timer->begin();
    fusedfeature_buffer->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for(const auto& lg : local_geoms){
        warp_timer->begin();
        lg->draw();
        warp_timer->end();
    }
    fusedfeature_buffer->unbind();
    complete_encode_timer->end();

    fusedfeature_buffer->bind();
    fragmentdata_buffer->bind_base(2);
    blit_neural_fusion_buffer(gg.WIDTH, gg.HEIGHT);
    fusedfeature_buffer->unbind();                

    return fused_feats[0]->texture2D;
}


Texture2D render_neural_forward(){
    complete_encode_timer->begin();
    // prepare render target
    fusedfeature_buffer->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // select views
    auto local_geoms = view_selector->get_closest();
    // render selected view geometries
    for(const auto& lg : local_geoms){
        // query cache for encoded views
        if(!enc_cache->misses_left()) break;    
        auto src_feat = enc_cache->get_encoded(lg);  

        warp_timer->begin();
        // warp view into
        lg->draw( src_feat->texture2Darray );
        warp_timer->end();
    }
    fusedfeature_buffer->unbind();
    enc_cache->reset_misses();
    complete_encode_timer->end();

    // blit from buffer to textures
    fusedfeature_buffer->bind();
    fragmentdata_buffer->bind_base(2);
    blit_neural_fusion_buffer(gg.WIDTH, gg.HEIGHT);
    fusedfeature_buffer->unbind();              

    // convert depth
    linear_depth_buffer->bind();
    depth_nonlinear_to_linear(fused_nonlin_depth, glm::inverse(current_camera()->proj), glm::vec3(0.01f, 30.f,30.f));
    linear_depth_buffer->unbind();

    decode(out_col, fused_feats, fused_lin_depth);  

    return out_col->texture2D;
}



Texture2D render_neural_deferred(){
    // select views
    auto local_geoms_depth = view_selector->get_all_static();
    auto local_geoms = view_selector->get_closest();
    glm::mat4 inv_proj_view = glm::inverse(current_camera()->proj*current_camera()->view);

    complete_encode_timer->begin();
    // prepare render target
    fusedfeature_buffer->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for(auto& lg : local_geoms_depth) {
        // bind buffers
        counter_buffer->bind_base(0);
        lock_buffer->bind_base(1);
        fragmentdata_buffer->bind_base(2);
        
        // draw to fused_feat textures
        lg->draw( Shader::find("Scene: localgeom_deferred"), GL_TRIANGLES, gg.cut_off_depth_deferred[0], true );
    }
    fusedfeature_buffer->unbind();

    // blit buffer
    fusedfeature_buffer->bind();
    fragmentdata_buffer->bind_base(2);
    blit_neural_fusion_buffer(gg.WIDTH, gg.HEIGHT);
    fusedfeature_buffer->unbind();

    // render feature maps of selected view geometries
    for(auto& lg : local_geoms) {
        
        if(!enc_cache->misses_left()) break;
        auto src_feat = enc_cache->get_encoded(lg);

        warp_timer->begin();
        lg->depth_texture->texture2D->bind(0);
        glGenerateMipmap(GL_TEXTURE_2D);
        lg->depth_texture->texture2D->unbind();


        static Shader deferred_feat_pass = Shader("deferred-frag-fusion-feat-pass", "shader/deferred-frag-fusion-feat-pass.cs");
        deferred_feat_pass->bind();
        deferred_feat_pass->uniform("WIDTH", gg.WIDTH);
        deferred_feat_pass->uniform("HEIGHT", gg.HEIGHT);
        deferred_feat_pass->uniform("tgt_depth_texx", fused_nonlin_depth, 0);
        deferred_feat_pass->uniform("src_depth_tex", lg->depth_texture->texture2D, 1);
        deferred_feat_pass->uniform("src_feat", src_feat->texture2Darray, 2);
        deferred_feat_pass->uniform("inv_tgt_proj_view", inv_proj_view);
        deferred_feat_pass->uniform("inv_tgt_proj", glm::inverse(current_camera()->proj));
        deferred_feat_pass->uniform("src_proj_view", glm::inverse(lg->inv_view_proj));
        deferred_feat_pass->uniform("inv_src_proj", glm::inverse(lg->cam.get_gl_proj(gg.NEAR, gg.FAR)));
        deferred_feat_pass->uniform("tgt_cam_wpos", current_camera()->pos);
        deferred_feat_pass->uniform("src_cam_wpos", lg->cam.get_gl_campos());
        deferred_feat_pass->uniform("use_weights", glm::ivec4(gg.use_vignette_weight, gg.use_view_weight, gg.use_depth_weight, gg.weight_exponent));
        deferred_feat_pass->uniform("use_background", gg.use_background_deferred);
        deferred_feat_pass->uniform("background_t", gg.inf_depth_deferred);
        
        // bind buffers
        fragmentdata_buffer->bind_base(2);

        deferred_feat_pass->dispatch_compute(gg.WIDTH, gg.HEIGHT);

        deferred_feat_pass->unbind();
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        warp_timer->end();
    }
    enc_cache->reset_misses();
    complete_encode_timer->end();

    // blit from buffer to textures
    fusedfeature_buffer->bind();
    fragmentdata_buffer->bind_base(2);
    blit_neural_fusion_buffer(gg.WIDTH, gg.HEIGHT);
    fusedfeature_buffer->unbind();              

    // convert depth
    linear_depth_buffer->bind();
    depth_nonlinear_to_linear(  fused_nonlin_depth, glm::inverse(current_camera()->proj), 
                                glm::vec3(0.01f, 30.f,30.f)); // 30.0 was the clamped depth the net was trained with
    linear_depth_buffer->unbind();

    decode(out_col, fused_feats, fused_lin_depth);  

    return out_col->texture2D;
}


void render_surface(){

}

void render_confidence_map(){


}

void auxiliaries_gui(){
    ImGui::Begin("Auxiliaries"); 
    float width = 0.5f*0.95f*ImGui::GetWindowWidth();
    // gui
    float aspect_ratio = (gg.HEIGHT/(float)gg.WIDTH);

    ImGui::Text("Shaded Geometry & Confidence Map");
    ImGui::Image((ImTextureID) shaded_geom->id, ImVec2(width, aspect_ratio*width), 
        ImVec2(0, 1), ImVec2(1, 0), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5));
    ImGui::SameLine();
    ImGui::Image((ImTextureID) confidence->id, ImVec2(width, aspect_ratio*width), 
        ImVec2(0, 1), ImVec2(1, 0), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5));

    ImGui::Text("Features");
    ImGui::Image((ImTextureID) fused_feats[0]->texture2D->id, ImVec2(width, aspect_ratio*width), 
        ImVec2(0, 1), ImVec2(1, 0), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5));
    ImGui::SameLine();
    ImGui::Image((ImTextureID) fused_feats[1]->texture2D->id, ImVec2(width, aspect_ratio*width), 
        ImVec2(0, 1), ImVec2(1, 0), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5));
    ImGui::End();
}

void render_aux_features(){
    // shaded depth map
    shaded_geom_buffer->bind();
    glm::mat4 inv_proj_view = glm::inverse(current_camera()->proj*current_camera()->view);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    render_depthmap(fused_nonlin_depth, gg.WIDTH, gg.HEIGHT,inv_proj_view);
    shaded_geom_buffer->unbind();

    // confidence map
    confidence_buffer->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    render_confidencemap(gg.WIDTH, gg.HEIGHT);
    confidence_buffer->unbind();

    // orig color 
    // always in in_feat[0] but might be in [-1,1]
}



void render(){
    // ----------------------------------------------------
    // render

    // reset fusion buffers
    reset_buffers_timer->begin();
    reset_fusion_buffers();
    reset_buffers_timer->end();

    // select and get views
    view_selection_timer->begin();
    view_selection_timer_cpu->begin();
    view_selector->sort_views();
    view_selection_timer->end();
    view_selection_timer_cpu->end();

    Texture2D ret_col;
    switch (SingleViewGeometryImpl::mode){
        case SingleViewGeometryImpl::POINTS:
        case SingleViewGeometryImpl::COL_FORWARD:
            ret_col = render_color();
            break;
        case SingleViewGeometryImpl::N_FORWARD:
            ret_col = render_neural_forward();
            break;
        case SingleViewGeometryImpl::N_DEFERRED:
            ret_col = render_neural_deferred();
            break;
        default:
            std::cout << "Unknown mode" << std::endl;
    }

    // blit to screen 
    if(gg.mask_background){
        fused_nonlin_depth->bind(0);
        glGenerateMipmap(GL_TEXTURE_2D);
        fused_nonlin_depth->unbind();
        geom_mask_buffer->bind();
        render_geom_mask(fused_nonlin_depth, gg.WIDTH, gg.HEIGHT);
        geom_mask_buffer->unbind();
        blit_interp_color(ret_col, geom_mask);
    }else{
        // blit to screen 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        blit(ret_col);
    }

    if(gg.render_aux){
        render_aux_features();
    }

    if (view_selector->show_frusta){
        glClear(GL_DEPTH_BUFFER_BIT); // TODO: blit correct depth from render pass to default framebuffer 
        for(const auto& svg : view_selector->get_closest()){
            svg->show_frustum();
        }
    }
}
