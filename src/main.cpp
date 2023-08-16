#include <iostream>
#include <fstream>
#include <functional>
#include <string>

#include <torchgl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#include "globals+gui.h"
#include "manage-datasets.h"
#include "single-view-geometry.h"
#include "preprocessor.h"
#include "rendering.h"
#include "nets.h"

#include <regex>
#include "cxxopts.hpp"


// ---------------------------------------
// extern
extern std::vector<Dataset> datasets; //manage_datasets.cpp

// ---------------------------------------
// globals
globals_n_gui gg;
std::shared_ptr<Preprocessor> preprocessor;


bool move_forwards = false;
bool move_backwards = false;
bool reset = false;
std::shared_ptr<CameraTrajectoryControl> ui_trajectory;

// from https://github.com/pytorch/pytorch/issues/49460
// it's unnecessary to invoke this function, just enforce library compiled
bool dummy() {
    std::regex regstr("Why");
    std::string s = "Why crashed";
    auto twos = 2*torch::full({ 4,64,64 }, 1.).to(device_str);
    bool thing = std::regex_search(s, regstr);
    return thing;
}

void gui_callback(){
    gg.gui_callback();
}

// ---------------------------------------
// callbacks

void keyboard_callback(int key, int scancode, int action, int mods) {
    if (ImGui::GetIO().WantCaptureKeyboard) return;
    if (mods == GLFW_MOD_SHIFT && key == GLFW_KEY_R && action == GLFW_PRESS)
        reload_modified_shaders();
    if (key == GLFW_KEY_F2 && action == GLFW_PRESS) make_camera_current(Camera::find("default"));
    if (key == GLFW_KEY_F3 && action == GLFW_PRESS) make_camera_current(Camera::find("dataset_cam"));
    // if (key == GLFW_KEY_F5 && action == GLFW_PRESS) {
    //     static bool wireframe = false;
    //     wireframe = !wireframe;
    //     glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    // }
    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
        move_backwards = true;
        move_forwards = false;
    }
    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
        move_forwards = true;
        move_backwards = false;
    }
    if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE) {
        move_backwards = false;
        move_forwards = false;
    }
    if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE) {
        move_forwards = false;
        move_backwards = false;
    }
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        reset = true;
    }
}


void mouse_button_callback(int button, int action, int mods) {
    if (ImGui::GetIO().WantCaptureMouse) return;
}

// ------------------------------------------
// main

