#include "nets.h"
#include "globals+gui.h"
#include "rendering.h"

extern globals_n_gui gg;

static torch::jit::script::Module encoder_trace;
static torch::jit::script::Module decoder_trace;
static torch::jit::script::Module outconv_trace;


void load_nets( const std::filesystem::path& enc_path, 
                const std::filesystem::path& dec_path, 
                const std::filesystem::path& out_path){
    std::cout << "Load encoder." << std::endl; 
    encoder_trace  = load_module(enc_path);
    std::cout << "Load decoder." << std::endl; 
    decoder_trace = load_module(dec_path);
    std::cout << "Load outconv." << std::endl; 
    outconv_trace = load_module(out_path);

    decoder_trace.to(torch::kFloat16);
    encoder_trace.to(torch::kFloat16);
    outconv_trace.to(torch::kFloat16);
}

extern Framebuffer linear_depth_buffer;
extern InteropTexture2D fused_lin_depth;
InteropTexture2DArray encode(const SingleViewGeometry& lg) {
    using namespace torch::indexing;
    const std::string arg_inmode("rgbcd"); // net input: rgb_confidence from encoder + lin depth
    static TimerQuery enc_timer("Encode Net");
    enc_timer->begin();

    auto img_tensor = lg->color_texture->copy_as_tensor().to(torch::kFloat16);
    img_tensor = img_tensor.index({Slice(0,3),"..."}).unsqueeze(0);
    map_value_range(img_tensor, {0.,255.}, {-1.,1.});
    
    if (arg_inmode.find('d')!=std::string::npos || arg_inmode.find('o')!=std::string::npos){
        // we need linear depth here :()
        linear_depth_buffer->bind();
        depth_nonlinear_to_linear(lg->depth_texture->texture2D, glm::inverse(current_camera()->proj), glm::vec3(0.01f, 30.f,30.f));
        linear_depth_buffer->unbind();
        auto depth_tensor = fused_lin_depth->copy_as_tensor().to(torch::kFloat16).unsqueeze(0);
        img_tensor = torch::cat({img_tensor, depth_tensor}, 1);
    }

    // pad 
    static const auto dims = img_tensor.sizes();
    static const int pad_0 = (32-(dims[2]%32))%32;
    static const int pad_1 = (32-(dims[3]%32))%32;
    img_tensor = torch::nn::functional::pad(img_tensor, torch::nn::functional::PadFuncOptions({0,pad_1,0,pad_0}));

    //encode
    auto enc_feat_tensor = encoder_trace.forward({img_tensor}).toTensor();

    // unpad
    if(pad_0 > 0 || pad_1 > 0)
        enc_feat_tensor= enc_feat_tensor.index({"...", Slice(0,gg.HEIGHT), Slice(0,gg.WIDTH)});

    if (arg_inmode.find('c')!=std::string::npos){
        // last channel is some confidence map
        auto confidence = torch::sigmoid(enc_feat_tensor.slice(1, -1, enc_feat_tensor.sizes()[1]));
        enc_feat_tensor = torch::cat({enc_feat_tensor.slice(1, 0, 3), enc_feat_tensor.slice(1, 3, -1)*confidence},1); 
    }

    InteropTexture2DArray src_feat = InteropTexture2DArray("src_feat_"+lg->name, enc_feat_tensor.squeeze(0));
    // copy to texture array
    remove_InteropTexture2DArray_from_global_maps("src_feat_"+lg->name);

    if (gg.sync_active) sync_cuda();
    enc_timer->end();
    return src_feat;
}


void decode(InteropTexture2D out_col, const std::vector<InteropTexture2D>& fused_feats, const InteropTexture2D& fused_lin_depth){
    using namespace torch::indexing;
    static TimerQuery ref_timer("Decode Net");

    ref_timer->begin();

    // prepare input
    std::vector<at::Tensor> fused_feat_tensors;
    // copy back to tensors
    for(auto& fused_feat : fused_feats)
        fused_feat_tensors.push_back(fused_feat->copy_as_tensor().unsqueeze(0));    
    fused_feat_tensors.push_back(fused_lin_depth->copy_as_tensor().unsqueeze(0));
    if (gg.sync_active) sync_cuda();

    at::Tensor decode_trace_input = torch::cat(fused_feat_tensors,1);
    // pad 
    static const auto dims = decode_trace_input.sizes();
    static const int pad_0 = (32-(dims[2]%32))%32;
    static const int pad_1 = (32-(dims[3]%32))%32;
    decode_trace_input = torch::nn::functional::pad(decode_trace_input, torch::nn::functional::PadFuncOptions({0,pad_1,0,pad_0}));

    // init history tensors, dimensions are baked
    // TODO kinda ugly... maybe control via config?
    static std::vector<torch::Tensor> prevs;
    if (prevs.size() ==  0){
        if (gg.WIDTH == 1296){
            prevs.push_back(decode_trace_input.new_full({1, 8, 992, 1312},1.0));
            prevs.push_back(decode_trace_input.new_full({1, 16, 496, 656},1.0));
            prevs.push_back(decode_trace_input.new_full({1, 32, 248, 328},1.0));
            prevs.push_back(decode_trace_input.new_full({1, 64, 124, 164},1.0));
            prevs.push_back(decode_trace_input.new_full({1, 64, 62,   82},1.0));
        }
        else if (gg.WIDTH == 1280){
            prevs.push_back(decode_trace_input.new_full({1, 8, 736, 1280},1.0));
            prevs.push_back(decode_trace_input.new_full({1, 16, 368, 640},1.0));
            prevs.push_back(decode_trace_input.new_full({1, 32, 184, 320},1.0));
            prevs.push_back(decode_trace_input.new_full({1, 64, 92, 160},1.0));
            prevs.push_back(decode_trace_input.new_full({1, 64, 46, 80},1.0));
        }
        else if (gg.WIDTH == 1920){
            prevs.push_back(decode_trace_input.new_full({1, 8, 1088, 1920},1.0));
            prevs.push_back(decode_trace_input.new_full({1, 16, 544, 960},1.0));
            prevs.push_back(decode_trace_input.new_full({1, 32, 272, 480},1.0));
            prevs.push_back(decode_trace_input.new_full({1, 64, 136, 240},1.0));
            prevs.push_back(decode_trace_input.new_full({1, 64, 68, 120},1.0));
        } else{
            std::cout << " WIDTH and HEIGHT not supported, check resolution" << std::endl;
        }
    }

    // blend with history
    auto alpha = decode_trace_input.new_full({1},gg.temporal_feedback_weight);
    auto output = decoder_trace.forward({ decode_trace_input, prevs[0], prevs[1], prevs[2], prevs[3], prevs[4], alpha }).toTuple();
    static const int LAYERS = 5;
    for( int i = 0; i < 5; i++)
        prevs[i] = gg.temporal_feedback_weight*prevs[i] + (1.0-gg.temporal_feedback_weight)*output->elements()[i+1].toTensor().clone();
    auto ref_feat_tensor = output->elements()[0].toTensor(); 
    if (gg.sync_active) sync_cuda();

    // unpad
    ref_feat_tensor= ref_feat_tensor.index({"...", Slice(0,gg.HEIGHT), Slice(0,gg.WIDTH)});

    // outconv
    static auto last_ref_feat_tensor = torch::ones_like(ref_feat_tensor).to(device_str);
    static auto out_col_tensor = torch::full({ 1,gg.HEIGHT,gg.WIDTH }, 1.).to(device_str);
    static auto ones = torch::full({ 1,gg.HEIGHT,gg.WIDTH }, 1.).to(device_str);

    out_col_tensor = outconv_trace.forward({ ref_feat_tensor }).toTensor();
    if (gg.sync_active) sync_cuda();

    // post processing map -1,1 to 0,1
    map_value_range(out_col_tensor, { -1.,1. }, { 0.,1. });
    out_col_tensor = out_col_tensor.squeeze(0);

    // back to 32-bit
    out_col_tensor = out_col_tensor.to(torch::kFloat32);

    if (gg.sync_active) sync_cuda();

    // to GL
    out_col->copy_from_tensor(out_col_tensor);
    if (gg.sync_active) sync_cuda();

    ref_timer->end();
}