int main(int argc, const char** argv) {
    std::cout << dummy() << std::endl;

    std::cout <<  "-------------------------------------" << std::endl << "" << std::endl;
    torch::NoGradGuard no_grad;

    ////////////////////////////////////////////////////////////////////////
    // setup and parsing of arguments
    cxxopts::Options options("create_dataset_from", "This is a scriptable dataset creation program");
    options.add_options()
        ("h,help", "Print this help info") 
        ("i,interactive", "Start in interactive mode, creation can then be started by pressing [ENTER]", cxxopts::value<bool>()->default_value("false")) 
        ("d,debug", "Enable debugging") // a bool parameter

        //input stuff
        //datasets
        ("datasets", "path to dataset config files. order: init, eval, <other>", cxxopts::value<std::vector<std::string>>())
        ("dataset_rootdir", "path to dataset rootdir. rootdir in config will be ignored! handy for eval scripting.", cxxopts::value<std::string>()->default_value(std::string()))

        ("eval_output_dir", "path to write out imgs of traj given by colmap_eval_txt_dir.", cxxopts::value<std::string>()->default_value(""))

        //limit loaded images from dataset
        ("max_images", "Limit number of max images loaded", cxxopts::value<int>()->default_value("1000"))
        ("freq_images", "frequency of loaded images", cxxopts::value<int>()->default_value("1"))
        ("find_keyframes", "prepocess dataset to find keyframes", cxxopts::value<bool>()->default_value("false"))
        ("write_keyframes_colmap_dir", "wirte poses of found colmap files to this dir", cxxopts::value<std::string>()->default_value(""))
        ("preload_dataset", "preload all the data for dataset", cxxopts::value<bool>()->default_value("false"))
        ("fused_depth_out_dir", "path to write fused depth to eg. /path/to/scnene00_0000/fused_depth", cxxopts::value<std::string>()->default_value(""))
        ("write_fused_depths", "writes out all fused_depthmaps to fused_depth_out_dir after loading and preprocessing the dataset", cxxopts::value<bool>()->default_value("false"))


        // nns
        
        ("inmode", "valid auxillary inputs: c: neural confidence, output by enc net, d: fused depth, input for anc and ref net, v: variance, input for ref net, o: original depth input for enc net, input for ref net is fused depth", cxxopts::value<std::string>()->default_value("rgb"))
        ("e, encoder_path", "Path to encoder net (torch trace)", cxxopts::value<std::string>()->default_value(""))
        // ("m, mlp_path", "Path to mlp net (torch trace)", cxxopts::value<std::string>()->default_value(""))
        ("r, decoder_path", "Path to decoder net (torch trace)", cxxopts::value<std::string>()->default_value(""))
        ("o, outconv_path", "Path to outconv net (torch trace)", cxxopts::value<std::string>()->default_value(""))

        //output stuff
        ("output_colmap_txt_dir", "When this is given in combination with a non-colmap input dataset, this", cxxopts::value<std::string>()->default_value(""))
        
        ("max_dataset_output_res", "Maximum resolution of an image default: 2048px. \n WARNING: larger images than given below will be cropped!!!", cxxopts::value<int>()->default_value("2150"))
        //("i,integer", "Int param", cxxopts::value<int>())
        //("if,input file", "File name", cxxopts::value<std::string>())
        ("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))
        ;

    auto result = options.parse(argc, argv);
    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }
    std::cout << dummy() << std::endl;

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // init GL
    ContextParameters params;
    params.width = gg.WIDTH;
    params.height = gg.HEIGHT;
    params.title = "LiveNVS";
    params.floating = GLFW_TRUE;
    //params.resizable = GLFW_FALSE;
    params.swap_interval = 1;
    Context::init(params); // TODO FIXME invalid query object msg
    Context::set_keyboard_callback(keyboard_callback);
    Context::set_mouse_button_callback(mouse_button_callback);
    
    gui_add_callback("Main GUI", gui_callback);

    //////////////////////////////////////////////////////////////////////////////////////////////////
    //// init nets
    const std::string arg_inmode = std::string(result["inmode"].as<std::string>());
    const std::filesystem::path arg_enc_path(result["encoder_path"].as<std::string>());
    const std::filesystem::path arg_dec_path(result["decoder_path"].as<std::string>());
    const std::filesystem::path arg_outconv_path(result["outconv_path"].as<std::string>());
    load_nets(arg_enc_path, arg_dec_path, arg_outconv_path);

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // load dataset 
    if (result.count("datasets") > 0){
        std::vector<std::string> arg_dataset_configs = result["datasets"].as<std::vector<std::string>>();
        for (const auto& dc : arg_dataset_configs)
            load_dataset_from_config(dc, result["dataset_rootdir"].as<std::string>());
        active_dataset = std::make_shared<Dataset>(datasets[0]);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // process dataset
    preprocessor = std::make_shared<Preprocessor>();
    const int arg_max_images(result["max_images"].as<int>());
    const int arg_freq_images(result["freq_images"].as<int>());
    const bool arg_find_keyframes(result["find_keyframes"].as<bool>());
    const bool arg_preload_dataset(result["preload_dataset"].as<bool>());
    const std::string arg_write_keyframes_colmap_dir(result["write_keyframes_colmap_dir"].as<std::string>());
    if (arg_preload_dataset && active_dataset ){
        std::cout <<  "-------------------------------------" << std::endl << "Process dataset " << std::endl;
        if (!active_dataset->mesh_path.empty()) {
            preprocessor->process_dataset_from_mesh(*active_dataset, arg_max_images, arg_freq_images);
        } else {
            if (arg_find_keyframes){
                processed_dataset = preprocessor->process_dataset_from_depthmap_find_keyframes(*active_dataset, arg_max_images, arg_freq_images); // TODO XXX
                if(!arg_write_keyframes_colmap_dir.empty())
                    processed_dataset.export_as_colmap(arg_write_keyframes_colmap_dir);
                else std::cout << "WARNING: write_keyframes_colmap_dir not given, found keyframes are not exported!" << std::endl;
            } else {
                processed_dataset = preprocessor->process_dataset_from_depthmaps(*active_dataset, arg_max_images, arg_freq_images); // TODO XXX
            }
        }
        std::cout <<  "-------------------------------------" << std::endl;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // camera controls things
    ui_trajectory = std::make_shared<CameraTrajectoryControl>(*active_dataset, true);

    auto dataset_cam = Camera("dataset_cam");   
    current_camera()->update();
    ui_trajectory->reset_camera(dataset_cam);
    make_camera_current(dataset_cam);
    Camera default_cam = Camera::find("default");
    default_cam->fix_up_vector = false;
    ui_trajectory->set_cppgl_camera_to_current(default_cam);
    default_cam->update();

    //////////////////////////////////////////////////////////////////////////////////////////////////
    //// render resources
    init_render_resources();

    // run
    while (Context::running()) {

        static uint32_t frame_counter = 0;

        // handle input
        glfwPollEvents();
        // update and reload shaders
        if ("default" == current_camera()->name){
            // default cam update
            CameraImpl::default_input_handler(Context::frame_time());
            Camera::find("default")->update(); 
        }

        // interactive dataset_cam trajectory control
        if(reset) {
            reset = false;
            ui_trajectory->reset_camera(dataset_cam);
        }

        static uint32_t last_counter = 0;
        if(frame_counter > last_counter+5){
            if (move_forwards){
                ui_trajectory->move_camera_forward(dataset_cam);
                std::cout << ui_trajectory->current().image_name << std::endl;
                last_counter = frame_counter;
            }

            if (move_backwards) {
                ui_trajectory->move_camera_backward(dataset_cam);
                std::cout << ui_trajectory->current().image_name << std::endl;
                last_counter = frame_counter;
            }
        }
    
        if (frame_counter++ % 100 == 0)
            reload_modified_shaders();

        // render all lgs
        render();
        glClear(GL_DEPTH_BUFFER_BIT);// TODO: blit correct depth from render pass to default framebuffer 
        CameraVisualization::display_all_active();

        // finish frame
        Context::swap_buffers();
    }
}
